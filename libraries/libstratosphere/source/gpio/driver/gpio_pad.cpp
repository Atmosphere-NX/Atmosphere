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
#include <stratosphere.hpp>

#include "impl/gpio_driver_core.hpp"

namespace ams::gpio::driver {

    bool Pad::IsAnySessionBoundToInterrupt() const {
        /* Check to see if any session has an interrupt bound. */
        bool bound = false;
        this->ForEachSession([&](const ddsf::ISession &session) -> bool {
           const auto &impl = session.SafeCastTo<impl::PadSessionImpl>();
           if (impl.IsInterruptBound()) {
               bound = true;
               return false;
           }
           return true;
        });

        return bound;
    }

    void Pad::SignalInterruptBoundEvent() {
        /* Signal relevant sessions. */
        this->ForEachSession([&](ddsf::ISession &session) -> bool {
           auto &impl = session.SafeCastTo<impl::PadSessionImpl>();
           if (impl.IsInterruptBound()) {
               impl.SignalInterruptBoundEvent();
           }
           return true;
        });
    }

}
