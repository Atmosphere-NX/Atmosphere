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
#include "sprofile_srv_profile_manager.hpp"
#include "sprofile_srv_service_for_bg_agent.hpp"
#include "sprofile_srv_profile_importer_impl.hpp"
#include "sprofile_srv_fs_utils.hpp"

namespace ams::sprofile::srv {

    Result ServiceForBgAgent::OpenProfileImporter(sf::Out<sf::SharedPointer<::ams::sprofile::IProfileImporter>> out) {
        /* Allocate an object. */
        auto obj = sf::ObjectFactory<sf::MemoryResourceAllocationPolicy>::CreateSharedEmplaced<IProfileImporter, ProfileImporterImpl>(m_memory_resource, m_profile_manager);
        R_UNLESS(obj != nullptr, sprofile::ResultAllocationFailed());

        /* Confirm that we can begin an import. */
        R_TRY(m_profile_manager->OpenProfileImporter());

        /* Return the object. */
        *out = std::move(obj);
        return ResultSuccess();
    }

    Result ServiceForBgAgent::GetImportableProfileUrls(sf::Out<u32> out_count, const sf::OutArray<sprofile::srv::ProfileUrl> &out, const sprofile::srv::ProfileMetadataForImportMetadata &arg) {
        /* Check size. */
        R_UNLESS(out.GetSize() >= arg.metadata.num_entries, sprofile::ResultInvalidArgument());

        /* Load primary metadata. */
        sprofile::srv::ProfileMetadata primary_metadata;
        R_TRY_CATCH(m_profile_manager->LoadPrimaryMetadata(std::addressof(primary_metadata))) {
            R_CATCH(fs::ResultPathNotFound) {
                /* It's okay if we have no primary metadata -- this means that all profiles are importable. */
                primary_metadata.num_entries = 0;
            }
        } R_END_TRY_CATCH;

        /* We want to return the set of profiles that can be imported, which is just the profiles we don't already have. */
        u32 count = 0;
        for (u32 i = 0; i < arg.metadata.num_entries; ++i) {
            const auto &arg_entry = arg.metadata.entries[i];

            /* Check if we have the entry. */
            bool have_entry = false;
            for (u32 j = 0; j < primary_metadata.num_entries; ++j) {
                const auto &pri_entry = primary_metadata.entries[j];

                if (pri_entry.identifier_0 == arg_entry.identifier_0 && pri_entry.identifier_1 == arg_entry.identifier_1) {
                    have_entry = true;
                    break;
                }
            }

            /* If we don't already have the entry, it's importable -- copy it out. */
            if (!have_entry) {
                out[count++] = arg.profile_urls[i];
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
