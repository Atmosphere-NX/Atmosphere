/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <vapours.hpp>

namespace ams::ncm {

    struct ContentManagerConfig {
        bool build_system_database;
        bool import_database_from_system_on_sd;

        bool HasAnyConfig() const {
            return this->ShouldBuildDatabase() || this->import_database_from_system_on_sd;
        }

        bool ShouldBuildDatabase() const {
            return hos::GetVersion() < hos::Version_4_0_0 || this->build_system_database;
        }

        bool ShouldImportDatabaseFromSignedSystemPartitionOnSd() const {
            return this->import_database_from_system_on_sd;
        }
    };

}
