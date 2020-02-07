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
#include <mesosphere.hpp>

namespace ams::kern {

    NORETURN void HorizonKernelMain(s32 core_id) {
        /* Setup the Core Local Region, and note that we're initializing. */
        Kernel::InitializeCoreLocalRegion(core_id);
        Kernel::SetState(Kernel::State::Initializing);

        /* Ensure that all cores get to this point before proceeding. */
        cpu::SynchronizeAllCores();

        /* Initialize the main and idle thread for each core. */
        /* Synchronize after each init to ensure the cores go in order. */
        for (size_t i = 0; i < cpu::NumCores; i++) {
            if (static_cast<s32>(i) == core_id) {
                Kernel::InitializeMainAndIdleThreads(core_id);
            }
            cpu::SynchronizeAllCores();
        }

        if (core_id == 0) {
            /* Initialize KSystemControl. */
            KSystemControl::Initialize();

            /* Initialize the memory manager. */
            {
                const auto &metadata_region = KMemoryLayout::GetMetadataPoolRegion();
                Kernel::GetMemoryManager().Initialize(metadata_region.GetAddress(), metadata_region.GetSize());
            }

            /* Note: this is not actually done here, it's done later in main after more stuff is setup. */
            /* However, for testing (and to manifest this code in the produced binary, this is here for now. */
            /* TODO: Do this better. */
            init::InitializeSlabHeaps();
        }

        /* TODO: Implement more of Main() */
        cpu::SynchronizeAllCores();
        while (true) { /* ... */ }
    }

}
