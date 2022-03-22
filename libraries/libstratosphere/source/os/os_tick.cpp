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
#include "impl/os_tick_manager.hpp"

namespace ams::os {

    Tick GetSystemTick() {
        return impl::GetTickManager().GetTick();
    }

    Tick GetSystemTickOrdered() {
        return impl::GetTickManager().GetSystemTickOrdered();
    }

    s64 GetSystemTickFrequency() {
        return impl::GetTickManager().GetTickFrequency();
    }

    TimeSpan ConvertToTimeSpan(Tick tick) {
        return impl::GetTickManager().ConvertToTimeSpan(tick);
    }

    Tick ConvertToTick(TimeSpan ts) {
        return impl::GetTickManager().ConvertToTick(ts);
    }

}
