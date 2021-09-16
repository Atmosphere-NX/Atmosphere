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

    Result ServiceForBgAgent::ReadMetadata(sf::Out<u32> out_count, const sf::OutArray<sprofile::srv::ReadMetadataEntry> &out, const sprofile::srv::ReadMetadataArgument &arg) {
        /* Check size. */
        R_UNLESS(out.GetSize() >= arg.metadata.num_entries, sprofile::ResultInvalidArgument());

        /* Load primary metadata. */
        sprofile::srv::ProfileMetadata primary_metadata;
        R_TRY_CATCH(m_profile_manager->LoadPrimaryMetadata(std::addressof(primary_metadata))) {
            R_CATCH(fs::ResultPathNotFound) {
                /* If we have no metadata, we can't get any entries. */
                *out_count = 0;
                return ResultSuccess();
            }
        } R_END_TRY_CATCH;

        /* Copy matching entries. */
        u32 count = 0;
        for (u32 i = 0; i < arg.metadata.num_entries; ++i) {
            const auto &arg_entry = arg.metadata.entries[i];

            for (u32 j = 0; j < primary_metadata.num_entries; ++j) {
                const auto &pri_entry = primary_metadata.entries[j];

                if (pri_entry.identifier_0 == arg_entry.identifier_0 && pri_entry.identifier_1 == arg_entry.identifier_1) {
                    out[count++] = arg.entries[i];
                    break;
                }
            }
        }

        /* Set output count. */
        *out_count = count;
        return ResultSuccess();

    }
    Result ServiceForBgAgent::IsUpdateNeeded(sf::Out<bool> out, Identifier revision_key) {
        /* Load primary metadata. */
        bool loaded_metadata = true;
        sprofile::srv::ProfileMetadata primary_metadata;
        R_TRY_CATCH(m_profile_manager->LoadPrimaryMetadata(std::addressof(primary_metadata))) {
            R_CATCH(fs::ResultPathNotFound) {
                /* If we have no metadata, we don't have a revision key. */
                loaded_metadata = false;
            }
        } R_END_TRY_CATCH;

        /* Determine if update is needed. */
        *out = !(loaded_metadata && revision_key == primary_metadata.revision_key);
        return ResultSuccess();
    }

    Result ServiceForBgAgent::Reset() {
        return m_profile_manager->ResetSaveData();
    }

}
