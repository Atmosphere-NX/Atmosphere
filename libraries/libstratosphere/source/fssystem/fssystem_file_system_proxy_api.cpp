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

namespace ams::fssystem {

    /* TODO: All of this should really be inside fs process, but ams.mitm wants it to. */
    /* How should we handle this? */

    namespace {

        /* Official FS has a 4.5 MB exp heap, a 6 MB buffer pool, an 8 MB device buffer manager heap, and a 14 MB buffer manager heap. */
        /* We don't need so much memory for ams.mitm (as we're servicing a much more limited context). */
        /* We'll give ourselves a 1.5 MB exp heap, a 1 MB buffer pool, a 1 MB device buffer manager heap, and a 1 MB buffer manager heap. */
        /* These numbers are 1 MB less than signed-system-partition safe FS in all pools. */
        constexpr size_t ExpHeapSize           = 1_MB + 512_KB;
        constexpr size_t BufferPoolSize        = 1_MB;
        constexpr size_t DeviceBufferSize      = 1_MB;
        constexpr size_t BufferManagerHeapSize = 1_MB;

        constexpr size_t MaxCacheCount = 1024;
        constexpr size_t BlockSize     = 16_KB;

        alignas(os::MemoryPageSize) u8 g_exp_heap_buffer[ExpHeapSize];
        lmem::HeapHandle g_exp_heap_handle = nullptr;
        fssrv::PeakCheckableMemoryResourceFromExpHeap g_exp_allocator(ExpHeapSize);

        void InitializeExpHeap() {
            if (g_exp_heap_handle == nullptr) {
                g_exp_heap_handle = lmem::CreateExpHeap(g_exp_heap_buffer, ExpHeapSize, lmem::CreateOption_ThreadSafe);
                AMS_ABORT_UNLESS(g_exp_heap_handle != nullptr);
                g_exp_allocator.SetHeapHandle(g_exp_heap_handle);
            }
        }

        void *AllocateForFileSystemProxy(size_t size) {
            AMS_ABORT_UNLESS(g_exp_heap_handle != nullptr);

            auto scoped_lock = g_exp_allocator.GetScopedLock();

            void *p = lmem::AllocateFromExpHeap(g_exp_heap_handle, size);
            g_exp_allocator.OnAllocate(p, size);
            return p;
        }

        void DeallocateForFileSystemProxy(void *p, size_t size) {
            AMS_ABORT_UNLESS(g_exp_heap_handle != nullptr);

            auto scoped_lock = g_exp_allocator.GetScopedLock();

            g_exp_allocator.OnDeallocate(p, size);
            lmem::FreeToExpHeap(g_exp_heap_handle, p);
        }

        alignas(os::MemoryPageSize) u8 g_device_buffer[DeviceBufferSize];

        alignas(os::MemoryPageSize) u8 g_buffer_pool[BufferPoolSize];
        TYPED_STORAGE(mem::StandardAllocator) g_buffer_allocator;
        TYPED_STORAGE(fssrv::MemoryResourceFromStandardAllocator) g_allocator;

        /* TODO: Nintendo uses os::SetMemoryHeapSize (svc::SetHeapSize) and os::AllocateMemoryBlock for the BufferManager heap. */
        /* It's unclear how we should handle this in ams.mitm (especially hoping to reuse some logic for fs reimpl). */
        /* Should we be doing the same(?) */
        TYPED_STORAGE(fssystem::FileSystemBufferManager) g_buffer_manager;
        alignas(os::MemoryPageSize) u8 g_buffer_manager_heap[BufferManagerHeapSize];

        /* FileSystem creators. */
        TYPED_STORAGE(fssrv::fscreator::RomFileSystemCreator)       g_rom_fs_creator;
        TYPED_STORAGE(fssrv::fscreator::PartitionFileSystemCreator) g_partition_fs_creator;
        TYPED_STORAGE(fssrv::fscreator::StorageOnNcaCreator)        g_storage_on_nca_creator;

        fssrv::fscreator::FileSystemCreatorInterfaces g_fs_creator_interfaces = {};

    }

    void InitializeForFileSystemProxy() {
        /* TODO FS-REIMPL: fssystem::RegisterServiceContext */

        /* TODO FS-REIMPL: spl::InitializeForFs(); */

        /* Determine whether we're prod or dev. */
        bool is_prod                         = !spl::IsDevelopment();
        bool is_development_function_enabled = spl::IsDevelopmentFunctionEnabled();

        /* Setup our crypto configuration. */
        SetUpKekAccessKeys(is_prod);

        /* Setup our heap. */
        InitializeExpHeap();

        /* Initialize buffer allocator. */
        new (GetPointer(g_buffer_allocator)) mem::StandardAllocator(g_buffer_pool, BufferPoolSize);
        new (GetPointer(g_allocator))        fssrv::MemoryResourceFromStandardAllocator(GetPointer(g_buffer_allocator));

        /* Set allocators. */
        fs::SetAllocator(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);
        fssystem::InitializeAllocator(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);

        /* Initialize the buffer manager. */
        /* TODO FS-REIMPL: os::AllocateMemoryBlock(...); */
        new (GetPointer(g_buffer_manager)) fssystem::FileSystemBufferManager;
        GetReference(g_buffer_manager).Initialize(MaxCacheCount, reinterpret_cast<uintptr_t>(g_buffer_manager_heap), BufferManagerHeapSize, BlockSize);

        /* TODO FS-REIMPL: Memory Report Creators, fssrv::SetMemoryReportCreator */

        /* TODO FS-REIMPL: Create Pooled Threads, fssystem::RegisterThreadPool. */

        /* Initialize fs creators. */
        new (GetPointer(g_rom_fs_creator))         fssrv::fscreator::RomFileSystemCreator(GetPointer(g_allocator));
        new (GetPointer(g_partition_fs_creator))   fssrv::fscreator::PartitionFileSystemCreator;
        new (GetPointer(g_storage_on_nca_creator)) fssrv::fscreator::StorageOnNcaCreator(GetPointer(g_allocator), *GetNcaCryptoConfiguration(is_prod), is_prod, GetPointer(g_buffer_manager));

        /* TODO FS-REIMPL: Initialize other creators. */

        g_fs_creator_interfaces = {
            .rom_fs_creator         = GetPointer(g_rom_fs_creator),
            .partition_fs_creator   = GetPointer(g_partition_fs_creator),
            .storage_on_nca_creator = GetPointer(g_storage_on_nca_creator),
        };

        /* TODO FS-REIMPL: Sd Card detection, speed emulation. */

        /* Initialize fssrv. TODO FS-REIMPL: More arguments, more actions taken. */
        fssrv::InitializeForFileSystemProxy(std::addressof(g_fs_creator_interfaces), GetPointer(g_buffer_manager), is_development_function_enabled);

        /* Disable auto-abort in fs library code. */
        /* TODO: fs::SetEnabledAutoAbort(false); */

        /* TODO FS-REIMPL: Initialize fsp server. */

        /* NOTE: This is done in fsp server init, normally. */
        fssystem::InitializeBufferPool(reinterpret_cast<char *>(g_device_buffer), DeviceBufferSize);
    }

    const ::ams::fssrv::fscreator::FileSystemCreatorInterfaces *GetFileSystemCreatorInterfaces() {
        return std::addressof(g_fs_creator_interfaces);
    }

}
