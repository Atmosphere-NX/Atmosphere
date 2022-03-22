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
#include <vapours.hpp>
#include <stratosphere/osdbg/osdbg_thread_types.hpp>

namespace ams::osdbg {

    constexpr inline s32 GetThreadPriority(const ThreadInfo *thread_info) {
        return thread_info->_base_priority;
    }

    constexpr inline s32 GetThreadCurrentPriority(const ThreadInfo *thread_info) {
        return thread_info->_current_priority;
    }

    constexpr inline size_t GetThreadStackSize(const ThreadInfo *thread_info) {
        return thread_info->_stack_size;
    }

    constexpr inline uintptr_t GetThreadStackAddress(const ThreadInfo *thread_info) {
        return thread_info->_stack;
    }

    constexpr inline uintptr_t GetThreadFunction(const ThreadInfo *thread_info) {
        return thread_info->_function;
    }

    constexpr inline uintptr_t GetThreadFunctionArgument(const ThreadInfo *thread_info) {
        return thread_info->_argument;
    }

    constexpr inline uintptr_t GetThreadNamePointer(const ThreadInfo *thread_info) {
        return thread_info->_name_pointer;
    }

}
