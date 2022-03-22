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
#include <stratosphere.hpp>

#define AMS_BPC_MITM_ATMOSPHERE_INTERFACE_INTERFACE_INFO(C, H)                                                \
    AMS_SF_METHOD_INFO(C, H, 65000, void, RebootToFatalError, (const ams::FatalErrorContext &ctx), (ctx))     \
    AMS_SF_METHOD_INFO(C, H, 65001, void, SetRebootPayload,   (const ams::sf::InBuffer &payload),  (payload))

AMS_SF_DEFINE_INTERFACE(ams::mitm::bpc::impl, IAtmosphereInterface, AMS_BPC_MITM_ATMOSPHERE_INTERFACE_INTERFACE_INFO)

namespace ams::mitm::bpc {

    class AtmosphereService {
        public:
            void RebootToFatalError(const ams::FatalErrorContext &ctx);
            void SetRebootPayload(const ams::sf::InBuffer &payload);
    };
    static_assert(impl::IsIAtmosphereInterface<AtmosphereService>);

}
