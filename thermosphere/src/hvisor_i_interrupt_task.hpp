/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "defines.hpp"
#include <vapours/util/util_intrusive_list.hpp>

namespace ams::hvisor {

    class IInterruptTask : public util::IntrusiveListBaseNode<IInterruptTask> {
        NON_COPYABLE(IInterruptTask);
        NON_MOVEABLE(IInterruptTask);
        protected:
            constexpr IInterruptTask() = default;
        public:
            virtual std::optional<bool> InterruptTopHalfHandler(u32 irqId, u32 srcCore) = 0;
            virtual void InterruptBottomHalfHandler(u32 irqId, u32 srcCore) {}
    };

}
