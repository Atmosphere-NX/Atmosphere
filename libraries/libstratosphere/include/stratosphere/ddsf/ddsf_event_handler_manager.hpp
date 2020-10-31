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
            bool is_initialized;
            bool is_looping;
            os::SdkConditionVariable is_looping_cv;
            os::WaitableManagerType waitable_manager;
            os::ThreadType *loop_thread;
            os::Event loop_control_event;
            os::WaitableHolderType loop_control_event_holder;
            LoopControlCommandParameters *loop_control_command_params;
            os::Event loop_control_command_done_event; /* TODO: os::LightEvent, requires mesosphere for < 4.0.0 compat. */
            os::SdkMutex loop_control_lock;
        private:
            void ProcessControlCommand(LoopControlCommandParameters *params);
            void ProcessControlCommandImpl(LoopControlCommandParameters *params);
        public:
            EventHandlerManager()
                : is_initialized(false), is_looping(false), is_looping_cv(), waitable_manager(),
                  loop_thread(), loop_control_event(os::EventClearMode_AutoClear), loop_control_event_holder(),
                  loop_control_command_params(), loop_control_command_done_event(os::EventClearMode_AutoClear),
                  loop_control_lock()
            {
                this->Initialize();
            }

            ~EventHandlerManager() {
                if (this->is_looping) {
                    AMS_ASSERT(!this->IsRunningOnLoopThread());
                    this->RequestStop();
                }
                if (this->is_initialized) {
                    this->Finalize();
                }
            }

            bool IsRunningOnLoopThread() const { return this->loop_thread == os::GetCurrentThread(); }
            bool IsLooping() const { return this->is_looping; }

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
