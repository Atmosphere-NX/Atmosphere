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
#if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
#include <stratosphere.hpp>

namespace ams::sdmmc::impl {

    using InsertedCallback = void (*)(void *);
    using RemovedCallback  = void (*)(void *);

    struct CallbackInfo {
        InsertedCallback inserted_callback;
        void *inserted_callback_arg;
        RemovedCallback removed_callback;
        void *removed_callback_arg;
    };

    class DeviceDetector {
        private:
            enum State {
                State_Initializing = 0,
                State_Awake        = 1,
                State_Sleep        = 2,
                State_Finalized    = 3,
            };
        private:
            alignas(os::ThreadStackAlignment) u8 detector_thread_stack[8_KB];
            State state;
            bool is_prev_inserted;
            bool force_detection;
            os::ThreadType detector_thread;
            os::EventType detector_thread_end_event;
            os::EventType request_sleep_wake_event;
            os::EventType acknowledge_sleep_awake_event;
            os::EventType ready_device_status_event;

            DeviceCode gpio_device_code;
            gpio::GpioValue inserted_gpio_value;
            u32 gpio_debounce_ms;
            gpio::GpioPadSession gpio_pad_session;

            CallbackInfo callback_info;

            DeviceDetectionEventCallback device_detection_event_callback;
            void *device_detection_event_callback_arg;
        private:
            static void DetectorThreadEntry(void *arg) {
                reinterpret_cast<DeviceDetector *>(arg)->DetectorThread();
            }

            void DetectorThread();
            bool IsCurrentInserted();
            void HandleDeviceStatus(bool prev_inserted, bool cur_inserted);
        public:
            explicit DeviceDetector(DeviceCode dc, gpio::GpioValue igv, u32 gd)
                : gpio_device_code(dc), inserted_gpio_value(igv), gpio_debounce_ms(gd)
            {
                this->state                               = State_Finalized;
                this->is_prev_inserted                    = false;
                this->force_detection                     = false;
                this->callback_info                       = {};
                this->device_detection_event_callback     = nullptr;
                this->device_detection_event_callback_arg = nullptr;
            }

            void Initialize(CallbackInfo *ci);
            void Finalize();

            void PutToSleep();
            void Awaken(bool force_det);

            u32 GetDebounceMilliSeconds() const {
                return this->gpio_debounce_ms;
            }

            bool IsInserted();

            void RegisterDetectionEventCallback(DeviceDetectionEventCallback cb, void *arg);
            void UnregisterDetectionEventCallback();
    };

}

#endif