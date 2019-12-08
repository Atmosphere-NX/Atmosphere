/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "../amsmitm_initialization.hpp"
#include "../amsmitm_fs_utils.hpp"
#include "fsmitm_layered_romfs_storage.hpp"

namespace ams::mitm::fs {

    using namespace ams::fs;

    LayeredRomfsStorage::LayeredRomfsStorage(std::unique_ptr<IStorage> s_r, std::unique_ptr<IStorage> f_r, ncm::ProgramId pr_id) : storage_romfs(std::move(s_r)), file_romfs(std::move(f_r)), program_id(std::move(pr_id)) {
        /* Build new virtual romfs. */
        romfs::Builder builder(this->program_id);

        if (mitm::IsInitialized()) {
            builder.AddSdFiles();
        }
        if (this->file_romfs) {
            builder.AddStorageFiles(this->file_romfs.get(), romfs::DataSourceType::File);
        }
        if (this->storage_romfs) {
            builder.AddStorageFiles(this->storage_romfs.get(), romfs::DataSourceType::Storage);
        }

        builder.Build(&this->source_infos);
    }

    LayeredRomfsStorage::~LayeredRomfsStorage() {
        for (size_t i = 0; i < this->source_infos.size(); i++) {
            this->source_infos[i].Cleanup();
        }
    }

    Result LayeredRomfsStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Check if we can succeed immediately. */
        R_UNLESS(size >= 0, fs::ResultInvalidSize());
        R_UNLESS(size > 0,  ResultSuccess());


        /* Validate offset/size. */
        const s64 virt_size = this->GetSize();
        R_UNLESS(offset >= 0,        fs::ResultInvalidOffset());
        R_UNLESS(offset < virt_size, fs::ResultInvalidOffset());
        if (size_t(virt_size - offset) < size) {
            size = size_t(virt_size - offset);
        }

        /* Find first source info via binary search. */
        auto it = std::lower_bound(this->source_infos.begin(), this->source_infos.end(), offset);
        u8 *cur_dst = static_cast<u8 *>(buffer);

        /* Our operator < compares against start of info instead of end, so we need to subtract one from lower bound. */
        it--;

        size_t read_so_far = 0;
        while (read_so_far < size) {
            const auto &cur_source = *it;
            AMS_ASSERT(offset >= cur_source.virtual_offset);

            if (offset < cur_source.virtual_offset + cur_source.size) {
                const s64 offset_within_source = offset - cur_source.virtual_offset;
                const size_t cur_read_size = std::min(size - read_so_far, size_t(cur_source.size - offset_within_source));
                switch (cur_source.source_type) {
                    case romfs::DataSourceType::Storage:
                        R_ASSERT(this->storage_romfs->Read(cur_source.storage_source_info.offset + offset_within_source, cur_dst, cur_read_size));
                        break;
                    case romfs::DataSourceType::File:
                        R_ASSERT(this->file_romfs->Read(cur_source.file_source_info.offset + offset_within_source, cur_dst, cur_read_size));
                        break;
                    case romfs::DataSourceType::LooseSdFile:
                        {
                            FsFile file;
                            R_ASSERT(mitm::fs::OpenAtmosphereSdRomfsFile(&file, this->program_id, cur_source.loose_source_info.path, OpenMode_Read));
                            ON_SCOPE_EXIT { fsFileClose(&file); };

                            u64 out_read = 0;
                            R_ASSERT(fsFileRead(&file, offset_within_source, cur_dst, cur_read_size, FsReadOption_None, &out_read));
                            AMS_ASSERT(out_read == cur_read_size);
                        }
                        break;
                    case romfs::DataSourceType::Memory:
                        std::memcpy(cur_dst, cur_source.memory_source_info.data + offset_within_source, cur_read_size);
                        break;
                    case romfs::DataSourceType::Metadata:
                        {
                            size_t out_read = 0;
                            R_ASSERT(cur_source.metadata_source_info.file->Read(&out_read, offset_within_source, cur_dst, cur_read_size));
                            AMS_ASSERT(out_read == cur_read_size);
                        }
                        break;
                    AMS_UNREACHABLE_DEFAULT_CASE();
                }
                read_so_far += cur_read_size;
                cur_dst     += cur_read_size;
                offset      += cur_read_size;
            } else {
                /* Explicitly handle padding. */
                const auto &next_source = *(++it);
                const size_t padding_size = size_t(next_source.virtual_offset - offset);

                std::memset(cur_dst, 0, padding_size);
                read_so_far += padding_size;
                cur_dst     += padding_size;
                offset      += padding_size;
            }
        }

        return ResultSuccess();
    }

    Result LayeredRomfsStorage::GetSize(s64 *out_size) {
        *out_size = this->GetSize();
        return ResultSuccess();
    }

    Result LayeredRomfsStorage::Flush() {
        return ResultSuccess();
    }

    Result LayeredRomfsStorage::OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        switch (op_id) {
            case OperationId::InvalidateCache:
            case OperationId::QueryRange:
                if (size == 0) {
                    if (op_id == OperationId::QueryRange) {
                        R_UNLESS(dst != nullptr,                     fs::ResultNullptrArgument());
                        R_UNLESS(dst_size == sizeof(QueryRangeInfo), fs::ResultInvalidSize());
                        reinterpret_cast<QueryRangeInfo *>(dst)->Clear();
                    }
                    return ResultSuccess();
                }
                /* TODO: How to deal with this? */
                return fs::ResultUnsupportedOperation();
            default:
                return fs::ResultUnsupportedOperation();
        }
    }


}
