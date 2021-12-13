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

namespace ams::settings::impl {

    class SystemData {
        NON_COPYABLE(SystemData);
        NON_MOVEABLE(SystemData);
        private:
            static constexpr size_t FileNameLengthMax = 31;
        private:
            ncm::SystemDataId m_system_data_id;
            char m_mount_name[fs::MountNameLengthMax + 1];
            char m_file_path[fs::MountNameLengthMax + 1 + 1 + FileNameLengthMax + 1];
        public:
            SystemData() : m_system_data_id(), m_mount_name(), m_file_path() { /* ... */ }

            SystemData(ncm::SystemDataId id, const char *mn) : SystemData() {
                this->SetSystemDataId(id);
                this->SetMountName(mn);
            }

            void SetSystemDataId(ncm::SystemDataId id);
            void SetMountName(const char *name);
            Result Mount();
            Result OpenToRead();
            void Close();
            Result Read(s64 offset, void *dst, size_t size);
    };

}
