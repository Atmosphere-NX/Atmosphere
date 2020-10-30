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
#pragma once
#include <vapours.hpp>
#include "sdmmc_i_host_controller.hpp"
#include "sdmmc_i_device_accessor.hpp"

namespace ams::sdmmc::impl {

    #if defined(AMS_SDMMC_USE_LOGGER)
    class Logger {
        private:
            static constexpr size_t LogLengthMax = 0x20;
            static constexpr size_t LogCountMax  = 0x10;
        private:
            char logs[LogCountMax][LogLengthMax];
            int log_index;
        private:
            void Clear() {
                for (size_t i = 0; i < LogCountMax; ++i) {
                    this->logs[i][0] = '\0';
                }
                this->log_index = 0;
            }

            size_t Pop(char *dst, size_t dst_size) {
                /* Decrease log index. */
                if ((--this->log_index) < 0) {
                    this->log_index = LogCountMax - 1;
                }

                /* Check if we have a log. */
                if (this->logs[this->log_index][0] == '\0') {
                    return 0;
                }

                /* Copy log to output. */
                const int len = ::ams::util::Strlcpy(dst, this->logs[this->log_index], dst_size);

                /* Clear the log we copied. */
                this->logs[this->log_index][0] = '\0';

                return static_cast<size_t>(len);
            }

        public:
            Logger() {
                this->Clear();
            }

            void Push(const char *fmt, std::va_list vl) {
                /* Format the log into the current buffer. */
                ::ams::util::TVSNPrintf(this->logs[this->log_index], LogLengthMax, fmt, vl);

                /* Update our log index. */
                if ((++this->log_index) >= static_cast<int>(LogCountMax)) {
                    this->log_index = 0;
                }
            }

            void Push(const char *fmt, ...) {
                std::va_list vl;
                va_start(vl, fmt);
                this->Push(fmt, vl);
                va_end(vl);
            }

            bool HasLog() const {
                const int index = this->log_index > 0 ? this->log_index - 1 : static_cast<int>(LogCountMax - 1);
                return this->logs[index][0] != '\0';
            }

            size_t GetAndClearLogs(char *dst, size_t dst_size) {
                AMS_ABORT_UNLESS(dst != nullptr);
                AMS_ABORT_UNLESS(dst_size > 0);

                /* Pop logs until we run out of them. */
                size_t total_len = 0;
                while (true) {
                    /* Pop the current log. */
                    const size_t cur_len = this->Pop(dst + total_len, dst_size - total_len);
                    if (cur_len == 0) {
                        break;
                    }

                    /* Check if the log exceeded the buffer size. */
                    if (total_len + cur_len + 1 >= dst_size) {
                        break;
                    }

                    /* Advance the total length. */
                    total_len += cur_len;

                    /* Check if there's space for our separator. */
                    if (total_len + 3 >= dst_size) {
                        break;
                    }

                    dst[total_len + 0] = ',';
                    dst[total_len + 1] = ' ';
                    total_len += 2;
                }

                /* Ensure that the complete log fits in the buffer. */
                if (total_len >= dst_size) {
                    total_len = dst_size - 1;
                }

                /* Ensure null termination. */
                dst[total_len] = '\0';

                /* Clear any remaining logs. */
                this->Clear();

                /* Return the length of the logs, including null terminator. */
                return total_len + 1;
            }
    };
    #endif

    enum DeviceType {
        DeviceType_Mmc    = 0,
        DeviceType_SdCard = 1,
        DeviceType_GcAsic = 2,
    };

    enum DeviceState {
        DeviceState_Idle       =  0,
        DeviceState_Ready      =  1,
        DeviceState_Ident      =  2,
        DeviceState_Stby       =  3,
        DeviceState_Tran       =  4,
        DeviceState_Data       =  5,
        DeviceState_Rcv        =  6,
        DeviceState_Prg        =  7,
        DeviceState_Dis        =  8,
        DeviceState_Rsvd0      =  9,
        DeviceState_Rsvd1      = 10,
        DeviceState_Rsvd2      = 11,
        DeviceState_Rsvd3      = 12,
        DeviceState_Rsvd4      = 13,
        DeviceState_Rsvd5      = 14,
        DeviceState_RsvdIoMode = 15,
        DeviceState_Unknown    = 16,
    };

