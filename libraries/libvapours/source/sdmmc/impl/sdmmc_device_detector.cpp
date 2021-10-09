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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#if defined(AMS_SDMMC_USE_SD_CARD_DETECTOR)
#include "sdmmc_device_detector.hpp"

namespace ams::sdmmc::impl {

    bool DeviceDetector::IsCurrentInserted() {
        return gpio::GetValue(std::addressof(m_gpio_pad_session)) == m_inserted_gpio_value;
    }

    void DeviceDetector::HandleDeviceStatus(bool prev_inserted, bool cur_inserted) {
        if (!prev_inserted && !cur_inserted) {
            /* Not inserted -> Not inserted, nothing to do. */
        } else if (!prev_inserted && cur_inserted) {
            /* Card was inserted. */
            if (m_callback_info.inserted_callback != nullptr) {
                m_callback_info.inserted_callback(m_callback_info.inserted_callback_arg);
            }
        } else if (prev_inserted && !cur_inserted) {
            /* Card was removed. */
            if (m_callback_info.removed_callback != nullptr) {
                m_callback_info.removed_callback(m_callback_info.removed_callback_arg);
            }
        } else /* if (prev_inserted && cur_inserted) */ {
            /* Card was removed, and then inserted. */
            if (m_callback_info.removed_callback != nullptr) {
                m_callback_info.removed_callback(m_callback_info.removed_callback_arg);
            }

            if (m_callback_info.inserted_callback != nullptr) {
                m_callback_info.inserted_callback(m_callback_info.inserted_callback_arg);
            }
        }
    }

