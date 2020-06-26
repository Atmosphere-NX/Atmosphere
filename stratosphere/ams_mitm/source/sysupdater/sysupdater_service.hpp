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

namespace ams::mitm::sysupdater {

    constexpr inline size_t FirmwareVariationCountMax = 16;

    struct UpdateInformation {
        u32 version;
        bool exfat_supported;
        u32 firmware_variation_count;
        ncm::FirmwareVariationId firmware_variation_ids[FirmwareVariationCountMax];
    };

    class SystemUpdateService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                GetUpdateInformation = 0,
            };
        private:
            Result GetUpdateInformation(sf::Out<UpdateInformation> out, const ncm::Path &path);
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetUpdateInformation),
            };
    };

}
