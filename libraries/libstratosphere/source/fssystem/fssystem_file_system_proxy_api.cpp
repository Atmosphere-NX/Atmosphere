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
#include "../fssrv/impl/fssrv_program_registry_manager.hpp"

namespace ams::fssystem {

    /* TODO: All of this should really be inside fs process, but ams.mitm wants it to. */
    /* How should we handle this? */

    namespace {

        constexpr inline auto FileSystemProxyServerThreadCount = fssrv::FileSystemProxyServerActiveSessionCount;

        /* TODO: Heap sizes need to match FS, when this is FS in master rather than ams.mitm. */

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

        alignas(os::MemoryPageSize) constinit u8 g_exp_heap_buffer[ExpHeapSize];
        constinit lmem::HeapHandle g_exp_heap_handle = nullptr;
        constinit fssrv::PeakCheckableMemoryResourceFromExpHeap g_exp_allocator(ExpHeapSize);

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

        alignas(os::MemoryPageSize) constinit u8 g_device_buffer[DeviceBufferSize] = {};

        alignas(os::MemoryPageSize) constinit u8 g_buffer_pool[BufferPoolSize] = {};

        constinit util::TypedStorage<mem::StandardAllocator> g_buffer_allocator = {};
        constinit util::TypedStorage<fssrv::MemoryResourceFromStandardAllocator> g_allocator = {};

        /* TODO: Nintendo uses os::SetMemoryHeapSize (svc::SetHeapSize) and os::AllocateMemoryBlock for the BufferManager heap. */
        /* It's unclear how we should handle this in ams.mitm (especially hoping to reuse some logic for fs reimpl). */
        /* Should we be doing the same(?) */
        constinit util::TypedStorage<fssystem::FileSystemBufferManager> g_buffer_manager = {};
        alignas(os::MemoryPageSize) constinit u8 g_buffer_manager_heap[BufferManagerHeapSize] = {};

        /* FileSystem creators. */
        constinit util::TypedStorage<fssrv::fscreator::RomFileSystemCreator>       g_rom_fs_creator = {};
        constinit util::TypedStorage<fssrv::fscreator::PartitionFileSystemCreator> g_partition_fs_creator = {};
        constinit util::TypedStorage<fssrv::fscreator::StorageOnNcaCreator>        g_storage_on_nca_creator = {};

        constinit fssrv::fscreator::FileSystemCreatorInterfaces g_fs_creator_interfaces = {};

    }

