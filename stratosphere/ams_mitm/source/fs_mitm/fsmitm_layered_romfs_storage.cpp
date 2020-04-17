/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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

    namespace {

        os::Mutex g_mq_lock(false);
        bool g_started_req_thread;
        uintptr_t g_mq_storage[2];
        os::MessageQueue g_req_mq(g_mq_storage + 0, 1);
        os::MessageQueue g_ack_mq(g_mq_storage + 1, 1);

        void RomfsInitializerThreadFunction(void *arg) {
            while (true) {
                uintptr_t storage_uptr = 0;
                g_req_mq.Receive(&storage_uptr);
                std::shared_ptr<LayeredRomfsStorage> layered_storage = reinterpret_cast<LayeredRomfsStorage *>(storage_uptr)->GetShared();
                g_ack_mq.Send(storage_uptr);
                layered_storage->InitializeImpl();
            }
        }

        constexpr size_t RomfsInitializerThreadStackSize = 0x8000;
        os::ThreadType g_romfs_initializer_thread;
        alignas(os::ThreadStackAlignment) u8 g_romfs_initializer_thread_stack[RomfsInitializerThreadStackSize];

        void RequestInitializeStorage(uintptr_t storage_uptr) {
            std::scoped_lock lk(g_mq_lock);

            if (AMS_UNLIKELY(!g_started_req_thread)) {
                R_ABORT_UNLESS(os::CreateThread(std::addressof(g_romfs_initializer_thread), RomfsInitializerThreadFunction, nullptr, g_romfs_initializer_thread_stack, sizeof(g_romfs_initializer_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(mitm_fs, RomFileSystemInitializeThread)));
                os::SetThreadNamePointer(std::addressof(g_romfs_initializer_thread), AMS_GET_SYSTEM_THREAD_NAME(mitm_fs, RomFileSystemInitializeThread));
                os::StartThread(std::addressof(g_romfs_initializer_thread));
                g_started_req_thread = true;
            }

            g_req_mq.Send(storage_uptr);
            uintptr_t ack = 0;
            g_ack_mq.Receive(&ack);
            AMS_ABORT_UNLESS(ack == storage_uptr);
        }

    }

    using namespace ams::fs;

    LayeredRomfsStorage::LayeredRomfsStorage(std::unique_ptr<IStorage> s_r, std::unique_ptr<IStorage> f_r, ncm::ProgramId pr_id) : storage_romfs(std::move(s_r)), file_romfs(std::move(f_r)), initialize_event(os::EventClearMode_ManualClear), program_id(std::move(pr_id)), is_initialized(false), started_initialize(false) {
        /* ... */
    }

    LayeredRomfsStorage::~LayeredRomfsStorage() {
        for (size_t i = 0; i < this->source_infos.size(); i++) {
            this->source_infos[i].Cleanup();
        }
    }

    void LayeredRomfsStorage::BeginInitialize() {
        AMS_ABORT_UNLESS(!this->started_initialize);
        RequestInitializeStorage(reinterpret_cast<uintptr_t>(this));
        this->started_initialize = true;
    }

    void LayeredRomfsStorage::InitializeImpl() {
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

        this->is_initialized = true;
        this->initialize_event.Signal();
    }

    Result LayeredRomfsStorage::Read(s64 offset, void *buffer, size_t size) {
        /* Check if we can succeed immediately. */
        R_UNLESS(size >= 0, fs::ResultInvalidSize());
        R_SUCCEED_IF(size == 0);

        /* Ensure we're initialized. */
        if (!this->is_initialized) {
            this->initialize_event.Wait();
        }

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
            AMS_ABORT_UNLESS(offset >= cur_source.virtual_offset);

            if (offset < cur_source.virtual_offset + cur_source.size) {
                const s64 offset_within_source = offset - cur_source.virtual_offset;
                const size_t cur_read_size = std::min(size - read_so_far, size_t(cur_source.size - offset_within_source));
                switch (cur_source.source_type) {
                    case romfs::DataSourceType::Storage:
                        R_ABORT_UNLESS(this->storage_romfs->Read(cur_source.storage_source_info.offset + offset_within_source, cur_dst, cur_read_size));
                        break;
                    case romfs::DataSourceType::File:
                        R_ABORT_UNLESS(this->file_romfs->Read(cur_source.file_source_info.offset + offset_within_source, cur_dst, cur_read_size));
                        break;
                    case romfs::DataSourceType::LooseSdFile:
                        {
                            FsFile file;
                            R_ABORT_UNLESS(mitm::fs::OpenAtmosphereSdRomfsFile(&file, this->program_id, cur_source.loose_source_info.path, OpenMode_Read));
                            ON_SCOPE_EXIT { fsFileClose(&file); };

                            u64 out_read = 0;
                            R_ABORT_UNLESS(fsFileRead(&file, offset_within_source, cur_dst, cur_read_size, FsReadOption_None, &out_read));
                            AMS_ABORT_UNLESS(out_read == cur_read_size);
                        }
                        break;
                    case romfs::DataSourceType::Memory:
                        std::memcpy(cur_dst, cur_source.memory_source_info.data + offset_within_source, cur_read_size);
                        break;
                    case romfs::DataSourceType::Metadata:
                        {
                            size_t out_read = 0;
                            R_ABORT_UNLESS(cur_source.metadata_source_info.file->Read(&out_read, offset_within_source, cur_dst, cur_read_size));
                            AMS_ABORT_UNLESS(out_read == cur_read_size);
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
        /* Ensure we're initialized. */
        if (!this->is_initialized) {
            this->initialize_event.Wait();
        }

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
