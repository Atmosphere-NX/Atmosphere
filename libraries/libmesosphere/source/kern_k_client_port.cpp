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

    void KClientPort::Initialize(KPort *parent, s32 max_sessions) {
        /* Set member variables. */
        this->num_sessions  = 0;
        this->peak_sessions = 0;
        this->parent        = parent;
        this->max_sessions  = max_sessions;
    }

    bool KClientPort::IsLight() const {
        return this->GetParent()->IsLight();
    }

    void KClientPort::Destroy() {
        /* Note with our parent that we're closed. */
        this->parent->OnClientClosed();

        /* Close our reference to our parent. */
        this->parent->Close();
    }

    bool KClientPort::IsSignaled() const {
        /* TODO: Check preconditions later. */
        MESOSPHERE_ASSERT_THIS();
        return this->num_sessions < this->max_sessions;
    }

}