    void DeviceDetector::DetectorThread() {
        /* Initialize the gpio session. */
        gpio::Initialize();

        /* Open and configure the pad session. */
        gpio::OpenSession(std::addressof(m_gpio_pad_session), m_gpio_device_code);
        gpio::SetDirection(std::addressof(m_gpio_pad_session), gpio::Direction_Input);
        gpio::SetDebounceTime(std::addressof(m_gpio_pad_session), m_gpio_debounce_ms);
        gpio::SetDebounceEnabled(std::addressof(m_gpio_pad_session), true);
        gpio::SetInterruptMode(std::addressof(m_gpio_pad_session), gpio::InterruptMode_AnyEdge);

        /* Get the gpio session's interrupt event. */
        os::SystemEventType gpio_event;
        R_ABORT_UNLESS(gpio::BindInterrupt(std::addressof(gpio_event), std::addressof(m_gpio_pad_session)));

        /* Initialize and link multi wait/holders. */
        os::MultiWaitType multi_wait;
        os::MultiWaitHolderType detector_thread_end_holder;
        os::MultiWaitHolderType request_sleep_wake_event_holder;
        os::MultiWaitHolderType gpio_event_holder;
        os::InitializeMultiWait(std::addressof(multi_wait));
        os::InitializeMultiWaitHolder(std::addressof(detector_thread_end_holder), std::addressof(m_detector_thread_end_event));
        os::LinkMultiWaitHolder(std::addressof(multi_wait), std::addressof(detector_thread_end_holder));
        os::InitializeMultiWaitHolder(std::addressof(request_sleep_wake_event_holder), std::addressof(m_request_sleep_wake_event));
        os::LinkMultiWaitHolder(std::addressof(multi_wait), std::addressof(request_sleep_wake_event_holder));
        os::InitializeMultiWaitHolder(std::addressof(gpio_event_holder), std::addressof(gpio_event));
        os::LinkMultiWaitHolder(std::addressof(multi_wait), std::addressof(gpio_event_holder));

        /* Wait before detecting the initial state of the card. */
        os::SleepThread(TimeSpan::FromMilliSeconds(m_gpio_debounce_ms));
        bool cur_inserted = this->IsCurrentInserted();
        m_is_prev_inserted = cur_inserted;

        /* Set state as awake. */
        m_state = State_Awake;
        os::SignalEvent(std::addressof(m_ready_device_status_event));

        /* Enable interrupts to be informed of device status. */
        gpio::SetInterruptEnable(std::addressof(m_gpio_pad_session), true);

        /* Wait, servicing our events. */
        while (true) {
            /* Get the signaled holder. */
            os::MultiWaitHolderType *signaled_holder = os::WaitAny(std::addressof(multi_wait));

            /* Process the holder. */
            bool insert_change = false;
            if (signaled_holder == std::addressof(detector_thread_end_holder)) {
                /* We should kill ourselves. */
                os::ClearEvent(std::addressof(m_detector_thread_end_event));
                m_state = State_Finalized;
                break;
            } else if (signaled_holder == std::addressof(request_sleep_wake_event_holder)) {
                /* A request for us to sleep/wake has come in, so we'll acknowledge it. */
                os::ClearEvent(std::addressof(m_request_sleep_wake_event));
                m_state = State_Sleep;
                os::SignalEvent(std::addressof(m_acknowledge_sleep_awake_event));

                /* Temporarily unlink our interrupt event. */
                os::UnlinkMultiWaitHolder(std::addressof(gpio_event_holder));

                /* Wait to be signaled. */
                signaled_holder = os::WaitAny(std::addressof(multi_wait));

                /* Link our interrupt event back in. */
                os::LinkMultiWaitHolder(std::addressof(multi_wait), std::addressof(gpio_event_holder));

                /* We're awake again. Either because we should exit, or because we were asked to wake up. */
                os::ClearEvent(std::addressof(m_request_sleep_wake_event));
                m_state = State_Awake;
                os::SignalEvent(std::addressof(m_acknowledge_sleep_awake_event));

                /* If we were asked to exit, do so. */
                if (signaled_holder == std::addressof(detector_thread_end_holder)) {
                    /* We should kill ourselves. */
                    os::ClearEvent(std::addressof(m_detector_thread_end_event));
                    m_state = State_Finalized;
                    break;
                } else /* if (signaled_holder == std::addressof(request_sleep_wake_event_holder)) */ {
                    if ((m_force_detection) ||
                        (({ bool active; R_SUCCEEDED(gpio::IsWakeEventActive(std::addressof(active), m_gpio_device_code)) && active; })) ||
                        (os::TryWaitSystemEvent(std::addressof(gpio_event))) ||
                        (m_is_prev_inserted != this->IsCurrentInserted()))
                    {
                        insert_change = true;
                    }
                }
            } else /* if (signaled_holder == std::addressof(gpio_event_holder)) */ {
                /* An event was detected. */
                insert_change = true;
            }

            /* Handle an insert change, if one occurred. */
            if (insert_change) {
                /* Call the relevant callback, if we have one. */
                if (m_device_detection_event_callback != nullptr) {
                    m_device_detection_event_callback(m_device_detection_event_callback_arg);
                }

                /* Clear the interrupt event. */
                os::ClearSystemEvent(std::addressof(gpio_event));
                gpio::ClearInterruptStatus(std::addressof(m_gpio_pad_session));
                gpio::SetInterruptEnable(std::addressof(m_gpio_pad_session), true);

                /* Update insertion status. */
                cur_inserted = this->IsCurrentInserted();
                this->HandleDeviceStatus(m_is_prev_inserted, cur_inserted);
                m_is_prev_inserted = cur_inserted;
            }
        }

        /* Disable interrupts to our gpio event. */
        gpio::SetInterruptEnable(std::addressof(m_gpio_pad_session), false);

        /* Finalize and unlink multi wait/holders. */
        os::UnlinkMultiWaitHolder(std::addressof(gpio_event_holder));
        os::FinalizeMultiWaitHolder(std::addressof(gpio_event_holder));
        os::UnlinkMultiWaitHolder(std::addressof(request_sleep_wake_event_holder));
        os::FinalizeMultiWaitHolder(std::addressof(request_sleep_wake_event_holder));
        os::UnlinkMultiWaitHolder(std::addressof(detector_thread_end_holder));
        os::FinalizeMultiWaitHolder(std::addressof(detector_thread_end_holder));
        os::FinalizeMultiWait(std::addressof(multi_wait));

        /* Finalize the gpio session. */
        gpio::UnbindInterrupt(std::addressof(m_gpio_pad_session));
        gpio::CloseSession(std::addressof(m_gpio_pad_session));
        gpio::Finalize();
    }