    void InitializeForFileSystemProxy() {
        /* TODO FS-REIMPL: Setup MainThreadStackUsageReporter. */

        /* Register service context for main thread. */
        fssystem::ServiceContext context;
        fssystem::RegisterServiceContext(std::addressof(context));

        /* Initialize spl library. */
        spl::InitializeForFs();
        /* TODO FS-REIMPL: spl::SetIsAvailableAccessKeyHandler(fssrv::IsAvailableAccessKey) */

        /* Determine whether we're prod or dev. */
        bool is_prod                         = !spl::IsDevelopment();
        bool is_development_function_enabled = spl::IsDevelopmentFunctionEnabled();

        /* Set debug flags. */
        fssrv::SetDebugFlagEnabled(is_development_function_enabled);

        /* Setup our crypto configuration. */
        SetUpKekAccessKeys(is_prod);

        /* Setup our heap. */
        InitializeExpHeap();

        /* Initialize buffer allocator. */
        util::ConstructAt(g_buffer_allocator, g_buffer_pool, BufferPoolSize);
        util::ConstructAt(g_allocator, GetPointer(g_buffer_allocator));

        /* Set allocators. */
        /* TODO FS-REIMPL: sf::SetGlobalDefaultMemoryResource() */
        fs::SetAllocator(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);
        fssystem::InitializeAllocator(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);
        fssystem::InitializeAllocatorForSystem(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);

        /* Initialize the buffer manager. */
        /* TODO FS-REIMPL: os::AllocateMemoryBlock(...); */
        util::ConstructAt(g_buffer_manager);
        const auto bm_res = GetReference(g_buffer_manager).Initialize(MaxCacheCount, reinterpret_cast<uintptr_t>(g_buffer_manager_heap), BufferManagerHeapSize, BlockSize);
        R_ASSERT(bm_res);
        AMS_UNUSED(bm_res);

        /* TODO FS-REIMPL: os::AllocateMemoryBlock(...); */
        /* TODO FS-REIMPL: fssrv::storage::CreateDeviceAddressSpace(...); */
        const auto ibp_res = fssystem::InitializeBufferPool(reinterpret_cast<char *>(g_device_buffer), DeviceBufferSize);
        R_ASSERT(ibp_res);
        AMS_UNUSED(ibp_res);

        /* TODO FS-REIMPL: Create Pooled Threads/Stack Usage Reporter, fssystem::RegisterThreadPool. */

        /* TODO FS-REIMPL: fssrv::GetFileSystemProxyServices(), some service creation. */

        /* Initialize fs creators. */
        /* TODO FS-REIMPL: Revise for accuracy. */
        util::ConstructAt(g_rom_fs_creator, GetPointer(g_allocator));
        util::ConstructAt(g_partition_fs_creator);
        util::ConstructAt(g_storage_on_nca_creator, GetPointer(g_allocator), *GetNcaCryptoConfiguration(is_prod), *GetNcaCompressionConfiguration(), GetPointer(g_buffer_manager), fs::impl::GetNcaHashGeneratorFactorySelector());

        /* TODO FS-REIMPL: Initialize other creators. */

        g_fs_creator_interfaces = {
            .rom_fs_creator         = GetPointer(g_rom_fs_creator),
            .partition_fs_creator   = GetPointer(g_partition_fs_creator),
            .storage_on_nca_creator = GetPointer(g_storage_on_nca_creator),
        };

        /* TODO FS-REIMPL: Revise above for latest firmware, all the new Services creation. */
        fssrv::ProgramRegistryServiceImpl program_registry_service(fssrv::ProgramRegistryServiceImpl::Configuration{});
        fssrv::ProgramRegistryImpl::Initialize(std::addressof(program_registry_service));

        /* TODO FS-REIMPL: Memory Report Creators, fssrv::SetMemoryReportCreator */

        /* TODO FS-REIMPL: Sd Card detection, speed emulation. */

        /* Initialize fssrv. TODO FS-REIMPL: More arguments, more actions taken. */
        const fssrv::FileSystemProxyConfiguration config  = {
            .m_fs_creator_interfaces = std::addressof(g_fs_creator_interfaces),
            .m_base_storage_service_impl = nullptr /* TODO */,
            .m_base_file_system_service_impl = nullptr /* TODO */,
            .m_nca_file_system_service_impl = nullptr /* TODO */,
            .m_save_data_file_system_service_impl = nullptr /* TODO */,
            .m_access_failure_management_service_impl = nullptr /* TODO */,
            .m_time_service_impl = nullptr /* TODO */,
            .m_status_report_service_impl = nullptr /* TODO */,
            .m_program_registry_service_impl = std::addressof(program_registry_service),
            .m_access_log_service_impl = nullptr /* TODO */,
            .m_debug_configuration_service_impl = nullptr /* TODO */,
        };

        fssrv::InitializeForFileSystemProxy(config);

        /* TODO FS-REIMPL: GetFileSystemProxyServiceObject(), set current process, initialize global service object. */

        /* Disable auto-abort in fs library code. */
        fs::SetEnabledAutoAbort(false);

        /* Initialize fsp server. */
        fssrv::InitializeFileSystemProxyServer(FileSystemProxyServerThreadCount);

        /* TODO FS-REIMPL: Cleanup calls. */

        /* TODO FS-REIMPL: Spawn worker threads. */

        /* TODO FS-REIMPL: Set mmc devices ready. */

        /* TODO FS-REIMPL: fssrv::LoopPmEventServer(...); */

        /* TODO FS-REIMPL: Wait/destroy threads. */

        /* TODO FS-REIMPL: spl::Finalize(); */
    }

