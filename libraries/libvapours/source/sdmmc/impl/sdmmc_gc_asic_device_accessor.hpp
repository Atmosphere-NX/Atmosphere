/*
 * Copyright (c) Atmosphère-NX
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
#include "sdmmc_base_device_accessor.hpp"

namespace ams::sdmmc::impl {

    class GcAsicDevice : public BaseDevice {
        private:
            static constexpr u16 Rca = 0;
        private:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
            mutable os::EventType m_removed_event;
            #endif
        public:
            #if defined(AMS_SDMMC_USE_OS_EVENTS)
                virtual os::EventType *GetRemovedEvent() const override {
                    return std::addressof(m_removed_event);
                }
            #endif

            virtual DeviceType GetDeviceType() const override {
                return DeviceType_GcAsic;
            }

            virtual u16 GetRca() const override {
                return Rca;
            }
    };

    class GcAsicDeviceAccessor : public BaseDeviceAccessor {
        private:
            GcAsicDevice m_gc_asic_device;
            bool m_is_initialized;
        private:
            Result IssueCommandWriteOperation(const void *op_buf, size_t op_buf_size) const;
            Result IssueCommandFinishOperation() const;
            Result IssueCommandSleep();
            Result IssueCommandUpdateKey() const;
            Result StartupGcAsicDevice();
        protected:
            virtual Result OnActivate() override;
            virtual Result OnReadWrite(u32 sector_index, u32 num_sectors, void *buf, size_t buf_size, bool is_read) override;

            virtual Result ReStartup() override {
                AMS_ABORT("Can't ReStartup GcAsic\n");
            }
        public:
            virtual void Initialize() override;
            virtual void Finalize() override;
            virtual Result GetSpeedMode(SpeedMode *out_speed_mode) const override;
        public:
            explicit GcAsicDeviceAccessor(IHostController *hc) : BaseDeviceAccessor(hc), m_is_initialized(false) {
                /* ... */
            }

            void PutGcAsicToSleep();
            Result AwakenGcAsic();
            Result WriteGcAsicOperation(const void *op_buf, size_t op_buf_size);
            Result FinishGcAsicOperation();
            Result AbortGcAsicOperation();
            Result SleepGcAsic();
            Result UpdateGcAsicKey();

            void SignalGcRemovedEvent() {
                #if defined(AMS_SDMMC_USE_OS_EVENTS)
                    m_gc_asic_device.SignalRemovedEvent();
                #else
                    AMS_ABORT("SignalGcRemovedEvent called without event support\n");
                #endif
            }

            void ClearGcRemovedEvent() {
                #if defined(AMS_SDMMC_USE_OS_EVENTS)
                    m_gc_asic_device.ClearRemovedEvent();
                #else
                    AMS_ABORT("ClearGcRemovedEvent called without event support\n");
                #endif
            }
    };

}
