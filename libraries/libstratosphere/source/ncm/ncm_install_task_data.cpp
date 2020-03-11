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
#include <stratosphere.hpp>

namespace ams::ncm {

    namespace {

        using BoundedPath = kvdb::BoundedString<64>;

        constexpr inline bool Includes(const ContentMetaKey *keys, s32 count, const ContentMetaKey &key) {
            for (s32 i = 0; i < count; i++) {
                if (keys[i] == key) {
                    return true;
                }
            }
            return false;
        }

    }

    Result InstallTaskDataBase::Get(InstallContentMeta *out, s32 index) {
        /* Determine the data size. */
        size_t data_size;
        R_TRY(this->GetSize(std::addressof(data_size), index));

        /* Create a buffer and read data into it. */
        std::unique_ptr<char[]> buffer(new (std::nothrow) char[data_size]);
        R_UNLESS(buffer != nullptr, ncm::ResultAllocationFailed());
        R_TRY(this->Get(index, buffer.get(), data_size));

        /* Output the buffer and size. */
        out->data = std::move(buffer);
        out->size = data_size;
        return ResultSuccess();
    }

    Result InstallTaskDataBase::Update(const InstallContentMeta &content_meta, s32 index) {
        return this->Update(index, content_meta.data.get(), content_meta.size);
    }

    Result InstallTaskDataBase::Has(bool *out, u64 id) {
        s32 count;
        R_TRY(this->Count(std::addressof(count)));

        /* Iterate over each entry. */
        for (s32 i = 0; i < count; i++) {
            InstallContentMeta content_meta;
            R_TRY(this->Get(std::addressof(content_meta), i));

            /* If the id matches we are successful. */
            if (content_meta.GetReader().GetKey().id == id) {
                *out = true;
                return ResultSuccess();
            }
        }

        /* We didn't find the value. */
        *out = false;
        return ResultSuccess();
    }

