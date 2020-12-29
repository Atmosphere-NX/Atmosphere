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
        return gpio::GetValue(std::addressof(this->gpio_pad_session)) == this->inserted_gpio_value;
    }

    void DeviceDetector::HandleDeviceStatus(bool prev_inserted, bool cur_inserted) {
        if (!prev_inserted && !cur_inserted) {
            /* Not inserted -> Not inserted, nothing to do. */
        } else if (!prev_inserted && cur_inserted) {
            /* Card was inserted. */
            if (this->callback_info.inserted_callback != nullptr) {
                this->callback_info.inserted_callback(this->callback_info.inserted_callback_arg);
            }
        } else if (prev_inserted && !cur_inserted) {
            /* Card was removed. */
            if (this->callback_info.removed_callback != nullptr) {
                this->callback_info.removed_callback(this->callback_info.removed_callback_arg);
            }
        } else /* if (prev_inserted && cur_inserted) */ {
            /* Card was removed, and then inserted. */
            if (this->callback_info.removed_callback != nullptr) {
                this->callback_info.removed_callback(this->callback_info.removed_callback_arg);
            }

            if (this->callback_info.inserted_callback != nullptr) {
                this->callback_info.inserted_callback(this->callback_info.inserted_callback_arg);
            }
        }
    }

    void DeviceDetector::DetectorThread() {
        /* Initialize the gpio session. */
        sm::DoWithSession([] { gpio::Initialize(); });
        gpio::OpenSession(std::addressof(this->gpio_pad_session), this->gpio_device_code);
        gpio::SetDirection(std::addressof(this->gpio_pad_session), gpio::Direction_Input);
        gpio::SetDebounceTime(std::addressof(this->gpio_pad_session), this->gpio_debounce_ms);
        gpio::SetDebounceEnabled(std::addressof(this->gpio_pad_session), true);
        gpio::SetInterruptMode(std::addressof(this->gpio_pad_session), gpio::InterruptMode_AnyEdge);

        /* Get the gpio session's interrupt event. */
        os::SystemEventType gpio_event;
        R_ABORT_UNLESS(gpio::BindInterrupt(std::addressof(gpio_event), std::addressof(this->gpio_pad_session)));

        /* Initialize and link waitable holders. */
        os::WaitableManagerType wait_manager;
        os::WaitableHolderType detector_thread_end_holder;
        os::WaitableHolderType request_sleep_wake_event_holder;
        os::WaitableHolderType gpio_event_holder;
        os::InitializeWaitableManager(std::addressof(wait_manager));
        os::InitializeWaitableHolder(std::addressof(detector_thread_end_holder), std::addressof(this->detector_thread_end_event));
        os::LinkWaitableHolder(std::addressof(wait_manager), std::addressof(detector_thread_end_holder));
        os::InitializeWaitableHolder(std::addressof(request_sleep_wake_event_holder), std::addressof(this->request_sleep_wake_event));
        os::LinkWaitableHolder(std::addressof(wait_manager), std::addressof(request_sleep_wake_event_holder));
        os::InitializeWaitableHolder(std::addressof(gpio_event_holder), std::addressof(gpio_event));
        os::LinkWaitableHolder(std::addressof(wait_manager), std::addressof(gpio_event_holder));

        /* Wait before detecting the initial state of the card. */
        os::SleepThread(TimeSpan::FromMilliSeconds(this->gpio_debounce_ms));
        bool cur_inserted = this->IsCurrentInserted();
        this->is_prev_inserted = cur_inserted;

        /* Set state as awake. */
        this->state = State_Awake;
        os::SignalEvent(std::addressof(this->ready_device_status_event));

        /* Enable interrupts to be informed of device status. */
        gpio::SetInterruptEnable(std::addressof(this->gpio_pad_session), true);

        /* Wait, servicing our events. */
        while (true) {
            /* Get the signaled holder. */
            os::WaitableHolderType *signaled_holder = os::WaitAny(std::addressof(wait_manager));

            /* Process the holder. */
            bool insert_change = false;
            if (signaled_holder == std::addressof(detector_thread_end_holder)) {
                /* We should kill ourselves. */
                os::ClearEvent(std::addressof(this->detector_thread_end_event));
                this->state = State_Finalized;
                break;
            } else if (signaled_holder == std::addressof(request_sleep_wake_event_holder)) {
                /* A request for us to sleep/wake has come in, so we'll acknowledge it. */
                os::ClearEvent(std::addressof(this->request_sleep_wake_event));
                this->state = State_Sleep;
                os::SignalEvent(std::addressof(this->acknowledge_sleep_awake_event));

                /* Temporarily unlink our interrupt event. */
                os::UnlinkWaitableHolder(std::addressof(gpio_event_holder));

                /* Wait to be signaled. */
                signaled_holder = os::WaitAny(std::addressof(wait_manager));

                /* Link our interrupt event back in. */
                os::LinkWaitableHolder(std::addressof(wait_manager), std::addressof(gpio_event_holder));

                /* We're awake again. Either because we should exit, or because we were asked to wake up. */
                os::ClearEvent(std::addressof(this->request_sleep_wake_event));
                this->state = State_Awake;
                os::SignalEvent(std::addressof(this->acknowledge_sleep_awake_event));

                /* If we were asked to exit, do so. */
                if (signaled_holder == std::addressof(detector_thread_end_holder)) {
                    /* We should kill ourselves. */
                    os::ClearEvent(std::addressof(this->detector_thread_end_event));
                    this->state = State_Finalized;
                    break;
                } else /* if (signaled_holder == std::addressof(request_sleep_wake_event_holder)) */ {
                    if ((this->force_detection) ||
                        (({ bool active; R_SUCCEEDED(gpio::IsWakeEventActive(std::addressof(active), this->gpio_device_code)) && active; })) ||
                        (os::TryWaitSystemEvent(std::addressof(gpio_event))) ||
                        (this->is_prev_inserted != this->IsCurrentInserted()))
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
                if (this->device_detection_event_callback != nullptr) {
                    this->device_detection_event_callback(this->device_detection_event_callback_arg);
                }

                /* Clear the interrupt event. */
                os::ClearSystemEvent(std::addressof(gpio_event));
                gpio::ClearInterruptStatus(std::addressof(this->gpio_pad_session));
                gpio::SetInterruptEnable(std::addressof(this->gpio_pad_session), true);

                /* Update insertion status. */
                cur_inserted = this->IsCurrentInserted();
                this->HandleDeviceStatus(this->is_prev_inserted, cur_inserted);
                this->is_prev_inserted = cur_inserted;
            }
        }

        /* Disable interrupts to our gpio event. */
        gpio::SetInterruptEnable(std::addressof(this->gpio_pad_session), false);

        /* Finalize and unlink waitable holders. */
        os::UnlinkWaitableHolder(std::addressof(gpio_event_holder));
        os::FinalizeWaitableHolder(std::addressof(gpio_event_holder));
        os::UnlinkWaitableHolder(std::addressof(request_sleep_wake_event_holder));
        os::FinalizeWaitableHolder(std::addressof(request_sleep_wake_event_holder));
        os::UnlinkWaitableHolder(std::addressof(detector_thread_end_holder));
        os::FinalizeWaitableHolder(std::addressof(detector_thread_end_holder));
        os::FinalizeWaitableManager(std::addressof(wait_manager));

        /* Finalize the gpio session. */
        gpio::UnbindInterrupt(std::addressof(this->gpio_pad_session));
        gpio::CloseSession(std::addressof(this->gpio_pad_session));
        gpio::Finalize();
    }

    void DeviceDetector::Initialize(CallbackInfo *ci) {
        /* Transition our state from finalized to initializing. */
        AMS_ABORT_UNLESS(this->state == State_Finalized);
        this->state = State_Initializing;

        /* Set our callback infos. */
        this->callback_info = *ci;

        /* Initialize our events. */
        os::InitializeEvent(std::addressof(this->ready_device_status_event), false, os::EventClearMode_ManualClear);
        os::InitializeEvent(std::addressof(this->request_sleep_wake_event), false, os::EventClearMode_ManualClear);
        os::InitializeEvent(std::addressof(this->acknowledge_sleep_awake_event), false, os::EventClearMode_ManualClear);
        os::InitializeEvent(std::addressof(this->detector_thread_end_event), false, os::EventClearMode_ManualClear);

        /* Create and start the detector thread. */
        os::CreateThread(std::addressof(this->detector_thread), DetectorThreadEntry, this, this->detector_thread_stack, sizeof(this->detector_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(sdmmc, DeviceDetector));
        os::SetThreadNamePointer(std::addressof(this->detector_thread), AMS_GET_SYSTEM_THREAD_NAME(sdmmc, DeviceDetector));
        os::StartThread(std::addressof(this->detector_thread));
    }

    void DeviceDetector::Finalize() {
        /* Ensure we're not already finalized. */
        AMS_ABORT_UNLESS(this->state != State_Finalized);

        /* Signal event to end the detector thread. */
        os::SignalEvent(std::addressof(this->detector_thread_end_event));
        os::WaitThread(std::addressof(this->detector_thread));

        /* Finalize thread and events. */
        os::DestroyThread(std::addressof(this->detector_thread));
        os::FinalizeEvent(std::addressof(this->ready_device_status_event));
        os::FinalizeEvent(std::addressof(this->request_sleep_wake_event));
        os::FinalizeEvent(std::addressof(this->acknowledge_sleep_awake_event));
        os::FinalizeEvent(std::addressof(this->detector_thread_end_event));
    }

    void DeviceDetector::PutToSleep() {
        /* Signal request, wait for acknowledgement. */
        os::SignalEvent(std::addressof(this->request_sleep_wake_event));
        os::WaitEvent(std::addressof(this->acknowledge_sleep_awake_event));
        os::ClearEvent(std::addressof(this->acknowledge_sleep_awake_event));
    }

    void DeviceDetector::Awaken(bool force_det) {
        /* Signal request, wait for acknowledgement. */
        this->force_detection = force_det;
        os::SignalEvent(std::addressof(this->request_sleep_wake_event));
        os::WaitEvent(std::addressof(this->acknowledge_sleep_awake_event));
        os::ClearEvent(std::addressof(this->acknowledge_sleep_awake_event));
    }

    bool DeviceDetector::IsInserted() {
        bool inserted = false;

        switch (this->state) {
            case State_Initializing:
                /* Wait for us to know whether the device is inserted. */
                os::WaitEvent(std::addressof(this->ready_device_status_event));
                [[fallthrough]];
            case State_Awake:
                /* Get whether the device is currently inserted. */
                inserted = this->IsCurrentInserted();
                break;
            case State_Sleep:
            case State_Finalized:
                /* Get whether the device was inserted when we last knew. */
                inserted = this->is_prev_inserted;
                break;
        }

        return inserted;
    }

    void DeviceDetector::RegisterDetectionEventCallback(DeviceDetectionEventCallback cb, void *arg) {
        this->device_detection_event_callback_arg = arg;
        this->device_detection_event_callback     = cb;
    }

    void DeviceDetector::UnregisterDetectionEventCallback() {
        this->device_detection_event_callback = nullptr;
    }

}

#endif
