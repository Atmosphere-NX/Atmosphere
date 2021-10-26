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
#include "sprofile_srv_profile_importer_impl.hpp"

namespace ams::sprofile::srv {

    Result ProfileImporterImpl::ImportProfile(const sprofile::srv::ProfileDataForImportData &import) {
        return m_manager->ImportProfile(import);
    }

    Result ProfileImporterImpl::Commit() {
        return m_manager->Commit();
    }

    Result ProfileImporterImpl::ImportMetadata(const sprofile::srv::ProfileMetadataForImportMetadata &import) {
        return m_manager->ImportMetadata(import);
    }

}
