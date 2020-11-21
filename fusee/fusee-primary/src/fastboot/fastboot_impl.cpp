/*
 * Copyright (c) 2018-2020 Atmosphère-NX
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fastboot.h"
#include "fastboot_gadget.h"

#include "../lib/miniz.h"
extern "C" {
#include "../lib/log.h"
#include "../utils.h"
#include "../fs_utils.h"
#include "../sdmmc/sdmmc.h"
#include "../chainloader.h"
#include "../lib/fatfs/ff.h"
}

#include<array>
#include<stdio.h>

#include<vapours/util/util_scope_guard.hpp>

uint32_t crc32(uint32_t crc, const uint8_t *buf, size_t len) {
    int k;

    crc = ~crc;
    while (len--) {
        crc ^= *buf++;
        for (k = 0; k < 8; k++)
            crc = crc & 1 ? (crc >> 1) ^ 0xedb88320 : crc >> 1;
    }
    return ~crc;
}

struct android_boot_image_v2 {
    // version 0
    uint8_t magic[8];

    uint32_t kernel_size;
    uint32_t kernel_addr;

    uint32_t ramdisk_size;
    uint32_t ramdisk_addr;

    uint32_t second_size;
    uint32_t second_addr;

    uint32_t tags_addr;
    uint32_t page_size;

    uint32_t header_version;

    uint32_t os_version;

    uint8_t name[16];
    uint8_t cmdline[512];
    uint32_t id[8];
    uint8_t extra_cmdline[1024];

    // version 1
    uint32_t recovery_dtbo_size;
    uint64_t recovery_dtbo_offset;
    uint32_t header_size;

    // version 2
    uint32_t dtb_size;
    uint64_t dtb_addr;
} __attribute__((packed));

/* Simple bump heap for zip extraction. */
struct MzHeap {
    public:
    
    static void Reset() {
        heap_head = heap;
    }

    static void *Alloc(void *opaque, size_t items, size_t size) {
        if (heap_head + items * size > heap + sizeof(heap)) {
            return nullptr;
        } else {
            uint8_t *ret = heap_head;
            heap_head+= items * size;
            return ret;
        }
    }

    static void Free(void *opaque, void *addr) {
        /* No-op. */
    }

    static void *Realloc(void *opaque, void *addr, size_t items, size_t size) {
        void *new_allocation = Alloc(opaque, items, size);
        memmove(new_allocation, addr, items * size);
        return new_allocation;
    }
    
    private:
    static inline uint8_t heap[8 * 1024 * 1024] __attribute__((section(".dram")));
    static inline uint8_t *heap_head;
};

namespace ams {

    namespace fastboot {

        using ResponseDisposition = FastbootGadget::ResponseDisposition;
        using ResponseToken = FastbootGadget::ResponseToken;

        FastbootImpl::FastbootImpl(FastbootGadget &gadget) : gadget(gadget) {
        }
    
        Result FastbootImpl::ProcessCommand(char *command_buffer) {
            print(SCREEN_LOG_LEVEL_DEBUG, "got host command: '%s'\n", command_buffer);

            const char *argument = nullptr;
            for (size_t i = 0; command_buffer[i] != 0; i++) {
                if (command_buffer[i] == ':') {
                    command_buffer[i] = 0;
                    argument = command_buffer + i + 1;
                    break;
                }
            }
            
            if (!strcmp(command_buffer, "getvar")) {
                return this->GetVar(argument);
            } else if (!strcmp(command_buffer, "download")) {
                return this->Download(argument);
            } else if (!strcmp(command_buffer, "flash")) {
                return this->Flash(argument);
            } else if (!strcmp(command_buffer, "reboot")) {
                return this->Reboot(argument);
            } else if (!strcmp(command_buffer, "boot")) {
                return this->Boot(argument);
            } else if (!strcmp(command_buffer, "oem crc32")) {
                return this->OemCRC32(argument);
            } else {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "unknown command: %s", command_buffer);
            }
        }

