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

#include <switch.h>
#include <stratosphere.hpp>

#include "fsmitm_layeredrom.hpp"
#include "../utils.hpp"
#include "../debug.hpp"

IStorage::~IStorage() = default;

LayeredRomFS::LayeredRomFS(std::shared_ptr<IROStorage> s_r, std::shared_ptr<IROStorage> f_r, u64 tid) : storage_romfs(s_r), file_romfs(f_r), title_id(tid) {
    /* Start building the new virtual romfs. */
    RomFSBuildContext build_ctx(this->title_id);
    this->p_source_infos = std::shared_ptr<std::vector<RomFSSourceInfo>>(new std::vector<RomFSSourceInfo>(), [](std::vector<RomFSSourceInfo> *to_delete) {
        for (unsigned int i = 0; i < to_delete->size(); i++) {
            (*to_delete)[i].Cleanup();
        }
        delete to_delete;
    });
    if (Utils::IsSdInitialized()) {
        build_ctx.MergeSdFiles();
    }
    if (this->file_romfs) {
        build_ctx.MergeRomStorage(this->file_romfs.get(), RomFSDataSource::FileRomFS);
    }
    if (this->storage_romfs) {
        build_ctx.MergeRomStorage(this->storage_romfs.get(), RomFSDataSource::BaseRomFS);
    }
    build_ctx.Build(this->p_source_infos.get());
}


Result LayeredRomFS::Read(void *buffer, size_t size, u64 offset)  {
    /* Size zero reads should always succeed. */
    if (size == 0) {
        return ResultSuccess();
    }

    /* Validate size. */
    u64 virt_size = (*this->p_source_infos)[this->p_source_infos->size() - 1].virtual_offset + (*this->p_source_infos)[this->p_source_infos->size() - 1].size;
    if (offset >= virt_size) {
        return ResultFsInvalidOffset;
    }
    if (virt_size - offset < size) {
        size = virt_size - offset;
    }
    /* Find first source info via binary search. */
    u32 cur_source_ind = 0;
    u32 low = 0, high = this->p_source_infos->size() - 1;
    while (low <= high) {
        u32 mid = (low + high) / 2;
        if ((*this->p_source_infos)[mid].virtual_offset > offset) {
            /* Too high. */
            high = mid - 1;
        } else {
            /* sources[mid].virtual_offset <= offset, invariant */
            if (mid == this->p_source_infos->size() - 1 || (*this->p_source_infos)[mid + 1].virtual_offset > offset) {
                /* Success */
                cur_source_ind = mid;
                break;
            }
            low = mid + 1;
        }
    }

    size_t read_so_far = 0;
    while (read_so_far < size) {
        RomFSSourceInfo *cur_source = &((*this->p_source_infos)[cur_source_ind]);
        if (cur_source->virtual_offset + cur_source->size > offset) {
            u64 cur_read_size = size - read_so_far;
            if (cur_read_size > cur_source->size - (offset - cur_source->virtual_offset)) {
                cur_read_size = cur_source->size - (offset - cur_source->virtual_offset);
            }
            switch (cur_source->type) {
                case RomFSDataSource::MetaData:
                    {
                        FsFile file;
                        R_ASSERT(Utils::OpenSdFileForAtmosphere(this->title_id, ROMFS_METADATA_FILE_PATH, FS_OPEN_READ, &file));
                        size_t out_read;
                        R_ASSERT(fsFileRead(&file, (offset - cur_source->virtual_offset), (void *)((uintptr_t)buffer + read_so_far), cur_read_size, FS_READOPTION_NONE, &out_read));
                        AMS_ASSERT(out_read == cur_read_size);
                        fsFileClose(&file);
                    }
                    break;
                case RomFSDataSource::LooseFile:
                    {
                        FsFile file;
                        R_ASSERT(Utils::OpenRomFSSdFile(this->title_id, cur_source->loose_source_info.path, FS_OPEN_READ, &file));
                        size_t out_read;
                        R_ASSERT(fsFileRead(&file, (offset - cur_source->virtual_offset), (void *)((uintptr_t)buffer + read_so_far), cur_read_size, FS_READOPTION_NONE, &out_read));
                        AMS_ASSERT(out_read == cur_read_size);
                        fsFileClose(&file);
                    }
                    break;
                case RomFSDataSource::Memory:
                    {
                        memcpy((void *)((uintptr_t)buffer + read_so_far), cur_source->memory_source_info.data + (offset - cur_source->virtual_offset), cur_read_size);
                    }
                    break;
                case RomFSDataSource::BaseRomFS:
                    {
                        R_ASSERT(this->storage_romfs->Read((void *)((uintptr_t)buffer + read_so_far), cur_read_size, cur_source->base_source_info.offset + (offset - cur_source->virtual_offset)));
                    }
                    break;
                case RomFSDataSource::FileRomFS:
                    {
                        R_ASSERT(this->file_romfs->Read((void *)((uintptr_t)buffer + read_so_far), cur_read_size, cur_source->base_source_info.offset + (offset - cur_source->virtual_offset)));
                    }
                    break;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
            read_so_far += cur_read_size;
            offset += cur_read_size;
        } else {
            /* Handle padding explicitly. */
            cur_source_ind++;
            /* Zero out the padding we skip, here. */
            memset((void *)((uintptr_t)buffer + read_so_far), 0, ((*this->p_source_infos)[cur_source_ind]).virtual_offset - offset);
            read_so_far += ((*this->p_source_infos)[cur_source_ind]).virtual_offset - offset;
            offset = ((*this->p_source_infos)[cur_source_ind]).virtual_offset;
        }
    }

    return ResultSuccess();
}
Result LayeredRomFS::GetSize(u64 *out_size)  {
    *out_size = (*this->p_source_infos)[this->p_source_infos->size() - 1].virtual_offset + (*this->p_source_infos)[this->p_source_infos->size() - 1].size;
    return ResultSuccess();
}
Result LayeredRomFS::OperateRange(FsOperationId operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) {
    /* TODO: How should I implement this for a virtual romfs? */
    if (operation_type == FsOperationId_QueryRange) {
        *out_range_info = {0};
    }
    return ResultSuccess();
}
