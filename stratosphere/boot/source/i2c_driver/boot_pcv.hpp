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

/* pcv isn't alive at the time boot runs, but nn::i2c::driver needs nn::pcv. */
/* These are the overrides N puts in boot. */

class Pcv {
    public:
        static void Initialize();
        static void Finalize();
        static Result SetClockRate(PcvModule module, u32 hz);
        static Result SetClockEnabled(PcvModule module, bool enabled);
        static Result SetVoltageEnabled(u32 domain, bool enabled);
        static Result SetVoltageValue(u32 domain, u32 voltage);
        static Result SetReset(PcvModule module, bool reset);
};