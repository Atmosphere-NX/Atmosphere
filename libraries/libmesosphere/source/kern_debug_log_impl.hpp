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
#include <mesosphere.hpp>

namespace ams::kern {

    class KDebugLogImpl {
        public:
            static NOINLINE bool Initialize();
            static NOINLINE void PutChar(char c);
            static NOINLINE void Flush();

            /* Functionality for preserving across sleep. */
            static NOINLINE void Save();
            static NOINLINE void Restore();
    };

}
