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
#include <stratosphere.hpp>

namespace ams::mitm::bpc {

    namespace impl {

        #define AMS_BPC_MITM_INTERFACE_INFO(C, H)                   \
            AMS_SF_METHOD_INFO(C, H, 0, Result, ShutdownSystem, ()) \
            AMS_SF_METHOD_INFO(C, H, 1, Result, RebootSystem,   ())

        AMS_SF_DEFINE_MITM_INTERFACE(IBpcMitmInterface, AMS_BPC_MITM_INTERFACE_INFO)

    }

    class BpcMitmService : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - am, to intercept the Reboot/Power buttons in the overlay menu.
                 * - fatal, to simplify payload reboot logic significantly
                 * - hbl, to allow homebrew to take advantage of the feature.
                 */
                return client_info.program_id == ncm::SystemProgramId::Am ||
                       client_info.program_id == ncm::SystemProgramId::Fatal ||
                       client_info.override_status.IsHbl();
            }
        public:
            /* Overridden commands. */
            Result ShutdownSystem();
            Result RebootSystem();
    };
    static_assert(impl::IsIBpcMitmInterface<BpcMitmService>);

}