    Result FileInstallTaskData::Create(const char *path, s32 max_entries) {
        /* Create the file. */
        R_TRY(fs::CreateFile(path, 0));

        /* Open the file. */
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), path, fs::OpenMode_Write | fs::OpenMode_AllowAppend));
        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Create an initial header and write it to the file. */
        const Header header = MakeInitialHeader(max_entries);
        return fs::WriteFile(file, 0, std::addressof(header), sizeof(Header), fs::WriteOption::Flush);
    }

    Result FileInstallTaskData::Initialize(const char *path) {
        strncpy(this->path, path, sizeof(this->path));
        this->path[sizeof(this->path) - 1] = '\x00';
        return this->Read(std::addressof(this->header), sizeof(Header), 0);
    }

    Result FileInstallTaskData::Read(void *out, size_t out_size, s64 offset) {
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), this->path, fs::OpenMode_Read));
        ON_SCOPE_EXIT { fs::CloseFile(file); };
        return fs::ReadFile(file, offset, out, out_size);
    }

    Result FileInstallTaskData::GetProgress(InstallProgress *out_progress) {
        /* Initialize install progress. */
        InstallProgress install_progress = {
            .state = this->header.progress_state,
        };
        install_progress.SetLastResult(this->header.last_result);

        /* Only states after prepared are allowed. */
        if (this->header.progress_state != InstallProgressState::NotPrepared && this->header.progress_state != InstallProgressState::DataPrepared) {
            for (s32 i = 0; i < this->header.count; i++) {
                /* Obtain the content meta for this entry. */
                InstallContentMeta content_meta;
                R_TRY(InstallTaskDataBase::Get(std::addressof(content_meta), i));
                const InstallContentMetaReader reader = content_meta.GetReader();

                /* Sum the sizes from this entry's content infos. */
                for (size_t j = 0; j < reader.GetContentCount(); j++) {
                    const InstallContentInfo *content_info = reader.GetContentInfo(j);
                    install_progress.installed_size += content_info->GetSize();
                    install_progress.total_size += content_info->GetSizeWritten();
                }
            }
        }

        *out_progress = install_progress;
        return ResultSuccess();
    }

    Result FileInstallTaskData::GetEntryInfo(EntryInfo *out_entry_info, s32 index) {
        AMS_ABORT_UNLESS(index < this->header.count);
        return this->Read(out_entry_info, sizeof(EntryInfo), GetEntryInfoOffset(index));
    }

    Result FileInstallTaskData::GetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo *out_info) {
        *out_info = this->header.system_update_task_apply_info;
        return ResultSuccess();
    }

    Result FileInstallTaskData::SetState(InstallProgressState state) {
        this->header.progress_state = state;
        return this->WriteHeader();
    }

    Result FileInstallTaskData::WriteHeader() {
        return this->Write(std::addressof(this->header), sizeof(Header), 0);
    }

    Result FileInstallTaskData::SetLastResult(Result result) {
        this->header.last_result = result;
        return this->WriteHeader();
    }

    Result FileInstallTaskData::SetSystemUpdateTaskApplyInfo(SystemUpdateTaskApplyInfo info) {
        this->header.system_update_task_apply_info = info;
        return this->WriteHeader();
    }

    Result FileInstallTaskData::Push(const void *data, size_t data_size) {
        R_UNLESS(this->header.count < this->header.max_entries, ncm::ResultBufferInsufficient());

        /* Create a new entry info. Data of the given size will be stored at the end of the file. */
        const EntryInfo entry_info = { this->header.last_data_offset, data_size };

        /* Write the new entry info. */
        R_TRY(this->Write(std::addressof(entry_info), sizeof(EntryInfo), GetEntryInfoOffset(this->header.count)));

        /* Write the data to the offset in the entry info. */
        R_TRY(this->Write(data, data_size, entry_info.offset));

        /* Update the header for the new entry. */
        this->header.last_data_offset += data_size;
        this->header.count++;

        /* Write the updated header. */
        return this->WriteHeader();
    }

    Result FileInstallTaskData::Write(const void *data, size_t size, s64 offset) {
        fs::FileHandle file;
        R_TRY(fs::OpenFile(std::addressof(file), this->path, fs::OpenMode_Write | fs::OpenMode_Append));
        ON_SCOPE_EXIT { fs::CloseFile(file); };
        return fs::WriteFile(file, offset, data, size, fs::WriteOption::Flush);
    }

    Result FileInstallTaskData::Count(s32 *out) {
        *out = this->header.count;
        return ResultSuccess();
    }

    Result FileInstallTaskData::GetSize(size_t *out_size, s32 index) {
        EntryInfo entry_info;
        R_TRY(this->GetEntryInfo(std::addressof(entry_info), index));
        *out_size = entry_info.size;
        return ResultSuccess();
    }

    Result FileInstallTaskData::Get(s32 index, void *out, size_t out_size) {
        /* Obtain the entry info. */
        EntryInfo entry_info;
        R_TRY(this->GetEntryInfo(std::addressof(entry_info), index));

        /* Read the entry to the output buffer. */
        R_UNLESS(entry_info.size <= out_size, ncm::ResultBufferInsufficient());
        return this->Read(out, out_size, entry_info.offset);
    }

    Result FileInstallTaskData::Update(s32 index, const void *data, size_t data_size) {
        /* Obtain the entry info. */
        EntryInfo entry_info;
        R_TRY(this->GetEntryInfo(std::addressof(entry_info), index));

        /* Data size must match existing data size. */
        R_UNLESS(entry_info.size == data_size, ncm::ResultBufferInsufficient());
        return this->Write(data, data_size, entry_info.offset);
    }

    Result FileInstallTaskData::Delete(const ContentMetaKey *keys, s32 num_keys) {
        /* Create the path for the temporary data. */
        BoundedPath tmp_path(this->path);
        tmp_path.Append(".tmp");

        /* Create a new temporary install task data. */
        FileInstallTaskData install_task_data;
        R_TRY(FileInstallTaskData::Create(tmp_path, this->header.max_entries));
        R_TRY(install_task_data.Initialize(tmp_path));

        /* Get the number of entries. */
        s32 count;
        R_TRY(this->Count(std::addressof(count)));

        /* Copy entries that are not excluded to the new install task data. */
        for (s32 i = 0; i < count; i++) {
            InstallContentMeta content_meta;
            R_TRY(InstallTaskDataBase::Get(std::addressof(content_meta), i));

            /* Check if entry is excluded. If not, push it to our new install task data. */
            if (Includes(keys, num_keys, content_meta.GetReader().GetKey())) {
                continue;
            }

            /* NOTE: Nintendo doesn't check that this operation succeeds. */
            install_task_data.Push(content_meta.data.get(), content_meta.size);
        }

        /* Change from our current data to the new data. */
        this->header = install_task_data.header;
        R_TRY(fs::DeleteFile(this->path));
        return fs::RenameFile(tmp_path, this->path);
    }

    Result FileInstallTaskData::Cleanup() {
        this->header = MakeInitialHeader(this->header.max_entries);
        return this->WriteHeader();
    }

}