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
#include "../amsmitm_initialization.hpp"
#include "../amsmitm_fs_utils.hpp"
#include "fsmitm_layered_romfs_storage.hpp"

namespace ams::mitm::fs {

    namespace {

        constinit os::SdkMutex g_mq_lock;
        constinit bool g_started_req_thread;
        constinit uintptr_t g_mq_storage[2];
        os::MessageQueue g_req_mq(g_mq_storage + 0, 1);
        os::MessageQueue g_ack_mq(g_mq_storage + 1, 1);

        class LayeredRomfsStorageHolder : public util::IntrusiveRedBlackTreeBaseNode<LayeredRomfsStorageHolder> {
            public:
                using RedBlackKeyType = u64;
            private:
                LayeredRomfsStorageImpl *m_impl;
                u32 m_reference_count;
                bool m_second_chance;
                bool m_process_romfs;
            public:
                LayeredRomfsStorageHolder(LayeredRomfsStorageImpl *impl, bool process_rom) : m_impl(impl), m_reference_count(1), m_second_chance(true), m_process_romfs(process_rom) {
                    /* ... */
                }

                ~LayeredRomfsStorageHolder() {
                    delete m_impl;
                }

                constexpr LayeredRomfsStorageImpl *GetImpl() const { return m_impl; }
                constexpr ncm::ProgramId GetProgramId() const { return m_impl->GetProgramId(); }
                constexpr u32 GetReferenceCount() const { return m_reference_count; }

                void OpenReferenceImpl() { ++m_reference_count; }
                void CloseReferenceImpl() { --m_reference_count; }

                bool GetSecondChanceImpl() const { return m_second_chance; }
                void SetSecondChanceImpl(bool sc) { m_second_chance = sc; }

                bool IsProcessRomfs() const { return m_process_romfs; }

                static constexpr ALWAYS_INLINE int Compare(const RedBlackKeyType &lval, const LayeredRomfsStorageHolder &rhs) {
                    const auto rval = rhs.GetProgramId().value;

                    if (lval < rval) {
                        return -1;
                    } else if (lval == rval) {
                        return 0;
                    } else {
                        return 1;
                    }
                }

                static constexpr ALWAYS_INLINE int Compare(const LayeredRomfsStorageHolder &lhs, const LayeredRomfsStorageHolder &rhs) {
                    return Compare(lhs.GetProgramId().value, rhs);
                }
        };

        using LayeredRomfsStorageSet = typename util::IntrusiveRedBlackTreeBaseTraits<LayeredRomfsStorageHolder>::TreeType<LayeredRomfsStorageHolder>;

        constinit os::SdkRecursiveMutex g_storage_set_mutex;
        constinit LayeredRomfsStorageSet g_storage_set;
        constinit os::SdkMutex g_initialization_mutex;

        void OpenReference(LayeredRomfsStorageImpl *impl) {
            std::scoped_lock lk(g_storage_set_mutex);

            auto it = g_storage_set.find_key(impl->GetProgramId().value);
            AMS_ABORT_UNLESS(it != g_storage_set.end());

            it->OpenReferenceImpl();
        }

        void CloseReference(LayeredRomfsStorageImpl *impl) {
            std::scoped_lock lk(g_storage_set_mutex);

            auto it = g_storage_set.find_key(impl->GetProgramId().value);
            AMS_ABORT_UNLESS(it != g_storage_set.end());

            AMS_ABORT_UNLESS(it->GetReferenceCount() > 0);
            it->CloseReferenceImpl();
        }

        void RomfsInitializerThreadFunction(void *) {
            while (true) {
                uintptr_t storage_uptr = 0;

                g_req_mq.Receive(std::addressof(storage_uptr));
                auto *impl = reinterpret_cast<LayeredRomfsStorageImpl *>(storage_uptr);
                g_ack_mq.Send(storage_uptr);

                std::scoped_lock lk(g_initialization_mutex);

                impl->InitializeImpl();

                /* Close the initial reference. */
                CloseReference(impl);
            }
        }

