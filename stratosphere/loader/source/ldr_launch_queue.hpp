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

#define LAUNCH_QUEUE_SIZE (10)
#define LAUNCH_QUEUE_FULL (-1)

#define LAUNCH_QUEUE_ARG_SIZE_MAX (0x8000)

class LaunchQueue { 
    public:
        struct LaunchItem {
            u64 tid;
            u64 arg_size;
            char args[LAUNCH_QUEUE_ARG_SIZE_MAX];
        };
        
        static LaunchQueue::LaunchItem *get_item(u64 tid);
        
        static Result add(u64 tid, const char *args, u64 arg_size);
        static Result add_item(const LaunchItem *item);
        static Result add_copy(u64 tid_base, u64 new_tid);
        static int get_index(u64 tid);
        static int get_free_index(u64 tid);
        static bool contains(u64 tid);
        static void clear();
};