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
            alignas(os::ThreadStackAlignment) u8 m_detector_thread_stack[8_KB];
            State m_state;
            bool m_is_prev_inserted;
            bool m_force_detection;
            os::ThreadType m_detector_thread;
            os::EventType m_detector_thread_end_event;
            os::EventType m_request_sleep_wake_event;
            os::EventType m_acknowledge_sleep_awake_event;
            os::EventType m_ready_device_status_event;

            DeviceCode m_gpio_device_code;
            gpio::GpioValue m_inserted_gpio_value;
            u32 m_gpio_debounce_ms;
            gpio::GpioPadSession m_gpio_pad_session;

            CallbackInfo m_callback_info;

            DeviceDetectionEventCallback m_device_detection_event_callback;
            void *m_device_detection_event_callback_arg;
        private:
            static void DetectorThreadEntry(void *arg) {
                reinterpret_cast<DeviceDetector *>(arg)->DetectorThread();
            }

            void DetectorThread();
            bool IsCurrentInserted();
            void HandleDeviceStatus(bool prev_inserted, bool cur_inserted);
        public:
            explicit DeviceDetector(DeviceCode dc, gpio::GpioValue igv, u32 gd)
                : m_gpio_device_code(dc), m_inserted_gpio_value(igv), m_gpio_debounce_ms(gd)
            {
                m_state                               = State_Finalized;
                m_is_prev_inserted                    = false;
                m_force_detection                     = false;
                m_callback_info                       = {};
                m_device_detection_event_callback     = nullptr;
                m_device_detection_event_callback_arg = nullptr;
            }

            void Initialize(CallbackInfo *ci);
            void Finalize();

            void PutToSleep();
            void Awaken(bool force_det);

            u32 GetDebounceMilliSeconds() const {
                return m_gpio_debounce_ms;
            }

            bool IsInserted();

            void RegisterDetectionEventCallback(DeviceDetectionEventCallback cb, void *arg);
            void UnregisterDetectionEventCallback();
    };

}

#endif