    void DeviceDetector::Initialize(CallbackInfo *ci) {
        /* Transition our state from finalized to initializing. */
        AMS_ABORT_UNLESS(m_state == State_Finalized);
        m_state = State_Initializing;

        /* Set our callback infos. */
        m_callback_info = *ci;

        /* Initialize our events. */
        os::InitializeEvent(std::addressof(m_ready_device_status_event), false, os::EventClearMode_ManualClear);
        os::InitializeEvent(std::addressof(m_request_sleep_wake_event), false, os::EventClearMode_ManualClear);
        os::InitializeEvent(std::addressof(m_acknowledge_sleep_awake_event), false, os::EventClearMode_ManualClear);
        os::InitializeEvent(std::addressof(m_detector_thread_end_event), false, os::EventClearMode_ManualClear);

        /* Create and start the detector thread. */
        os::CreateThread(std::addressof(m_detector_thread), DetectorThreadEntry, this, m_detector_thread_stack, sizeof(m_detector_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(sdmmc, DeviceDetector));
        os::SetThreadNamePointer(std::addressof(m_detector_thread), AMS_GET_SYSTEM_THREAD_NAME(sdmmc, DeviceDetector));
        os::StartThread(std::addressof(m_detector_thread));
    }

    void DeviceDetector::Finalize() {
        /* Ensure we're not already finalized. */
        AMS_ABORT_UNLESS(m_state != State_Finalized);

        /* Signal event to end the detector thread. */
        os::SignalEvent(std::addressof(m_detector_thread_end_event));
        os::WaitThread(std::addressof(m_detector_thread));

        /* Finalize thread and events. */
        os::DestroyThread(std::addressof(m_detector_thread));
        os::FinalizeEvent(std::addressof(m_ready_device_status_event));
        os::FinalizeEvent(std::addressof(m_request_sleep_wake_event));
        os::FinalizeEvent(std::addressof(m_acknowledge_sleep_awake_event));
        os::FinalizeEvent(std::addressof(m_detector_thread_end_event));
    }

    void DeviceDetector::PutToSleep() {
        /* Signal request, wait for acknowledgement. */
        os::SignalEvent(std::addressof(m_request_sleep_wake_event));
        os::WaitEvent(std::addressof(m_acknowledge_sleep_awake_event));
        os::ClearEvent(std::addressof(m_acknowledge_sleep_awake_event));
    }

    void DeviceDetector::Awaken(bool force_det) {
        /* Signal request, wait for acknowledgement. */
        m_force_detection = force_det;
        os::SignalEvent(std::addressof(m_request_sleep_wake_event));
        os::WaitEvent(std::addressof(m_acknowledge_sleep_awake_event));
        os::ClearEvent(std::addressof(m_acknowledge_sleep_awake_event));
    }

    bool DeviceDetector::IsInserted() {
        bool inserted = false;

        switch (m_state) {
            case State_Initializing:
                /* Wait for us to know whether the device is inserted. */
                os::WaitEvent(std::addressof(m_ready_device_status_event));
                [[fallthrough]];
            case State_Awake:
                /* Get whether the device is currently inserted. */
                inserted = this->IsCurrentInserted();
                break;
            case State_Sleep:
            case State_Finalized:
                /* Get whether the device was inserted when we last knew. */
                inserted = m_is_prev_inserted;
                break;
        }

        return inserted;
    }

    void DeviceDetector::RegisterDetectionEventCallback(DeviceDetectionEventCallback cb, void *arg) {
        m_device_detection_event_callback_arg = arg;
        m_device_detection_event_callback     = cb;
    }

    void DeviceDetector::UnregisterDetectionEventCallback() {
        m_device_detection_event_callback = nullptr;
    }

}

#endif
