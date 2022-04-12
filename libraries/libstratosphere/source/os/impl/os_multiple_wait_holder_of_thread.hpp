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
#include "os_multiple_wait_holder_base.hpp"

namespace ams::os::impl {

    class MultiWaitHolderOfThread : public MultiWaitHolderOfUserWaitObject {
        private:
            ThreadType *m_thread;
        private:
            TriBool IsSignaledImpl() const {
                return m_thread->state == ThreadType::State_Terminated ? TriBool::True : TriBool::False;
            }
        public:
            explicit MultiWaitHolderOfThread(ThreadType *t) : m_thread(t) { /* ... */ }

            /* IsSignaled, Link, Unlink implemented. */
            virtual TriBool IsSignaled() const override {
                std::scoped_lock lk(GetReference(m_thread->cs_thread));
                return this->IsSignaledImpl();
            }

            virtual TriBool AddToObjectList() override {
                std::scoped_lock lk(GetReference(m_thread->cs_thread));

                GetReference(m_thread->waitlist).PushBackToList(*this);
                return this->IsSignaledImpl();
            }

            virtual void RemoveFromObjectList() override {
                std::scoped_lock lk(GetReference(m_thread->cs_thread));

                GetReference(m_thread->waitlist).EraseFromList(*this);
            }
    };

}
