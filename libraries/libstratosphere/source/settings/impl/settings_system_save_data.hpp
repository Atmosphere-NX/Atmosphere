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

    class SystemSaveData {
        NON_COPYABLE(SystemSaveData);
        private:
            fs::SystemSaveDataId m_system_save_data_id;
            fs::SaveDataSpaceId m_save_data_space_id;
            s64 m_total_size;
            s64 m_journal_size;
            u32 m_flags;
            char m_mount_name[fs::MountNameLengthMax + 1];
        public:
            SystemSaveData() : m_system_save_data_id(0), m_save_data_space_id(fs::SaveDataSpaceId::System), m_total_size(0), m_journal_size(0), m_flags(0) { /* ... */ }

            SystemSaveData(u64 id, s64 total_size, s64 journal_size, u32 flags, const char *mn) : SystemSaveData() {
                this->SetSystemSaveDataId(id);
                this->SetTotalSize(total_size);
                this->SetJournalSize(journal_size);
                this->SetFlags(flags);
                this->SetMountName(mn);
            }

            void SetSystemSaveDataId(u64 id);
            void SetTotalSize(s64 size);
            void SetJournalSize(s64 size);
            void SetFlags(u32 flags);
            void SetMountName(const char *name);

            Result Mount(bool create_save);
            Result Commit(bool synchronous);
            Result Create(s64 size);
            Result OpenToRead();
            Result OpenToWrite();
            void Close();
            Result Read(s64 offset, void *buf, size_t size);
            Result Write(s64 offset, const void *buf, size_t size);
            Result Flush();
            Result SetFileSize(s64 size);
    };

}
