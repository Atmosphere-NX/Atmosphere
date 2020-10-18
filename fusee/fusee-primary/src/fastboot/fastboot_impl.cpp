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

extern "C" {
#include "../lib/log.h"
#include "../utils.h"
#include "../chainloader.h"
#include "../lib/fatfs/ff.h"
}

#include<array>
#include<stdio.h>

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
            return this->gadget.SendResponse(ResponseDisposition::ReadHostCommand, ResponseToken::FAIL, "unknown partition");
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
            }
        }
    
    } // namespace fastboot

} // namespace ams
