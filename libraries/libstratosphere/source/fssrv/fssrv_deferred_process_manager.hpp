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
#include <stratosphere.hpp>

namespace ams::fssrv {

    using NotifyProcessDeferredFunction = void (*)(u64 process_id);

    struct DeferredProcessEntryForDeviceError : public util::IntrusiveListBaseNode<DeferredProcessEntryForDeviceError>, public fs::impl::Newable {
        os::MultiWaitHolderType *process_holder;
        u64 process_id;
    };

    class DeferredProcessQueueForDeviceError {
        NON_COPYABLE(DeferredProcessQueueForDeviceError);
        NON_MOVEABLE(DeferredProcessQueueForDeviceError);
        private:
            using EntryList = util::IntrusiveListBaseTraits<DeferredProcessEntryForDeviceError>::ListType;
        private:
            EntryList m_list{};
            os::SdkMutex m_mutex{};
        public:
            constexpr DeferredProcessQueueForDeviceError() = default;
    };

    class DeferredProcessEntryForPriority : public util::IntrusiveListBaseNode<DeferredProcessEntryForPriority>, public fs::impl::Newable {
        private:
            os::MultiWaitHolderType *m_process_holder;
            FileSystemProxyServerSessionType m_session_type;
    };

    class DeferredProcessQueueForPriority {
        NON_COPYABLE(DeferredProcessQueueForPriority);
        NON_MOVEABLE(DeferredProcessQueueForPriority);
        private:
            using EntryList = util::IntrusiveListBaseTraits<DeferredProcessEntryForPriority>::ListType;
        private:
            EntryList m_list{};
            os::SdkRecursiveMutex m_mutex{};
            os::SdkConditionVariable m_cv{};
        public:
            constexpr DeferredProcessQueueForPriority() = default;
    };

    template<typename ServerManager, NotifyProcessDeferredFunction NotifyProcessDeferred>
    class DeferredProcessManager {
        NON_COPYABLE(DeferredProcessManager);
        NON_MOVEABLE(DeferredProcessManager);
        private:
            DeferredProcessQueueForDeviceError m_queue_for_device_error{};
            os::SdkMutex m_invoke_mutex_for_device_error{};
            DeferredProcessQueueForPriority m_queue_for_priority{};
            std::atomic_bool m_is_invoke_deferred_process_event_linked{};
            os::EventType m_invoke_event{};
            os::MultiWaitHolderType m_invoke_event_holder{};
            bool m_initialized{false};
        public:
            constexpr DeferredProcessManager() = default;

            void Initialize() {
                /* Check pre-conditions. */
                AMS_ASSERT(!m_initialized);
                os::InitializeEvent(std::addressof(m_invoke_event), false, os::EventClearMode_ManualClear);
                os::InitializeMultiWaitHolder(std::addressof(m_invoke_event_holder), std::addressof(m_invoke_event));
                m_initialized = true;
            }
    };

}
