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

namespace ams::kern::init {

    struct KInitArguments {
        u64 ttbr0;
        u64 ttbr1;
        u64 tcr;
        u64 mair;
        u64 cpuactlr;
        u64 cpuectlr;
        u64 sctlr;
        u64 sp;
        u64 entrypoint;
        u64 argument;
        u64 setup_function;
        u64 exception_stack;
    };

}