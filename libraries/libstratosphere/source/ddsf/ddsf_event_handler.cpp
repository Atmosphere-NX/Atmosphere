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
#include <stratosphere.hpp>

namespace ams::ddsf {

    namespace {

        enum class LoopControlCommand {
            None       = 0,
            Register   = 1,
            Unregister = 2,
            Terminate  = 3,
        };

    }

    struct EventHandlerManager::LoopControlCommandParameters {
        LoopControlCommand command;
        IEventHandler *target;

        LoopControlCommandParameters() : command(LoopControlCommand::None), target(nullptr) { /* ... */ }
        LoopControlCommandParameters(LoopControlCommand c, IEventHandler *t) : command(c), target(t) { /* ... */ }
    };

    void EventHandlerManager::Initialize() {
        /* Check that we're not already initialized. */
        if (m_is_initialized) {
            return;
        }

        /* Initialize multi wait/holder. */
        os::InitializeMultiWait(std::addressof(m_multi_wait));
        os::InitializeMultiWaitHolder(std::addressof(m_loop_control_event_holder), m_loop_control_event.GetBase());
        os::LinkMultiWaitHolder(std::addressof(m_multi_wait), std::addressof(m_loop_control_event_holder));

        m_is_initialized = true;
    }

    void EventHandlerManager::Finalize() {
        /* Check that we're initialized and not looping. */
        AMS_ASSERT(!m_is_looping);
        AMS_ASSERT(m_is_initialized);
        if (!m_is_initialized) {
            return;
        }

        /* Finalize multi wait/holder. */
        os::UnlinkMultiWaitHolder(std::addressof(m_loop_control_event_holder));
        os::FinalizeMultiWaitHolder(std::addressof(m_loop_control_event_holder));
        os::FinalizeMultiWait(std::addressof(m_multi_wait));

        m_is_initialized = false;
    }

    void EventHandlerManager::ProcessControlCommand(LoopControlCommandParameters *params) {
        /* Check pre-conditions. */
        AMS_ASSERT(m_is_initialized);
        AMS_ASSERT(params != nullptr);

        /* Acquire exclusive access. */
        std::scoped_lock lk(m_loop_control_lock);

        /* If we're processing for the loop thread, we can directly handle. */
        if (!m_is_looping || this->IsRunningOnLoopThread()) {
            this->ProcessControlCommandImpl(params);
        } else {
            /* Otherwise, signal to the loop thread. */
            m_loop_control_command_params = params;
            m_loop_control_event.Signal();
            m_loop_control_command_done_event.Wait();
        }
    }

    void EventHandlerManager::ProcessControlCommandImpl(LoopControlCommandParameters *params) {
        /* Check pre-conditions. */
        AMS_ASSERT(m_loop_control_lock.IsLockedByCurrentThread() || !m_loop_control_lock.TryLock());
        AMS_ASSERT(params != nullptr);
        AMS_ASSERT(params->target != nullptr);

        /* Process the command. */
        switch (params->command) {
            case LoopControlCommand::Register:
                params->target->Link(std::addressof(m_multi_wait));
                break;
            case LoopControlCommand::Unregister:
                params->target->Unlink();
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void EventHandlerManager::RegisterHandler(IEventHandler *handler) {
        /* Check that the handler is valid. */
        AMS_ASSERT(handler != nullptr);
        AMS_ASSERT(handler->IsInitialized());

        /* Send registration command. */
        LoopControlCommandParameters params(LoopControlCommand::Register, handler);
        return this->ProcessControlCommand(std::addressof(params));
    }

    void EventHandlerManager::UnregisterHandler(IEventHandler *handler) {
        /* Check that the handler is valid. */
        AMS_ASSERT(handler != nullptr);
        AMS_ASSERT(handler->IsInitialized());

        /* Send registration command. */
        LoopControlCommandParameters params(LoopControlCommand::Unregister, handler);
        return this->ProcessControlCommand(std::addressof(params));
    }

    void EventHandlerManager::WaitLoopEnter() {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_loop_control_lock);

        /* Wait until we're looping. */
        while (!m_is_looping) {
            m_is_looping_cv.Wait(m_loop_control_lock);
        }
    }

    void EventHandlerManager::WaitLoopExit() {
        /* Acquire exclusive access. */
        std::scoped_lock lk(m_loop_control_lock);

        /* Wait until we're not looping. */
        while (m_is_looping) {
            m_is_looping_cv.Wait(m_loop_control_lock);
        }
    }

    void EventHandlerManager::RequestStop() {
        /* Check that we're looping and not the loop thread. */
        AMS_ASSERT(m_is_looping);
        AMS_ASSERT(!this->IsRunningOnLoopThread());

        if (m_is_looping) {
            /* Acquire exclusive access. */
            std::scoped_lock lk(m_loop_control_lock);

            /* Signal to the loop thread. */
            LoopControlCommandParameters params(LoopControlCommand::Terminate, nullptr);
            m_loop_control_command_params = std::addressof(params);
            m_loop_control_event.Signal();
            m_loop_control_command_done_event.Wait();
        }
    }

    void EventHandlerManager::LoopAuto() {
        /* Check that we're not already looping. */
        AMS_ASSERT(!m_is_looping);

        /* Begin looping with the current thread. */
        m_loop_thread   = os::GetCurrentThread();
        m_is_looping    = true;
        m_is_looping_cv.Broadcast();

        /* Whenever we're done looping, clean up. */
        ON_SCOPE_EXIT {
            m_loop_thread   = nullptr;
            m_is_looping    = false;
            m_is_looping_cv.Broadcast();
        };

        /* Loop until we're asked to stop. */
        bool should_terminate = false;
        while (!should_terminate) {
            /* Wait for a holder to be signaled. */
            os::MultiWaitHolderType *event_holder = os::WaitAny(std::addressof(m_multi_wait));
            AMS_ASSERT(event_holder != nullptr);

            /* Check if we have a request to handle. */
            if (event_holder == std::addressof(m_loop_control_event_holder)) {
                /* Check that the request hasn't already been handled. */
                if (m_loop_control_event.TryWait()) {
                    /* Handle the request. */
                    AMS_ASSERT(m_loop_control_command_params != nullptr);
                    switch (m_loop_control_command_params->command) {
                        case LoopControlCommand::Register:
                        case LoopControlCommand::Unregister:
                            this->ProcessControlCommandImpl(m_loop_control_command_params);
                            break;
                        case LoopControlCommand::Terminate:
                            should_terminate = true;
                            break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }

                    /* Clear the request, and signal that it's done. */
                    m_loop_control_command_params = nullptr;
                    m_loop_control_command_done_event.Signal();
                }
            } else {
                /* Handle the event. */
                IEventHandler::ToEventHandler(event_holder).HandleEvent();
            }
        }
    }

}