        Result FastbootImpl::GetVar(const char *argument) {
            if (strcmp(argument, "version") == 0) {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::OKAY, "0.4");
            } else if (strcmp(argument, "product") == 0) {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::OKAY, "Fusée Fastboot");
            } else if (strcmp(argument, "max-download-size") == 0) {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::OKAY, "%08X", this->gadget.GetMaxDownloadSize());
            } else {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "unknown variable");
            }
        }

        Result FastbootImpl::Download(const char *argument) {
            bool parsed_ok = true;
            uint32_t download_size = 0;
            for (int i = 0; i < 8; i++) {
                download_size<<= 4;
                char ch = argument[i];
                if (ch >= '0' && ch <= '9') {
                    download_size|= ch - '0';
                } else if (ch >= 'a' && ch <= 'f') {
                    download_size|= (ch - 'a') + 0xa;
                } else if (ch >= 'A' && ch <= 'F') {
                    download_size|= (ch - 'A') + 0xa;
                } else {
                    parsed_ok = false;
                    break;
                }
            }

            if (!parsed_ok) {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "failed to parse download size");
            }

            R_TRY_CATCH(gadget.PrepareDownload(download_size)) {
                R_CATCH(xusb::ResultDownloadTooLarge) {
                    return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "download size too large");
                }
            } R_END_TRY_CATCH;
                
            return this->gadget.SendResponse(ResponseDisposition::Download, ResponseToken::DATA, "%08X", download_size);
        }

        Result FastbootImpl::Flash(const char *argument) {
            if (!strcmp(argument, "ams")) {
                memset(&this->flash_ams.zip, 0, sizeof(this->flash_ams.zip));
                this->flash_ams.zip.m_pAlloc = &MzHeap::Alloc;
                this->flash_ams.zip.m_pFree = &MzHeap::Free;
                this->flash_ams.zip.m_pRealloc = &MzHeap::Realloc;
            
                MzHeap::Reset();

                if (!mz_zip_reader_init_mem(&this->flash_ams.zip, this->gadget.GetLastDownloadBuffer(), this->gadget.GetLastDownloadSize(), 0)) {
                    return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "failed to initialize zip reader");
                }

                if (!mount_sd()) {
                    return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "failed to mount sd card");
                }

                this->flash_ams.num_files = mz_zip_reader_get_num_files(&this->flash_ams.zip);
                this->flash_ams.file_index = 0;
                this->flash_ams.error = nullptr;

                this->current_action = Action::FlashAms;
            
                return this->ContinueFlashAms();
            } else if (!strcmp(argument, "sd")) {
                if (this->gadget.GetLastDownloadSize() % 512 != 0) {
                    return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "image size is not a multiple of 512 bytes");
                }
                
                bool was_sd_mounted = false;

                if (!acquire_sd_device()) {
                    return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "Failed to initialize SD card. Check that it is inserted.");
                }

                temporary_unmount_sd(&was_sd_mounted);
                
                ON_SCOPE_EXIT {
                    if (was_sd_mounted) {
                        temporary_remount_sd();
                    }
                    
                    release_sd_device();
                };

                if (sdmmc_device_write(&g_sd_device, 0, this->gadget.GetLastDownloadSize() / 512, this->gadget.GetLastDownloadBuffer())) {
                    return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::OKAY, "");
                } else {
                    return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "I/O error");
                }
            } else {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "unknown partition");
            }
        }

        Result FastbootImpl::Reboot(const char *argment) {
            return this->gadget.SendResponse(ResponseDisposition::Reboot, ResponseToken::OKAY, "");
        }

        Result FastbootImpl::Boot(const char *argment) {
            if (this->gadget.GetLastDownloadSize() == 0) {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "no image has been downloaded");
            }

            /* Validate the boot image we have received. */
            if (this->gadget.GetLastDownloadSize() < sizeof(android_boot_image_v2)) {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "received boot image is too small");
            }

            android_boot_image_v2 *bootimg = (android_boot_image_v2*) this->gadget.GetLastDownloadBuffer();
            if (memcmp(bootimg->magic, "ANDROID!", sizeof(bootimg->magic)) != 0) {
                print(SCREEN_LOG_LEVEL_DEBUG, "invalid magic: '%s'\n", bootimg->magic);
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "received boot image has invalid magic");
            }

            if (bootimg->header_version != 2) {
                print(SCREEN_LOG_LEVEL_DEBUG, "invalid version: %d\n", bootimg->header_version);
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "received boot image has invalid version");
            }

            if (bootimg->header_size != sizeof(*bootimg)) {
                print(SCREEN_LOG_LEVEL_DEBUG, "invalid size: %d, expected %d\n", bootimg->header_size, sizeof(*bootimg));
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "received boot image has invalid size");
            }

            if (bootimg->ramdisk_size != 0 ||
               bootimg->second_size != 0 ||
               bootimg->dtb_size != 0) {
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "received boot image has unexpected ramdisk, second stage bootloader, or dtb");
            }
                
            /* Setup chainloader. */
        
            print(SCREEN_LOG_LEVEL_DEBUG, "loading kernel to 0x%lx\n", bootimg->kernel_addr);
        
            g_chainloader_num_entries = 1;
            g_chainloader_entries[0].load_address = bootimg->kernel_addr;
            g_chainloader_entries[0].src_address  = (uintptr_t) this->gadget.GetLastDownloadBuffer() + bootimg->page_size;
            g_chainloader_entries[0].size         = bootimg->kernel_size;
            g_chainloader_entries[0].num          = 0;
            g_chainloader_entrypoint              = bootimg->kernel_addr;

            return this->gadget.SendResponse(ResponseDisposition::Chainload, ResponseToken::OKAY, "");
        }

        Result FastbootImpl::OemCRC32(const char *argument) {
            uint32_t crc = crc32(0, (const uint8_t*) this->gadget.GetLastDownloadBuffer(), this->gadget.GetLastDownloadSize());

            return this->gadget.SendResponse(ResponseDisposition::Okay, ResponseToken::INFO, "%08lx", crc);
        }

        Result FastbootImpl::Continue() {
            switch(this->current_action) {
            default:
            case Action::Invalid:
                fatal_error("fastboot implementation has invalid current action");
            case Action::FlashAms:
                return this->ContinueFlashAms();
            }
        }

        namespace {
            size_t flash_ams_mz_extract_callback(void *opaque, mz_uint64 file_ofs, const void *buf, size_t n) {
                FIL *f = (FIL*) opaque;

                if (f_lseek(f, file_ofs) != FR_OK) {
                    return 0;
                }

                UINT bw;
                if (f_write(f, buf, n, &bw) != FR_OK) {
                    return 0;
                }

                return bw;
            }
        } // anonymous namespace
    
        Result FastbootImpl::ContinueFlashAms() {
            if (this->flash_ams.error != nullptr) {
                unmount_sd();
                
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, this->flash_ams.error);
            }
            
            if (this->flash_ams.file_index == this->flash_ams.num_files) {
                unmount_sd();
                
                return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::OKAY, "");
            }

            mz_uint file_index = this->flash_ams.file_index++;
            static char filename_buffer[261];
            mz_zip_reader_get_filename(&this->flash_ams.zip, file_index, filename_buffer, sizeof(filename_buffer));

            R_TRY(this->gadget.SendResponse(ResponseDisposition::Continue, ResponseToken::INFO, "(%d/%d): %s", file_index + 1, this->flash_ams.num_files, filename_buffer));

            if (mz_zip_reader_is_file_a_directory(&this->flash_ams.zip, file_index)) {
                size_t len = strlen(filename_buffer);
                if (filename_buffer[len-1] == '/') {
                    /* FatFS needs us to trim trailing slash on directories. */
                    filename_buffer[len-1] = '\0';
                }
                        
                FRESULT r = f_mkdir(filename_buffer);
                if (r != FR_OK && r != FR_EXIST) {
                    this->flash_ams.error = "failed to create directory";
                    return ResultSuccess();
                }
            } else {
                FIL f;
                if (f_open(&f, filename_buffer, FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) {
                    this->flash_ams.error = "failed to open file";
                    return ResultSuccess();
                }

                f_sync(&f);

                if (!mz_zip_reader_extract_to_callback(&this->flash_ams.zip, file_index, flash_ams_mz_extract_callback, &f, 0)) {
                    this->flash_ams.error = "failed to extract file";
                    f_close(&f);
                    return ResultSuccess();
                }
                        
                if (f_close(&f) != FR_OK) {
                    this->flash_ams.error = "failed to close file";
                    return ResultSuccess();
                }
            }

            return ResultSuccess();
        }
    
    } // namespace fastboot

} // namespace ams