        void RomfsFinalizerThreadFunction(void *) {
            while (true) {
                {
                    std::scoped_lock lk(g_storage_set_mutex);

                    auto it = g_storage_set.begin();
                    while (it != g_storage_set.end()) {
                        if (it->GetReferenceCount() > 0) {
                            it->SetSecondChanceImpl(true);
                            ++it;
                        } else if (it->GetSecondChanceImpl()) {
                            it->SetSecondChanceImpl(false);
                            ++it;
                        } else {
                            auto *holder = std::addressof(*it);
                            it = g_storage_set.erase(it);
                            delete holder;
                        }
                    }
                }

                os::SleepThread(TimeSpan::FromMilliSeconds(500));
            }
        }

        constexpr size_t RomfsInitializerThreadStackSize = 0x8000;
        os::ThreadType g_romfs_initializer_thread;
        os::ThreadType g_romfs_finalizer_thread;
        alignas(os::ThreadStackAlignment) u8 g_romfs_initializer_thread_stack[RomfsInitializerThreadStackSize];
        alignas(os::ThreadStackAlignment) u8 g_romfs_finalizer_thread_stack[os::MemoryPageSize];

        void RequestInitializeStorage(uintptr_t storage_uptr) {
            std::scoped_lock lk(g_mq_lock);

            if (AMS_UNLIKELY(!g_started_req_thread)) {
                R_ABORT_UNLESS(os::CreateThread(std::addressof(g_romfs_initializer_thread), RomfsInitializerThreadFunction, nullptr, g_romfs_initializer_thread_stack, sizeof(g_romfs_initializer_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(mitm_fs, RomFileSystemInitializeThread)));
                os::SetThreadNamePointer(std::addressof(g_romfs_initializer_thread), AMS_GET_SYSTEM_THREAD_NAME(mitm_fs, RomFileSystemInitializeThread));
                os::StartThread(std::addressof(g_romfs_initializer_thread));

                R_ABORT_UNLESS(os::CreateThread(std::addressof(g_romfs_finalizer_thread), RomfsFinalizerThreadFunction, nullptr, g_romfs_finalizer_thread_stack, sizeof(g_romfs_finalizer_thread_stack), AMS_GET_SYSTEM_THREAD_PRIORITY(mitm_fs, RomFileSystemInitializeThread)));
                os::SetThreadNamePointer(std::addressof(g_romfs_finalizer_thread), AMS_GET_SYSTEM_THREAD_NAME(mitm_fs, RomFileSystemFinalizeThread));
                os::StartThread(std::addressof(g_romfs_finalizer_thread));

                g_started_req_thread = true;
            }

            g_req_mq.Send(storage_uptr);
            uintptr_t ack = 0;
            g_ack_mq.Receive(std::addressof(ack));
            AMS_ABORT_UNLESS(ack == storage_uptr);
        }

        class LayeredRomfsStorage : public ams::fs::IStorage {
            private:
                LayeredRomfsStorageImpl *m_impl;
            public:
                LayeredRomfsStorage(LayeredRomfsStorageImpl *impl) : m_impl(impl) {
                    OpenReference(m_impl);
                }

                virtual ~LayeredRomfsStorage() {
                    CloseReference(m_impl);
                }


                virtual Result Read(s64 offset, void *buffer, size_t size) override {
                    R_RETURN(m_impl->Read(offset, buffer, size));
                }

                virtual Result GetSize(s64 *out_size) override {
                    R_RETURN(m_impl->GetSize(out_size));
                }

                virtual Result Flush() override {
                    R_RETURN(m_impl->Flush());
                }

                virtual Result OperateRange(void *dst, size_t dst_size, ams::fs::OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) override {
                    R_RETURN(m_impl->OperateRange(dst, dst_size, op_id, offset, size, src, src_size));
                }

                virtual Result Write(s64 offset, const void *buffer, size_t size) override {
                    /* TODO: Better result code? */
                    AMS_UNUSED(offset, buffer, size);
                    R_THROW(ams::fs::ResultUnsupportedOperation())
                }

                virtual Result SetSize(s64 size) override {
                    /* TODO: Better result code? */
                    AMS_UNUSED(size);
                    R_THROW(ams::fs::ResultUnsupportedOperation())
                }
        };

    }

    using namespace ams::fs;