    enum DeviceStatus : u32 {
        DeviceStatus_AkeSeqError       = (1u <<  3),
        DeviceStatus_AppCmd            = (1u <<  5),
        DeviceStatus_SwitchError       = (1u <<  7),
        DeviceStatus_EraseReset        = (1u << 13),
        DeviceStatus_WpEraseSkip       = (1u << 15),
        DeviceStatus_CidCsdOverwrite   = (1u << 16),
        DeviceStatus_Error             = (1u << 19),
        DeviceStatus_CcError           = (1u << 20),
        DeviceStatus_DeviceEccFailed   = (1u << 21),
        DeviceStatus_IllegalCommand    = (1u << 22),
        DeviceStatus_ComCrcError       = (1u << 23),
        DeviceStatus_LockUnlockFailed  = (1u << 24),
        DeviceStatus_WpViolation       = (1u << 26),
        DeviceStatus_EraseParam        = (1u << 27),
        DeviceStatus_EraseSeqError     = (1u << 28),
        DeviceStatus_BlockLenError     = (1u << 29),
        DeviceStatus_AddressMisaligned = (1u << 30),
        DeviceStatus_AddressOutOfRange = (1u << 31),

        DeviceStatus_CurrentStateShift = 9,
        DeviceStatus_CurrentStateMask  = (0b1111u << DeviceStatus_CurrentStateShift),

        DeviceStatus_ErrorMask = (DeviceStatus_SwitchError       |
                                  DeviceStatus_EraseReset        |
                                  DeviceStatus_WpEraseSkip       |
                                  DeviceStatus_CidCsdOverwrite   |
                                  DeviceStatus_Error             |
                                  DeviceStatus_CcError           |
                                  DeviceStatus_DeviceEccFailed   |
                                  DeviceStatus_IllegalCommand    |
                                  DeviceStatus_ComCrcError       |
                                  DeviceStatus_LockUnlockFailed  |
                                  DeviceStatus_WpViolation       |
                                  DeviceStatus_EraseParam        |
                                  DeviceStatus_EraseSeqError     |
                                  DeviceStatus_BlockLenError     |
                                  DeviceStatus_AddressMisaligned |
                                  DeviceStatus_AddressOutOfRange),
    };

    class BaseDevice {
        private:
            u32 ocr;
            u8 cid[DeviceCidSize];
            u16 csd[DeviceCsdSize / sizeof(u16)];
            u32 memory_capacity;
            bool is_high_capacity;
            bool is_valid_ocr;
            bool is_valid_cid;
            bool is_valid_csd;
            bool is_valid_high_capacity;
            bool is_valid_memory_capacity;
            bool is_active;
            bool is_awake;
        public:
        #if defined(AMS_SDMMC_THREAD_SAFE)
            mutable os::Mutex device_mutex;
        public:
            BaseDevice() : device_mutex(true)
        #else
            BaseDevice()
        #endif
            {
                this->is_awake         = true;
                this->ocr              = 0;
                this->memory_capacity  = 0;
                this->is_high_capacity = false;
                this->OnDeactivate();
            }

            void OnDeactivate() {
                this->is_active                = false;
                this->is_valid_ocr             = false;
                this->is_valid_cid             = false;
                this->is_valid_csd             = false;
                this->is_valid_high_capacity   = false;
                this->is_valid_memory_capacity = false;
            }

            bool IsAwake() const {
                return this->is_awake;
            }

            void Awaken() {
                this->is_awake = true;
            }

            void PutToSleep() {
                this->is_awake = false;
            }

            bool IsActive() const {
                return this->is_active;
            }

            void SetActive() {
                this->is_active = true;
            }

            virtual void Deactivate() {
                this->OnDeactivate();
            }

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                virtual os::EventType *GetRemovedEvent() const = 0;
            #endif

            virtual DeviceType GetDeviceType() const = 0;
            virtual u16 GetRca() const = 0;

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                void InitializeRemovedEvent() {
                    if (os::EventType *removed_event = this->GetRemovedEvent(); removed_event != nullptr) {
                        os::InitializeEvent(removed_event, false, os::EventClearMode_ManualClear);
                    }
                }

                void FinalizeRemovedEvent() {
                    if (os::EventType *removed_event = this->GetRemovedEvent(); removed_event != nullptr) {
                        os::FinalizeEvent(removed_event);
                    }
                }

