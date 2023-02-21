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
#include <mesosphere.hpp>

namespace ams::kern::board::nintendo::nx {

    class KSleepManager {
        private:
            static void ResumeEntry(uintptr_t arg);

            static void ProcessRequests(uintptr_t buffer);
        public:
            static void Initialize();
            static void SleepSystem();
        public:
            static void CpuSleepHandler(uintptr_t arg, uintptr_t entry, uintptr_t entry_args);
    };


}