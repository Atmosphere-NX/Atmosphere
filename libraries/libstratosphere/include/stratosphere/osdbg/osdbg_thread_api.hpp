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
#include <stratosphere/osdbg/osdbg_thread_api_impl.hpp>

namespace ams::osdbg {

    struct ThreadInfo;

    Result InitializeThreadInfo(ThreadInfo *thread_info, os::NativeHandle debug_handle, const svc::DebugInfoCreateProcess *create_process, const svc::DebugInfoCreateThread *create_thread);
    Result UpdateThreadInfo(ThreadInfo *thread_info);

    Result GetThreadName(char *dst, const ThreadInfo *thread_info);

}
