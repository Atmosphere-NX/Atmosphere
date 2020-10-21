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
#include "sdmmc_sd_host_standard_registers.hpp"

namespace ams::sdmmc::impl {

    class SdHostStandardController : public IHostController {
        protected:
            #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
                struct BufferInfo {
                    uintptr_t buffer_address;
                    size_t buffer_size;
                    dd::DeviceVirtualAddress buffer_device_virtual_address;
                };

                static constexpr inline auto NumBufferInfos = 3;
            #endif
        protected:
            SdHostStandardRegisters *registers;

            #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
            BufferInfo buffer_infos[NumBufferInfos];
            #endif

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            os::WaitableManagerType waitable_manager;
            os::InterruptEventType *interrupt_event;
            os::WaitableHolderType interrupt_event_holder;
            os::EventType *removed_event;
            os::WaitableHolderType removed_event_holder;
            #endif

            u64 next_sdma_address;
            u32 check_transfer_interval_ms;

            u32 device_clock_frequency_khz;
            bool is_power_saving_enable;
            bool is_device_clock_enable;

            ResponseType last_response_type;
            u32 last_response[4];
            u32 last_stop_transmission_response;
        protected:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                void PreSetInterruptEvent(os::InterruptEventType *ie) {
                    this->interrupt_event = ie;
                }

                bool IsRemoved() const {
                    return this->removed_event != nullptr && os::TryWaitEvent(this->removed_event);
                }
            #endif

            void SetDeviceClockFrequencyKHz(u32 khz) {
                this->device_clock_frequency_khz = khz;
            }

            #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
                void ResetBufferInfos();
                dd::DeviceVirtualAddress GetDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size);
            #endif

            void EnsureControl();
            Result EnableInternalClock();
            void SetBusPower(BusPower bus_power);

            void EnableInterruptStatus();
            void DisableInterruptStatus();

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                Result WaitInterrupt(u32 timeout_ms);
                void ClearInterrupt();
            #endif

            void SetTransfer(u32 *out_num_transferred_blocks, const TransferData *xfer_data);
            void SetTransferForTuning();

            void SetCommand(const Command *command, bool has_xfer_data);
            void SetCommandForTuning(u32 command_index);

            Result ResetCmdDatLine();
            Result AbortTransaction();
            Result WaitWhileCommandInhibit(bool has_dat);
            Result CheckAndClearInterruptStatus(volatile u16 *out_normal_int_status, u16 wait_mask);
            Result WaitCommandComplete();
            Result WaitTransferComplete();
            Result WaitWhileBusy();

            void GetResponse(u32 *out_response, size_t response_size, ResponseType response_type) const;

            Result IssueCommandWithDeviceClock(const Command *command, TransferData *xfer_data, u32 *out_num_transferred_blocks);
            Result IssueStopTransmissionCommandWithDeviceClock(u32 *out_response);

            ALWAYS_INLINE Result CheckRemoved() {
                #if defined(AMS_SDMMC_USE_OS_EVENTS)
                R_UNLESS(!this->IsRemoved(), sdmmc::ResultDeviceRemoved());
                #endif

                return ResultSuccess();
            }
        public:
            SdHostStandardController(dd::PhysicalAddress registers_phys_addr, size_t registers_size);

            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                virtual void PreSetRemovedEvent(os::EventType *e) override {
                    this->removed_event = e;
                }
            #endif

            virtual void Initialize() override;
            virtual void Finalize() override;

            #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
            virtual void RegisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) override;
            virtual void UnregisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) override;
            #endif

            virtual void SetWorkBuffer(void *wb, size_t wb_size) override;

            virtual Result SwitchToSdr12() override {
                AMS_ABORT("SwitchToSdr12 not supported\n");
            }

            virtual BusPower GetBusPower() const override;

            virtual void SetBusWidth(BusWidth bus_width) override;
            virtual BusWidth GetBusWidth() const override;

            virtual u32 GetDeviceClockFrequencyKHz() const override {
                return this->device_clock_frequency_khz;
            }

            virtual void SetPowerSaving(bool en) override;
            virtual bool IsPowerSavingEnable() const override {
                return this->is_power_saving_enable;
            }

            virtual void EnableDeviceClock() override;
            virtual void DisableDeviceClock() override;
            virtual bool IsDeviceClockEnable() const override {
                return this->is_device_clock_enable;
            }

            virtual u32 GetMaxTransferNumBlocks() const override {
                return SdHostStandardRegisters::BlockCountMax;
            }

            virtual void ChangeCheckTransferInterval(u32 ms) override;
            virtual void SetDefaultCheckTransferInterval() override;

            virtual Result IssueCommand(const Command *command, TransferData *xfer_data, u32 *out_num_transferred_blocks) override;
            virtual Result IssueStopTransmissionCommand(u32 *out_response) override;

            virtual void GetLastResponse(u32 *out_response, size_t response_size, ResponseType response_type) const override;
            virtual void GetLastStopTransmissionResponse(u32 *out_response, size_t response_size) const override;

            virtual bool IsSupportedTuning() const override {
                return false;
            }

            virtual Result Tuning(SpeedMode speed_mode, u32 command_index) {
                AMS_UNUSED(speed_mode, command_index);
                AMS_ABORT("Tuning not supported\n");
            }

            virtual void SaveTuningStatusForHs400() {
                AMS_ABORT("SaveTuningStatusForHs400 not supported\n");
            }

    };

}