                void SignalRemovedEvent() {
                    if (os::EventType *removed_event = this->GetRemovedEvent(); removed_event != nullptr) {
                        os::SignalEvent(removed_event);
                    }
                }

                void ClearRemovedEvent() {
                    if (os::EventType *removed_event = this->GetRemovedEvent(); removed_event != nullptr) {
                        os::ClearEvent(removed_event);
                    }
                }

                bool IsRemoved() const {
                    if (os::EventType *removed_event = this->GetRemovedEvent(); removed_event != nullptr) {
                        return os::TryWaitEvent(removed_event);
                    }
                    return false;
                }
            #endif

            Result CheckRemoved() const {
                #if defined(AMS_SDMMC_USE_OS_EVENTS)
                R_UNLESS(!this->IsRemoved(), sdmmc::ResultDeviceRemoved());
                #endif

                return ResultSuccess();
            }

            Result CheckAccessible() const {
                /* Check awake. */
                R_UNLESS(this->IsAwake(), sdmmc::ResultNotAwakened());

                /* Check active. */
                R_UNLESS(this->IsActive(), sdmmc::ResultNotActivated());

                /* Check removed. */
                R_TRY(this->CheckRemoved());

                return ResultSuccess();
            }

            void SetHighCapacity(bool en) {
                this->is_high_capacity       = en;
                this->is_valid_high_capacity = true;
            }

            bool IsHighCapacity() const {
                AMS_ABORT_UNLESS(this->is_valid_high_capacity);
                return this->is_high_capacity;
            }

            void SetOcr(u32 o) {
                this->ocr          = o;
                this->is_valid_ocr = true;
            }

            u32 GetOcr() const {
                AMS_ABORT_UNLESS(this->is_valid_ocr);
                return this->ocr;
            }

            void SetCid(const void *src, size_t src_size) {
                AMS_ABORT_UNLESS(src != nullptr);
                AMS_ABORT_UNLESS(src_size >= DeviceCidSize);
                std::memcpy(this->cid, src, DeviceCidSize);
                this->is_valid_cid = true;
            }

            void GetCid(void *dst, size_t dst_size) const {
                AMS_ABORT_UNLESS(this->is_valid_cid);
                AMS_ABORT_UNLESS(dst != nullptr);
                AMS_ABORT_UNLESS(dst_size >= DeviceCidSize);
                std::memcpy(dst, this->cid, DeviceCidSize);
            }

            void SetCsd(const void *src, size_t src_size) {
                AMS_ABORT_UNLESS(src != nullptr);
                AMS_ABORT_UNLESS(src_size >= DeviceCsdSize);
                std::memcpy(this->csd, src, DeviceCsdSize);
                this->is_valid_csd = true;
            }

            void GetCsd(void *dst, size_t dst_size) const {
                AMS_ABORT_UNLESS(this->is_valid_csd);
                AMS_ABORT_UNLESS(dst != nullptr);
                AMS_ABORT_UNLESS(dst_size >= DeviceCsdSize);
                std::memcpy(dst, this->csd, DeviceCsdSize);
            }

            void SetMemoryCapacity(u32 num_sectors) {
                this->memory_capacity          = num_sectors;
                this->is_valid_memory_capacity = true;
            }

            u32 GetMemoryCapacity() const {
                AMS_ABORT_UNLESS(this->is_valid_memory_capacity);
                return this->memory_capacity;
            }

            void GetLegacyCapacityParameters(u8 *out_c_size_mult, u8 *out_read_bl_len) const;
            Result SetLegacyMemoryCapacity();

            Result CheckDeviceStatus(u32 r1_resp) const;
            DeviceState GetDeviceState(u32 r1_resp) const;
    };

    class BaseDeviceAccessor : public IDeviceAccessor {
        private:
            IHostController *host_controller;
            BaseDevice *base_device;
            u32 num_activation_failures;
            u32 num_activation_error_corrections;
            u32 num_read_write_failures;
            u32 num_read_write_error_corrections;
            #if defined(AMS_SDMMC_USE_LOGGER)
            Logger error_logger;
            #endif
        private:
            void ClearErrorInfo() {
                this->num_activation_failures           = 0;
                this->num_activation_error_corrections  = 0;
                this->num_read_write_failures           = 0;
                this->num_read_write_error_corrections  = 0;
            }
        protected:
            explicit BaseDeviceAccessor(IHostController *hc) : host_controller(hc), base_device(nullptr) {
                this->ClearErrorInfo();
            }

