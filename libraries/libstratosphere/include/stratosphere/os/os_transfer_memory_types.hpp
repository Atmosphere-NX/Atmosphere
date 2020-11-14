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
#include <vapours.hpp>
#include <stratosphere/os/impl/os_internal_critical_section.hpp>

namespace ams::os {

    struct TransferMemoryType {
        enum State {
            State_NotInitialized = 0,
            State_Created        = 1,
            State_Mapped         = 2,
            State_Detached       = 3,
        };

        u8 state;
        bool handle_managed;
        bool allocated;

        void *address;
        size_t size;
        Handle handle;

        mutable impl::InternalCriticalSectionStorage cs_transfer_memory;
    };
    static_assert(std::is_trivial<TransferMemoryType>::value);

}
