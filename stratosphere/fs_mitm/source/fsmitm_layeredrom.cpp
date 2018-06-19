#include <switch.h>
#include <stratosphere.hpp>

#include "fsmitm_layeredrom.hpp"
#include "fsmitm_utils.hpp"
#include "debug.hpp"


LayeredRomFS::LayeredRomFS(std::shared_ptr<RomInterfaceStorage> s_r, std::shared_ptr<RomFileStorage> f_r, u64 tid) : storage_romfs(s_r), file_romfs(f_r), title_id(tid) {
    /* Start building the new virtual romfs. */
    RomFSBuildContext build_ctx(this->title_id);
    this->p_source_infos = std::make_shared<std::vector<RomFSSourceInfo>>();
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
    /* Validate size. */
    u64 virt_size = (*this->p_source_infos)[this->p_source_infos->size() - 1].virtual_offset + (*this->p_source_infos)[this->p_source_infos->size() - 1].size;
    if (offset >= virt_size) {
        return 0x2F5A02;
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
            auto source_info_visitor = [&](auto& info) -> Result {
                Result rc = 0;
                if constexpr (std::is_same_v<decltype(info), RomFSBaseSourceInfo>) {
                    FsFile file;
                    if (R_FAILED((rc = Utils::OpenRomFSSdFile(this->title_id, info.path, FS_OPEN_READ, &file)))) {
                        fatalSimple(rc);
                    }
                    size_t out_read;
                    if (R_FAILED((rc = fsFileRead(&file, (offset - cur_source->virtual_offset), (void *)((uintptr_t)buffer + read_so_far), cur_read_size, &out_read)))) {
                        fatalSimple(rc);
                    }
                    if (out_read != cur_read_size) {
                        Reboot();
                    }
                    fsFileClose(&file);
                } else if constexpr (std::is_same_v<decltype(info), RomFSFileSourceInfo>) {
                    memcpy((void *)((uintptr_t)buffer + read_so_far), info.data + (offset - cur_source->virtual_offset), cur_read_size);
                } else if constexpr (std::is_same_v<decltype(info), RomFSLooseSourceInfo>) {
                    if (R_FAILED((rc = this->storage_romfs->Read((void *)((uintptr_t)buffer + read_so_far), cur_read_size, info.offset + (offset - cur_source->virtual_offset))))) {
                        /* TODO: Can this ever happen? */
                        /* fatalSimple(rc); */
                        return rc;
                    }
                } else if constexpr (std::is_same_v<decltype(info), RomFSMemorySourceInfo>) {
                    if (R_FAILED((rc = this->file_romfs->Read((void *)((uintptr_t)buffer + read_so_far), cur_read_size, info.offset + (offset - cur_source->virtual_offset))))) {
                        fatalSimple(rc);
                    }
                }
                return rc;
            };
            Result rc = std::visit(source_info_visitor, cur_source->info);

            read_so_far += cur_read_size;
        } else {
            /* Handle padding explicitly. */
            cur_source_ind++;
            /* Zero out the padding we skip, here. */
            memset((void *)((uintptr_t)buffer + read_so_far), 0, ((*this->p_source_infos)[cur_source_ind]).virtual_offset - (cur_source->virtual_offset + cur_source->size));
            read_so_far += ((*this->p_source_infos)[cur_source_ind]).virtual_offset - (cur_source->virtual_offset + cur_source->size);
        }
    }

    return 0;
}
Result LayeredRomFS::GetSize(u64 *out_size)  {
    *out_size = (*this->p_source_infos)[this->p_source_infos->size() - 1].virtual_offset + (*this->p_source_infos)[this->p_source_infos->size() - 1].size;
    return 0x0;
}
Result LayeredRomFS::OperateRange(u32 operation_type, u64 offset, u64 size, FsRangeInfo *out_range_info) {
    /* TODO: How should I implement this for a virtual romfs? */
    if (operation_type == 3) {
        *out_range_info = {0};
    }
    return 0;
}
