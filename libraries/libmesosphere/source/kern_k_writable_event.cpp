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

    void KWritableEvent::Initialize(KEvent *p) {
        /* Set parent, open a reference to the readable event. */
        this->parent = p;
        this->parent->GetReadableEvent().Open();
    }

    Result KWritableEvent::Signal() {
        return this->parent->GetReadableEvent().Signal();
    }

    Result KWritableEvent::Clear() {
        return this->parent->GetReadableEvent().Clear();
    }

    void KWritableEvent::Destroy() {
        /* Close our references. */
        this->parent->GetReadableEvent().Close();
        this->parent->Close();
    }

}
