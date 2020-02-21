/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "hvisor_core_context.hpp"
#include "cpu/hvisor_cpu_instructions.hpp"

namespace ams::hvisor {

    std::array<CoreContext, MAX_CORE> CoreContext::instances{};
    std::atomic<u32> CoreContext::activeCoreMask{};
    bool CoreContext::coldboot = true;

    void CoreContext::InitializeCoreInstance(u32 coreId, bool isBootCore, u64 argument)
    {
        CoreContext &instance = instances[coreId];
        instance.m_coreId = coreId;
        instance.m_bootCore = isBootCore;
        instance.m_kernelArgument = argument;
        if (isBootCore && instance.m_kernelEntrypoint == 0) {
            instance.m_kernelEntrypoint = initialKernelEntrypoint;
        }
        currentCoreCtx = &instance;
        cpu::dmb();
    }
}
