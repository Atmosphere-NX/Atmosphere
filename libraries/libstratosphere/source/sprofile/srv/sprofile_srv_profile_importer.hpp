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
#include "sprofile_srv_types.hpp"

namespace ams::sprofile::srv {

    class ProfileImporter {
        private:
            struct ImportingProfile {
                Identifier identifier_0;
                Identifier identifier_1;
                bool is_new_import;
            };
        private:
            bool m_committed;
            bool m_imported_metadata;
            int m_importing_count;
            util::optional<ProfileMetadata> m_metadata;
            ImportingProfile m_importing_profiles[50];
            util::BitFlagSet<50> m_is_profile_importable;
            Identifier m_revision_key;
        public:
            ProfileImporter(const util::optional<ProfileMetadata> &meta) : m_committed(false), m_imported_metadata(false), m_importing_count(0), m_metadata(util::nullopt), m_is_profile_importable(), m_revision_key() {
                if (meta.has_value()) {
                    m_metadata = *meta;
                }
            }
        public:
            bool HasProfile(Identifier id0, Identifier id1) {
                /* Require that we have metadata. */
                if (m_metadata.has_value()) {
                    for (auto i = 0u; i < std::min<size_t>(m_metadata->num_entries, util::size(m_metadata->entries)); ++i) {
                        const auto &entry = m_metadata->entries[i];
                        if (entry.identifier_0 == id0 && entry.identifier_1 == id1) {
                            return true;
                        }
                    }
                }

                /* We don't have the desired profile. */
                return false;
            }

            bool CanImportMetadata() {
                /* We can only import metadata if we haven't already imported metadata. */
                return !m_imported_metadata;
            }

            void ImportMetadata(const ProfileMetadata &meta) {
                /* Set that we've imported metadata. */
                m_imported_metadata = true;

                /* Import the service revision key. */
                m_revision_key = meta.revision_key;

                /* Set importing count. */
                m_importing_count = static_cast<int>(std::min<size_t>(meta.num_entries, util::size(meta.entries)));

                /* Set all profiles as importable. */
                for (auto i = 0; i < m_importing_count; ++i) {
                    m_is_profile_importable[i] = true;
                }

                /* Determine import status for all profiles. */
                for (auto i = 0; i < m_importing_count; ++i) {
                    const auto &import_entry = meta.entries[i];

                    const bool is_new_import = !this->HasProfile(import_entry.identifier_0, import_entry.identifier_1);

                    m_importing_profiles[i] = {
                        .identifier_0  = import_entry.identifier_0,
                        .identifier_1  = import_entry.identifier_1,
                        .is_new_import = is_new_import,
                    };
                    m_is_profile_importable[i] = is_new_import;
                }
            }

            bool CanImportProfile(Identifier profile) {
                /* Require that we imported metadata. */
                if (m_imported_metadata) {
                    /* Find the specified profile. */
                    for (auto i = 0; i < m_importing_count; ++i) {
                        if (m_importing_profiles[i].identifier_0 == profile) {
                            /* Require the profile be importable. */
                            return m_is_profile_importable[i];
                        }
                    }
                }

                /* We can't import the desired profile. */
                return false;
            }

            void OnImportProfile(Identifier profile) {
                /* Set the profile as not importable (as it's imported). */
                for (auto i = 0; i < m_importing_count; ++i) {
                    if (m_importing_profiles[i].identifier_0 == profile) {
                        m_is_profile_importable[i] = false;
                        break;
                    }
                }
            }

            bool CanCommit() {
                /* We can't commit if we've already committed. */
                if (m_committed) {
                    return false;
                }

                /* We need metadata in order to commit. */
                if (!m_imported_metadata) {
                    return false;
                }

                /* We need to have imported everything we intended to import. */
                return m_is_profile_importable.IsAllOff();
            }

            int GetImportingCount() const { return m_importing_count; }
            const ImportingProfile &GetImportingProfile(int i) const { return m_importing_profiles[i]; }

            Identifier GetRevisionKey() const { return m_revision_key; }

            Result CleanupOrphanedProfiles(auto cleanup_impl) const {
                /* Cleanup any orphaned profiles in our metadata. */
                if (m_metadata.has_value()) {
                    for (auto i = 0u; i < std::min<size_t>(m_metadata->num_entries, util::size(m_metadata->entries)); ++i) {
                        const auto &entry = m_metadata->entries[i];
                        if (!this->IsImportingProfile(entry.identifier_0)) {
                            R_TRY(cleanup_impl(entry.identifier_0));
                        }
                    }
                }

                R_SUCCEED();
            }
        private:
            bool IsImportingProfile(Identifier profile) const {
                /* Check if we're importing the desired profile. */
                for (auto i = 0; i < m_importing_count; ++i) {
                    if (m_importing_profiles[i].identifier_0 == profile) {
                        return true;
                    }
                }

                /* We're not importing the desired profile. */
                return false;
            }
    };

}