    std::shared_ptr<ams::fs::IStorage> GetLayeredRomfsStorage(ncm::ProgramId program_id, ::FsStorage &data_storage, bool is_process_romfs) {
        /*Prepare to find or create a new storage. */
        LayeredRomfsStorageImpl *impl = nullptr;
        {
            std::scoped_lock lk(g_storage_set_mutex);

            /* Find an existing storage. */
            if (auto it = g_storage_set.find_key(program_id.value); it != g_storage_set.end()) {
                return std::make_shared<LayeredRomfsStorage>(it->GetImpl());
            }

            /* We don't have an existing storage. If we're creating process romfs, free any unreferenced process romfs. */
            /* This should help prevent too much memory in use at any time. */
            if (is_process_romfs) {
                auto it = g_storage_set.begin();
                while (it != g_storage_set.end()) {
                    if (it->GetReferenceCount() > 0 || !it->IsProcessRomfs()) {
                        ++it;
                    } else {
                        auto *holder = std::addressof(*it);
                        it = g_storage_set.erase(it);
                        delete holder;
                    }
                }
            }

            /* Create a new storage. */
            {
                ::FsFile data_file;
                if (R_SUCCEEDED(OpenAtmosphereSdFile(std::addressof(data_file), program_id, "romfs.bin", OpenMode_Read))) {
                    impl = new LayeredRomfsStorageImpl(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), std::make_unique<ReadOnlyStorageAdapter>(new FileStorage(new RemoteFile(data_file))), program_id);
                } else {
                    impl = new LayeredRomfsStorageImpl(std::make_unique<ReadOnlyStorageAdapter>(new RemoteStorage(data_storage)), nullptr, program_id);
                }
            }

            /* Insert holder. Reference count will now be one. */
            g_storage_set.insert(*(new LayeredRomfsStorageHolder(impl, is_process_romfs)));
        }

        /* Begin initialization. When this finishes, a decref will occur. */
        AMS_ABORT_UNLESS(impl != nullptr);
        impl->BeginInitialize();

        /* Return a new shared storage for the impl. */
        return std::make_shared<LayeredRomfsStorage>(impl);
    }

    void FinalizeLayeredRomfsStorage(ncm::ProgramId program_id) {
        std::scoped_lock lk(g_initialization_mutex);
        std::scoped_lock lk2(g_storage_set_mutex);

        /* Find an existing storage. */
        if (auto it = g_storage_set.find_key(program_id.value); it != g_storage_set.end()) {
            /* We need to delete the process romfs. Require invariant that it is unreferenced, by this point. */
            AMS_ABORT_UNLESS(it->GetReferenceCount() == 0);

            auto *holder = std::addressof(*it);
            it = g_storage_set.erase(it);
            delete holder;
        }
    }

    LayeredRomfsStorageImpl::LayeredRomfsStorageImpl(std::unique_ptr<IStorage> s_r, std::unique_ptr<IStorage> f_r, ncm::ProgramId pr_id) : m_storage_romfs(std::move(s_r)), m_file_romfs(std::move(f_r)), m_initialize_event(os::EventClearMode_ManualClear), m_program_id(std::move(pr_id)), m_is_initialized(false), m_started_initialize(false) {
        /* ... */
    }

    LayeredRomfsStorageImpl::~LayeredRomfsStorageImpl() {
        for (size_t i = 0; i < m_source_infos.size(); i++) {
            m_source_infos[i].Cleanup();
        }
    }

    void LayeredRomfsStorageImpl::BeginInitialize() {
        AMS_ABORT_UNLESS(!m_started_initialize);
        RequestInitializeStorage(reinterpret_cast<uintptr_t>(this));
        m_started_initialize = true;
    }

    void LayeredRomfsStorageImpl::InitializeImpl() {
        /* Build new virtual romfs. */
        romfs::Builder builder(m_program_id);

        if (mitm::IsInitialized()) {
            builder.AddSdFiles();
        }
        if (m_file_romfs) {
            builder.AddStorageFiles(m_file_romfs.get(), romfs::DataSourceType::File);
        }
        if (m_storage_romfs) {
            builder.AddStorageFiles(m_storage_romfs.get(), romfs::DataSourceType::Storage);
        }

        builder.Build(std::addressof(m_source_infos));

        m_is_initialized = true;
        m_initialize_event.Signal();
    }

