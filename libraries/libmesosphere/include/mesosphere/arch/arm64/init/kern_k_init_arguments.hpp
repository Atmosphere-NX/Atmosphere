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
        u64 cpuactlr;
        u64 cpuectlr;
        u64 sp;
        u64 entrypoint;
        u64 argument;
    };
    static_assert(alignof(KInitArguments) == util::CeilingPowerOfTwo(INIT_ARGUMENTS_SIZE));
    static_assert(sizeof(KInitArguments) == std::max(INIT_ARGUMENTS_SIZE, util::CeilingPowerOfTwo(INIT_ARGUMENTS_SIZE)));

    static_assert(AMS_OFFSETOF(KInitArguments, cpuactlr)        == INIT_ARGUMENTS_CPUACTLR);
    static_assert(AMS_OFFSETOF(KInitArguments, cpuectlr)        == INIT_ARGUMENTS_CPUECTLR);
    static_assert(AMS_OFFSETOF(KInitArguments, sp)              == INIT_ARGUMENTS_SP);
    static_assert(AMS_OFFSETOF(KInitArguments, entrypoint)      == INIT_ARGUMENTS_ENTRYPOINT);
    static_assert(AMS_OFFSETOF(KInitArguments, argument)        == INIT_ARGUMENTS_ARGUMENT);

}