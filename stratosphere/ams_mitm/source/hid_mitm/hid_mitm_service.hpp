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

namespace ams::mitm::hid {

    namespace {

        #define AMS_HID_MITM_INTERFACE_INFO(C, H) \
            AMS_SF_METHOD_INFO(C, H, 100, Result, SetSupportedNpadStyleSet, (const sf::ClientAppletResourceUserId &client_aruid, u32 style_set))

        AMS_SF_DEFINE_MITM_INTERFACE(IHidMitmInterface, AMS_HID_MITM_INTERFACE_INFO)

    }

    class HidMitmService : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* TODO: Remove in Atmosphere 0.10.2. */
                /* We will mitm:
                 * - hbl, to help homebrew not need to be recompiled.
                 */
                return client_info.override_status.IsHbl();
            }
        public:
            /* Overridden commands. */
            Result SetSupportedNpadStyleSet(const sf::ClientAppletResourceUserId &client_aruid, u32 style_set);
    };
    static_assert(IsIHidMitmInterface<HidMitmService>);

}
