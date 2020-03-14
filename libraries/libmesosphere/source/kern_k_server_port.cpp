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
#include <mesosphere.hpp>

namespace ams::kern {

    void KServerPort::Initialize(KPort *parent) {
        /* Set member variables. */
        this->parent = parent;
    }

    bool KServerPort::IsLight() const {
        return this->GetParent()->IsLight();
    }

    void KServerPort::CleanupSessions() {
        /* Ensure our preconditions are met. */
        MESOSPHERE_ASSERT(this->IsLight()  || this->session_list.empty());
        MESOSPHERE_ASSERT(!this->IsLight() || this->light_session_list.empty());

        /* Cleanup the session list. */
        while (true) {
            /* Get the last session in the list */
            KServerSession *session = nullptr;
            {
                KScopedSchedulerLock sl;
                while (!this->session_list.empty()) {
                    session = std::addressof(this->session_list.front());
                    this->session_list.pop_front();
                }
            }

            /* Close the session. */
            if (session != nullptr) {
                session->Close();
            } else {
                break;
            }
        }

        /* Cleanup the light session list. */
        while (true) {
            /* Get the last session in the list */
            KLightServerSession *session = nullptr;
            {
                KScopedSchedulerLock sl;
                while (!this->light_session_list.empty()) {
                    session = std::addressof(this->light_session_list.front());
                    this->light_session_list.pop_front();
                }
            }

            /* Close the session. */
            if (session != nullptr) {
                session->Close();
            } else {
                break;
            }
        }
    }

    void KServerPort::Destroy() {
        /* Note with our parent that we're closed. */
        this->parent->OnClientClosed();

        /* Perform necessary cleanup of our session lists. */
        this->CleanupSessions();

        /* Close our reference to our parent. */
        this->parent->Close();
    }

    bool KServerPort::IsSignaled() const {
        /* TODO: Check preconditions later. */
        MESOSPHERE_ASSERT_THIS();
        if (this->IsLight()) {
            return !this->light_session_list.empty();
        } else {
            return this->session_list.empty();
        }
    }

}
