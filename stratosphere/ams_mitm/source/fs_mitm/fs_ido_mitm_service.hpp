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
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>

#define AMS_FS_IDO_MITM_INTERFACE_INFO(C, H) \
    AMS_SF_METHOD_INFO(C, H, 220, Result, GetGameCardCompatibilityType, (sf::Out<u8> out, u32 card_handle), (out, card_handle), hos::Version_9_0_0)


AMS_SF_DEFINE_MITM_INTERFACE(ams::mitm::fs, IDoFsMitmInterface, AMS_FS_IDO_MITM_INTERFACE_INFO, 0x1484E21C)

namespace ams::mitm::fs {

    class FsDoMitmService  : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
            virtual ~FsDoMitmService() {
                serviceClose(m_forward_service.get());
            }
        public:
            /* Overridden commands. */
            Result GetGameCardCompatibilityType(sf::Out<u8> out, u32 card_handle);
    };
    static_assert(IsIDoFsMitmInterface<FsDoMitmService>);

}
