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
#include <mesosphere/kern_select_assembly_offsets.h>

namespace ams::kern::init {

    struct alignas(util::CeilingPowerOfTwo(INIT_ARGUMENTS_SIZE)) KInitArguments {
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
    static_assert(alignof(KInitArguments) == util::CeilingPowerOfTwo(INIT_ARGUMENTS_SIZE));
    static_assert(sizeof(KInitArguments) == std::max(INIT_ARGUMENTS_SIZE, util::CeilingPowerOfTwo(INIT_ARGUMENTS_SIZE)));

    static_assert(AMS_OFFSETOF(KInitArguments, ttbr0)           == INIT_ARGUMENTS_TTBR0);
    static_assert(AMS_OFFSETOF(KInitArguments, ttbr1)           == INIT_ARGUMENTS_TTBR1);
    static_assert(AMS_OFFSETOF(KInitArguments, tcr)             == INIT_ARGUMENTS_TCR);
    static_assert(AMS_OFFSETOF(KInitArguments, mair)            == INIT_ARGUMENTS_MAIR);
    static_assert(AMS_OFFSETOF(KInitArguments, cpuactlr)        == INIT_ARGUMENTS_CPUACTLR);
    static_assert(AMS_OFFSETOF(KInitArguments, cpuectlr)        == INIT_ARGUMENTS_CPUECTLR);
    static_assert(AMS_OFFSETOF(KInitArguments, sctlr)           == INIT_ARGUMENTS_SCTLR);
    static_assert(AMS_OFFSETOF(KInitArguments, sp)              == INIT_ARGUMENTS_SP);
    static_assert(AMS_OFFSETOF(KInitArguments, entrypoint)      == INIT_ARGUMENTS_ENTRYPOINT);
    static_assert(AMS_OFFSETOF(KInitArguments, argument)        == INIT_ARGUMENTS_ARGUMENT);
    static_assert(AMS_OFFSETOF(KInitArguments, setup_function)  == INIT_ARGUMENTS_SETUP_FUNCTION);
    static_assert(AMS_OFFSETOF(KInitArguments, exception_stack) == INIT_ARGUMENTS_EXCEPTION_STACK);

}