            IHostController *GetHostController() const {
                return this->host_controller;
            }

            void SetDevice(BaseDevice *bd) {
                this->base_device = bd;
            }

            Result CheckRemoved() const {
                return this->base_device->CheckRemoved();
            }

            Result IssueCommandAndCheckR1(u32 *out_response, u32 command_index, u32 command_arg, bool is_busy, DeviceState expected_state, u32 status_ignore_mask) const;

            Result IssueCommandAndCheckR1(u32 command_index, u32 command_arg, bool is_busy, DeviceState expected_state, u32 status_ignore_mask) const {
                u32 dummy;
                return this->IssueCommandAndCheckR1(std::addressof(dummy), command_index, command_arg, is_busy, expected_state, status_ignore_mask);
            }

            Result IssueCommandAndCheckR1(u32 command_index, u32 command_arg, bool is_busy, DeviceState expected_state) const {
                return this->IssueCommandAndCheckR1(command_index, command_arg, is_busy, expected_state, 0);
            }

            Result IssueCommandGoIdleState() const;
            Result IssueCommandAllSendCid(void *dst, size_t dst_size) const;
            Result IssueCommandSelectCard() const;
            Result IssueCommandSendCsd(void *dst, size_t dst_size) const;
            Result IssueCommandSendStatus(u32 *out_device_status, u32 status_ignore_mask) const;

            Result IssueCommandSendStatus(u32 status_ignore_mask) const {
                u32 dummy;
                return this->IssueCommandSendStatus(std::addressof(dummy), status_ignore_mask);
            }

            Result IssueCommandSendStatus() const {
                return this->IssueCommandSendStatus(0);
            }

            Result IssueCommandSetBlockLenToSectorSize() const;
            Result IssueCommandMultipleBlock(u32 *out_num_transferred_blocks, u32 sector_index, u32 num_sectors, void *buf, bool is_read) const;
            Result ReadWriteSingle(u32 *out_num_transferred_blocks, u32 sector_index, u32 num_sectors, void *buf, bool is_read) const;
            Result ReadWriteMultiple(u32 sector_index, u32 num_sectors, u32 sector_index_alignment, void *buf, size_t buf_size, bool is_read);

            void IncrementNumActivationErrorCorrections() {
                ++this->num_activation_error_corrections;
            }

            void PushErrorTimeStamp() {
                #if defined(AMS_SDMMC_USE_LOGGER)
                {
                    this->error_logger.Push("%u", static_cast<u32>(os::ConvertToTimeSpan(os::GetSystemTick()).GetSeconds()));
                }
                #endif
            }

            void PushErrorLog(bool with_timestamp, const char *fmt, ...) {
                #if defined(AMS_SDMMC_USE_LOGGER)
                {
                    std::va_list vl;
                    va_start(vl, fmt);
                    this->error_logger.Push(fmt, vl);
                    va_end(vl);

                    if (with_timestamp) {
                        this->PushErrorTimeStamp();
                    }
                }
                #else
                {
                    AMS_UNUSED(with_timestamp, fmt);
                }
                #endif
            }

            virtual Result OnActivate() = 0;
            virtual Result OnReadWrite(u32 sector_index, u32 num_sectors, void *buf, size_t buf_size, bool is_read) = 0;
            virtual Result ReStartup() = 0;
        public:
            #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
            virtual void RegisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) override;
            virtual void UnregisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) override;
            #endif

            virtual Result Activate() override;
            virtual void Deactivate() override;

            virtual Result ReadWrite(u32 sector_index, u32 num_sectors, void *buffer, size_t buffer_size, bool is_read) override;
            virtual Result CheckConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width) override;

            virtual Result GetMemoryCapacity(u32 *out_sectors) const override;
            virtual Result GetDeviceStatus(u32 *out) const override;
            virtual Result GetOcr(u32 *out) const override;
            virtual Result GetRca(u16 *out) const override;
            virtual Result GetCid(void *out, size_t size) const override;
            virtual Result GetCsd(void *out, size_t size) const override;

            virtual void GetAndClearErrorInfo(ErrorInfo *out_error_info, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size) override;
    };

}
