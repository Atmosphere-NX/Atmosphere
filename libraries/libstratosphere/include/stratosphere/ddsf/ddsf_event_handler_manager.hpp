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
#include <stratosphere/os.hpp>
#include <stratosphere/ddsf/ddsf_i_event_handler.hpp>

namespace ams::ddsf {

    class EventHandlerManager;

    class EventHandlerManager {
        NON_COPYABLE(EventHandlerManager);
        NON_MOVEABLE(EventHandlerManager);
        private:
            struct LoopControlCommandParameters;
        private:
            bool m_is_initialized;
            bool m_is_looping;
            os::SdkConditionVariable m_is_looping_cv;
            os::MultiWaitType m_multi_wait;
            os::ThreadType *m_loop_thread;
            os::Event m_loop_control_event;
            os::MultiWaitHolderType m_loop_control_event_holder;
            LoopControlCommandParameters *m_loop_control_command_params;
            os::LightEvent m_loop_control_command_done_event;
            os::SdkMutex m_loop_control_lock;
        private:
            void ProcessControlCommand(LoopControlCommandParameters *params);
            void ProcessControlCommandImpl(LoopControlCommandParameters *params);
        public:
            EventHandlerManager()
                : m_is_initialized(false), m_is_looping(false), m_is_looping_cv(), m_multi_wait(),
                  m_loop_thread(), m_loop_control_event(os::EventClearMode_AutoClear), m_loop_control_event_holder(),
                  m_loop_control_command_params(), m_loop_control_command_done_event(os::EventClearMode_AutoClear),
                  m_loop_control_lock()
            {
                this->Initialize();
            }

            ~EventHandlerManager() {
                if (m_is_looping) {
                    AMS_ASSERT(!this->IsRunningOnLoopThread());
                    this->RequestStop();
                }
                if (m_is_initialized) {
                    this->Finalize();
                }
            }

            bool IsRunningOnLoopThread() const { return m_loop_thread == os::GetCurrentThread(); }
            bool IsLooping() const { return m_is_looping; }

            void Initialize();
            void Finalize();

            void RegisterHandler(IEventHandler *handler);
            void UnregisterHandler(IEventHandler *handler);

            void WaitLoopEnter();
            void WaitLoopExit();
            void RequestStop();

            void LoopAuto();
    };

}
