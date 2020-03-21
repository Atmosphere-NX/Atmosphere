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
#include <stratosphere/ncm/ncm_content_meta.hpp>
#include <stratosphere/ncm/ncm_install_progress.hpp>
#include <stratosphere/ncm/ncm_system_update_task_apply_info.hpp>

namespace ams::ncm {

    struct InstallContentMeta {
        std::unique_ptr<char[]> data;
        size_t size;

        InstallContentMeta GetReader() const {
            return InstallContentMetaReader(this->data.get(), this->size);
        }
    };

    class InstallTaskDataBase {
        public:
            Result Get(InstallContentMeta *out, s32 index);
            Result Update(const InstallContentMeta &content_meta, s32 index);
            Result Has(bool *out, u64 id);
        public:
            virtual Result GetProgress(InstallProgress *out_progress) = 0;
            virtual Result GetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo *out_info) = 0;
            virtual Result SetState(InstallProgressState state) = 0;
            virtual Result SetLastResult(Result result) = 0;
            virtual Result SetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo info) = 0;
            virtual Result Push(const void *data, size_t data_size) = 0;
            virtual Result Count(s32 *out) = 0;
            virtual Result Delete(const ContentMetaKey *keys, s32 num_keys) = 0;
            virtual Result Cleanup() = 0;
        private:
            virtual Result GetSize(size_t *out_size, s32 index) = 0;
            virtual Result Get(s32 index, void *out, size_t out_size) = 0;
            virtual Result Update(s32 index, const void *data, size_t data_size) = 0;
    };

    /* TODO: MemoryInstallTaskData */

    class FileInstallTaskData : public InstallTaskDataBase {
        private:
            struct Header {
                s32 max_entries;
                s32 count;
                s64 last_data_offset;
                Result last_result;
                InstallProgressState progress_state;
                SystemUpdateTaskApplyInfo system_update_task_apply_info;
            };

            static_assert(sizeof(Header) == 0x18);

            struct EntryInfo {
                s64 offset;
                s64 size;
            };

            static_assert(sizeof(EntryInfo) == 0x10);
        private:
            Header header;
            char path[64];
        private:
            static constexpr s64 GetEntryInfoOffset(s32 index) {
                return index * sizeof(EntryInfo) + sizeof(Header);
            }

            static constexpr Header MakeInitialHeader(s32 max_entries) {
                return {
                    .max_entries                      = max_entries,
                    .count                            = 0,
                    .last_data_offset                 = GetEntryInfoOffset(max_entries),
                    .last_result                      = ResultSuccess(),
                    .progress_state                   = InstallProgressState::NotPrepared,
                    .system_update_task_apply_info    = SystemUpdateTaskApplyInfo::Unknown,
                };
            }
        public:
            static Result Create(const char *path, s32 max_entries);
            Result Initialize(const char *path);
        public:
            virtual Result GetProgress(InstallProgress *out_progress) override;
            virtual Result GetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo *out_info) override;
            virtual Result SetState(InstallProgressState state) override;
            virtual Result SetLastResult(Result result) override;
            virtual Result SetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo info) override;
            virtual Result Push(const void *data, size_t data_size) override;
            virtual Result Count(s32 *out) override;
            virtual Result Delete(const ContentMetaKey *keys, s32 num_keys) override;
            virtual Result Cleanup() override;
        private:
            Result GetEntryInfo(EntryInfo *out_entry_info, s32 index);

            Result Read(void *out, size_t out_size, s64 offset);
            Result Write(const void *data, size_t size, s64 offset);
            Result WriteHeader();

            virtual Result GetSize(size_t *out_size, s32 index) override;
            virtual Result Get(s32 index, void *out, size_t out_size) override;
            virtual Result Update(s32 index, const void *data, size_t data_size) override;
    };

}