    Result LayeredRomfsStorageImpl::Read(s64 offset, void *buffer, size_t size) {
        /* Check if we can succeed immediately. */
        R_SUCCEED_IF(size == 0);

        /* Ensure we're initialized. */
        if (!m_is_initialized) {
            m_initialize_event.Wait();
        }

        /* Validate offset/size. */
        const s64 virt_size = this->GetSize();
        R_UNLESS(offset >= 0,        fs::ResultInvalidOffset());
        R_UNLESS(offset < virt_size, fs::ResultInvalidOffset());
        if (static_cast<size_t>(virt_size - offset) < size) {
            size = static_cast<size_t>(virt_size - offset);
        }

        /* Find first source info via binary search. */
        auto it = std::lower_bound(m_source_infos.begin(), m_source_infos.end(), offset);
        u8 *cur_dst = static_cast<u8 *>(buffer);

        /* Our operator < compares against start of info instead of end, so we need to subtract one from lower bound. */
        it--;

        size_t read_so_far = 0;
        while (read_so_far < size) {
            const auto &cur_source = *it;
            AMS_ABORT_UNLESS(offset >= cur_source.virtual_offset);

            if (offset < cur_source.virtual_offset + cur_source.size) {
                const s64 offset_within_source = offset - cur_source.virtual_offset;
                const size_t cur_read_size = std::min(size - read_so_far, static_cast<size_t>(cur_source.size - offset_within_source));
                switch (cur_source.source_type) {
                    case romfs::DataSourceType::Storage:
                        R_ABORT_UNLESS(m_storage_romfs->Read(cur_source.storage_source_info.offset + offset_within_source, cur_dst, cur_read_size));
                        break;
                    case romfs::DataSourceType::File:
                        R_ABORT_UNLESS(m_file_romfs->Read(cur_source.file_source_info.offset + offset_within_source, cur_dst, cur_read_size));
                        break;
                    case romfs::DataSourceType::LooseSdFile:
                        {
                            FsFile file;
                            R_ABORT_UNLESS(mitm::fs::OpenAtmosphereSdRomfsFile(std::addressof(file), m_program_id, cur_source.loose_source_info.path, OpenMode_Read));
                            ON_SCOPE_EXIT { fsFileClose(std::addressof(file)); };

                            u64 out_read = 0;
                            R_ABORT_UNLESS(fsFileRead(std::addressof(file), offset_within_source, cur_dst, cur_read_size, FsReadOption_None, std::addressof(out_read)));
                            AMS_ABORT_UNLESS(out_read == cur_read_size);
                        }
                        break;
                    case romfs::DataSourceType::Memory:
                        std::memcpy(cur_dst, cur_source.memory_source_info.data + offset_within_source, cur_read_size);
                        break;
                    case romfs::DataSourceType::Metadata:
                        {
                            size_t out_read = 0;
                            R_ABORT_UNLESS(cur_source.metadata_source_info.file->Read(std::addressof(out_read), offset_within_source, cur_dst, cur_read_size));
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
                const size_t padding_size = static_cast<size_t>(next_source.virtual_offset - offset);

                std::memset(cur_dst, 0, padding_size);
                read_so_far += padding_size;
                cur_dst     += padding_size;
                offset      += padding_size;
            }
        }

        R_SUCCEED();
    }

    Result LayeredRomfsStorageImpl::GetSize(s64 *out_size) {
        /* Ensure we're initialized. */
        if (!m_is_initialized) {
            m_initialize_event.Wait();
        }

        *out_size = this->GetSize();
        R_SUCCEED();
    }

    Result LayeredRomfsStorageImpl::Flush() {
        R_SUCCEED();
    }

    Result LayeredRomfsStorageImpl::OperateRange(void *dst, size_t dst_size, OperationId op_id, s64 offset, s64 size, const void *src, size_t src_size) {
        AMS_UNUSED(offset, src, src_size);

        switch (op_id) {
            case OperationId::Invalidate:
            case OperationId::QueryRange:
                if (size == 0) {
                    if (op_id == OperationId::QueryRange) {
                        R_UNLESS(dst != nullptr,                     fs::ResultNullptrArgument());
                        R_UNLESS(dst_size == sizeof(QueryRangeInfo), fs::ResultInvalidSize());
                        reinterpret_cast<QueryRangeInfo *>(dst)->Clear();
                    }
                    R_SUCCEED();
                }
                /* TODO: How to deal with this? */
                R_THROW(fs::ResultUnsupportedOperation());
            default:
                R_THROW(fs::ResultUnsupportedOperation());
        }
    }


}
