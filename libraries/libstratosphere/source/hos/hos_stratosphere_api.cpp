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
#include <stratosphere.hpp>
#include "../os/impl/os_rng_manager.hpp"

namespace ams::os {

    void InitializeForStratosphereInternal();

}

extern "C" {

    /* Provide libnx address space allocation shim. */
    uintptr_t __libnx_virtmem_rng(void) {
        return static_cast<uintptr_t>(::ams::os::impl::GetRngManager().GenerateRandomU64());
    }

}

namespace ams::hos {

    namespace {

        bool CanAllowTemporaryApproximateVersion() {
            /* Check if we're a program that can use a temporary approximate version. */
            const auto program_id = os::GetCurrentProgramId();

            return program_id == ncm::SystemProgramId::Pm   || /* Needed so that boot has permissions to use fsp-srv. */
                   program_id == ncm::SystemProgramId::Sm   || /* Needed so that boot can acquire fsp-srv. */
                   program_id == ncm::SystemProgramId::Boot || /* Needed so that boot can set the true target firmware. */
                   program_id == ncm::SystemProgramId::Spl  || /* Needed so that FS can complete initialization. */
                   program_id == ncm::SystemProgramId::Ncm;    /* Needed so that FS can determine where SystemVersion is located. */
        }

    }

    bool IsUnitTestProgramForSetVersion();
    void InitializeVersionInternal(bool allow_approximate);

    void InitializeForStratosphere() {
        /* Initialize the global os resource managers. This *must* be done before anything else in stratosphere. */
        os::InitializeForStratosphereInternal();

        /* Initialize hos::Version API. */
        hos::InitializeVersionInternal(CanAllowTemporaryApproximateVersion());

        /* Check that we're running under mesosphere. */
        AMS_ABORT_UNLESS(IsUnitTestProgramForSetVersion() || svc::IsKernelMesosphere());
    }

}