    void InitializeForAtmosphereMitm() {
        /* Initialize spl library. */
        spl::InitializeForFs();

        /* TODO FS-REIMPL: spl::SetIsAvailableAccessKeyHandler(fssrv::IsAvailableAccessKey) */

        /* Determine whether we're prod or dev. */
        bool is_prod                         = !spl::IsDevelopment();
        bool is_development_function_enabled = spl::IsDevelopmentFunctionEnabled();

        /* Set debug flags. */
        fssrv::SetDebugFlagEnabled(is_development_function_enabled);

        /* Setup our crypto configuration. */
        SetUpKekAccessKeys(is_prod);

        /* Setup our heap. */
        InitializeExpHeap();

        /* Initialize buffer allocator. */
        util::ConstructAt(g_buffer_allocator, g_buffer_pool, BufferPoolSize);
        util::ConstructAt(g_allocator, GetPointer(g_buffer_allocator));

        /* Set allocators. */
        /* TODO FS-REIMPL: sf::SetGlobalDefaultMemoryResource() */
        fs::SetAllocator(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);
        fssystem::InitializeAllocator(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);
        fssystem::InitializeAllocatorForSystem(AllocateForFileSystemProxy, DeallocateForFileSystemProxy);

        /* Initialize the buffer manager. */
        /* TODO FS-REIMPL: os::AllocateMemoryBlock(...); */
        util::ConstructAt(g_buffer_manager);
        const auto bm_res = GetReference(g_buffer_manager).Initialize(MaxCacheCount, reinterpret_cast<uintptr_t>(g_buffer_manager_heap), BufferManagerHeapSize, BlockSize);
        R_ASSERT(bm_res);
        AMS_UNUSED(bm_res);

        /* TODO FS-REIMPL: os::AllocateMemoryBlock(...); */
        /* TODO FS-REIMPL: fssrv::storage::CreateDeviceAddressSpace(...); */
        const auto ibp_res = fssystem::InitializeBufferPool(reinterpret_cast<char *>(g_device_buffer), DeviceBufferSize);
        R_ASSERT(ibp_res);
        AMS_UNUSED(ibp_res);

        /* TODO FS-REIMPL: Create Pooled Threads/Stack Usage Reporter, fssystem::RegisterThreadPool. */

        /* TODO FS-REIMPL: fssrv::GetFileSystemProxyServices(), some service creation. */

        /* Initialize fs creators. */
        /* TODO FS-REIMPL: Revise for accuracy. */
        util::ConstructAt(g_rom_fs_creator, GetPointer(g_allocator));
        util::ConstructAt(g_partition_fs_creator);
        util::ConstructAt(g_storage_on_nca_creator, GetPointer(g_allocator), *GetNcaCryptoConfiguration(is_prod), *GetNcaCompressionConfiguration(), GetPointer(g_buffer_manager), fs::impl::GetNcaHashGeneratorFactorySelector());

        /* TODO FS-REIMPL: Initialize other creators. */
        g_fs_creator_interfaces = {
            .rom_fs_creator         = GetPointer(g_rom_fs_creator),
            .partition_fs_creator   = GetPointer(g_partition_fs_creator),
            .storage_on_nca_creator = GetPointer(g_storage_on_nca_creator),
        };

        /* Initialize fssrv. TODO FS-REIMPL: More arguments, more actions taken. */
        const fssrv::FileSystemProxyConfiguration config  = {
            .m_fs_creator_interfaces = std::addressof(g_fs_creator_interfaces),
            .m_base_storage_service_impl = nullptr /* TODO */,
            .m_base_file_system_service_impl = nullptr /* TODO */,
            .m_nca_file_system_service_impl = nullptr /* TODO */,
            .m_save_data_file_system_service_impl = nullptr /* TODO */,
            .m_access_failure_management_service_impl = nullptr /* TODO */,
            .m_time_service_impl = nullptr /* TODO */,
            .m_status_report_service_impl = nullptr /* TODO */,
            .m_program_registry_service_impl = nullptr /* TODO */,
            .m_access_log_service_impl = nullptr /* TODO */,
            .m_debug_configuration_service_impl = nullptr /* TODO */,
        };

        fssrv::InitializeForFileSystemProxy(config);

        /* Disable auto-abort in fs library code. */
        fs::SetEnabledAutoAbort(false);

        /* Quick sanity check, before we leave. */
        #if defined(ATMOSPHERE_OS_HORIZON)
        AMS_ABORT_UNLESS(os::GetCurrentProgramId() == ncm::AtmosphereProgramId::Mitm);
        #endif
    }

    const ::ams::fssrv::fscreator::FileSystemCreatorInterfaces *GetFileSystemCreatorInterfaces() {
        return std::addressof(g_fs_creator_interfaces);
    }

}
