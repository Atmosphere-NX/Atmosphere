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
#include <stratosphere.hpp>
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_service_for_bg_agent.hpp"
#include "sprofile_srv_fs_utils.hpp"

namespace ams::sprofile::srv {

    Result ServiceForBgAgent::OpenProfileImporter(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileImporter>> out) {
        AMS_ABORT("TODO: OpenProfileImporter");
    }

    Result ServiceForBgAgent::ReadMetadata(sf::Out<u32> out_count, const sf::OutBuffer &out_buf, const sf::InBuffer &meta) {
        WriteFile("sprof-dbg:/sprof/meta.bin", meta.GetPointer(), meta.GetSize());
        AMS_ABORT("TODO: ReadMetadata");
    }
    Result ServiceForBgAgent::IsUpdateNeeded(sf::Out<bool> out, Identifier revision_key) {
        WriteFile("sprof-dbg:/sprof/revision_key.bin", std::addressof(revision_key), sizeof(revision_key));
        AMS_ABORT("TODO: IsUpdateNeeded");
    }

    Result ServiceForBgAgent::Reset() {
        AMS_ABORT("TODO: Reset");
    }

}
