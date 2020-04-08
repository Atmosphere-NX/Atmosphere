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
#include <stratosphere.hpp>

namespace ams::os::impl {

    class WaitableObjectList;
    class WaitableManagerImpl;

    class WaitableHolderBase {
        private:
            WaitableManagerImpl *manager = nullptr;
        public:
            util::IntrusiveListNode manager_node;
            util::IntrusiveListNode object_list_node;
        public:
            /* Gets whether the held waitable is currently signaled. */
            virtual TriBool IsSignaled() const = 0;
            /* Adds to manager's object list, returns is signaled. */
            virtual TriBool LinkToObjectList() = 0;
            /* Removes from the manager's object list. */
            virtual void UnlinkFromObjectList() = 0;
            /* Gets handle to output, returns INVALID_HANDLE on failure. */
            virtual Handle GetHandle() const = 0;
            /* Gets the amount of time remaining until this wakes up. */
            virtual TimeSpan GetAbsoluteWakeupTime() const {
                return TimeSpan::FromNanoSeconds(std::numeric_limits<s64>::max());
            }

            /* Interface with manager. */
            void SetManager(WaitableManagerImpl *m) {
                this->manager = m;
            }

            WaitableManagerImpl *GetManager() const {
                return this->manager;
            }

            bool IsLinkedToManager() const {
                return this->manager != nullptr;
            }
    };

    class WaitableHolderOfUserObject : public WaitableHolderBase {
        public:
            /* All user objects have no handle to wait on. */
            virtual Handle GetHandle() const override final {
                return svc::InvalidHandle;
            }
    };

    class WaitableHolderOfKernelObject : public WaitableHolderBase {
        public:
            /* All kernel objects have native handles, and thus don't have object list semantics. */
            virtual TriBool LinkToObjectList() override final {
                return TriBool::Undefined;
            }
            virtual void UnlinkFromObjectList() override final {
                /* ... */
            }
    };

}
