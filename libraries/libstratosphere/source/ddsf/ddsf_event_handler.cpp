/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
        if (this->is_initialized) {
            return;
        }

        /* Initialize waitable manager/holder. */
        os::InitializeWaitableManager(std::addressof(this->waitable_manager));
        os::InitializeWaitableHolder(std::addressof(this->loop_control_event_holder), this->loop_control_event.GetBase());
        os::LinkWaitableHolder(std::addressof(this->waitable_manager), std::addressof(this->loop_control_event_holder));

        this->is_initialized = true;
    }

    void EventHandlerManager::Finalize() {
        /* Check that we're initialized and not looping. */
        AMS_ASSERT(!this->is_looping);
        AMS_ASSERT(this->is_initialized);
        if (!this->is_initialized) {
            return;
        }

        /* Finalize waitable manager/holder. */
        os::UnlinkWaitableHolder(std::addressof(this->loop_control_event_holder));
        os::FinalizeWaitableHolder(std::addressof(this->loop_control_event_holder));
        os::FinalizeWaitableManager(std::addressof(this->waitable_manager));

        this->is_initialized = false;
    }

    void EventHandlerManager::ProcessControlCommand(LoopControlCommandParameters *params) {
        /* Check pre-conditions. */
        AMS_ASSERT(this->is_initialized);
        AMS_ASSERT(params != nullptr);

        /* Acquire exclusive access. */
        std::scoped_lock lk(this->loop_control_lock);

        /* If we're processing for the loop thread, we can directly handle. */
        if (!this->is_looping || this->IsRunningOnLoopThread()) {
            this->ProcessControlCommandImpl(params);
        } else {
            /* Otherwise, signal to the loop thread. */
            this->loop_control_command_params = params;
            this->loop_control_event.Signal();
            this->loop_control_command_done_event.Wait();
        }
    }

    void EventHandlerManager::ProcessControlCommandImpl(LoopControlCommandParameters *params) {
        /* Check pre-conditions. */
        AMS_ASSERT(this->loop_control_lock.IsLockedByCurrentThread() || !this->loop_control_lock.TryLock());
        AMS_ASSERT(params != nullptr);
        AMS_ASSERT(params->target != nullptr);

        /* Process the command. */
        switch (params->command) {
            case LoopControlCommand::Register:
                params->target->Link(std::addressof(this->waitable_manager));
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
        std::scoped_lock lk(this->loop_control_lock);

        /* Wait until we're looping. */
        while (!this->is_looping) {
            this->is_looping_cv.Wait(this->loop_control_lock);
        }
    }

    void EventHandlerManager::WaitLoopExit() {
        /* Acquire exclusive access. */
        std::scoped_lock lk(this->loop_control_lock);

        /* Wait until we're not looping. */
        while (this->is_looping) {
            this->is_looping_cv.Wait(this->loop_control_lock);
        }
    }

    void EventHandlerManager::RequestStop() {
        /* Check that we're looping and not the loop thread. */
        AMS_ASSERT(this->is_looping);
        AMS_ASSERT(!this->IsRunningOnLoopThread());

        if (this->is_looping) {
            /* Acquire exclusive access. */
            std::scoped_lock lk(this->loop_control_lock);

            /* Signal to the loop thread. */
            LoopControlCommandParameters params(LoopControlCommand::Terminate, nullptr);
            this->loop_control_command_params = std::addressof(params);
            this->loop_control_event.Signal();
            this->loop_control_command_done_event.Wait();
        }
    }

    void EventHandlerManager::LoopAuto() {
        /* Check that we're not already looping. */
        AMS_ASSERT(!this->is_looping);

        /* Begin looping with the current thread. */
        this->loop_thread   = os::GetCurrentThread();
        this->is_looping    = true;
        this->is_looping_cv.Broadcast();

        /* Whenever we're done looping, clean up. */
        ON_SCOPE_EXIT {
            this->loop_thread   = nullptr;
            this->is_looping    = false;
            this->is_looping_cv.Broadcast();
        };

        /* Loop until we're asked to stop. */
        bool should_terminate = false;
        while (!should_terminate) {
            /* Wait for a holder to be signaled. */
            os::WaitableHolderType *event_holder = os::WaitAny(std::addressof(this->waitable_manager));
            AMS_ASSERT(event_holder != nullptr);

            /* Check if we have a request to handle. */
            if (event_holder == std::addressof(this->loop_control_event_holder)) {
                /* Check that the request hasn't already been handled. */
                if (this->loop_control_event.TryWait()) {
                    /* Handle the request. */
                    AMS_ASSERT(this->loop_control_command_params != nullptr);
                    switch (this->loop_control_command_params->command) {
                        case LoopControlCommand::Register:
                        case LoopControlCommand::Unregister:
                            this->ProcessControlCommandImpl(this->loop_control_command_params);
                            break;
                        case LoopControlCommand::Terminate:
                            should_terminate = true;
                            break;
                        AMS_UNREACHABLE_DEFAULT_CASE();
                    }

                    /* Clear the request, and signal that it's done. */
                    this->loop_control_command_params = nullptr;
                    this->loop_control_command_done_event.Signal();
                }
            } else {
                /* Handle the event. */
                IEventHandler::ToEventHandler(event_holder).HandleEvent();
            }
        }
    }

}
