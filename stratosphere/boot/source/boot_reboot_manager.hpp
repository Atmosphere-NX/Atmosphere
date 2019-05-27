/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stratosphere.hpp>

#define IRAM_BASE             0x40000000ull
#define IRAM_SIZE             0x40000
#define IRAM_PAYLOAD_MAX_SIZE 0x2E000
#define IRAM_PAYLOAD_BASE 0x40010000ull

class BootRebootManager {
    public:
        static Result PerformReboot();
        static void RebootForFatalError(AtmosphereFatalErrorContext *ctx);
};