/*
 * Copyright (c) 2018 Atmosphère-NX
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
#include <switch.h>
#include <atomic>
#include <vector>

class WaitableManagerBase {
    std::atomic<u64> cur_priority = 0;
    public:
        WaitableManagerBase() = default;
        virtual ~WaitableManagerBase() = default;

        u64 get_priority() {
            return std::atomic_fetch_add(&cur_priority, (u64)1);
        }
};
