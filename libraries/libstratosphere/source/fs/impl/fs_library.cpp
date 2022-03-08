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
#include <stratosphere/fssrv/fssrv_interface_adapters.hpp>
#include "fs_library.hpp"
#include "fs_file_system_service_object_adapter.hpp"
#include "../../fssrv/impl/fssrv_allocator_for_service_framework.hpp"

namespace ams::fs::impl {

    #if !defined(ATMOSPHERE_OS_HORIZON)
    namespace {

        constexpr size_t SystemHeapSize = 8_MB;
        alignas(os::MemoryPageSize) constinit u8 g_system_heap[SystemHeapSize];

        ALWAYS_INLINE auto &GetSystemHeapAllocator() {
            AMS_FUNCTION_LOCAL_STATIC(mem::StandardAllocator, s_system_heap_allocator, g_system_heap, sizeof(g_system_heap));
            return s_system_heap_allocator;
        }

        constinit util::optional<fssrv::MemoryResourceFromStandardAllocator> g_system_heap_memory_resource;

        void *AllocateForSystem(size_t size) { return g_system_heap_memory_resource->Allocate(size); }
        void DeallocateForSystem(void *p, size_t size) { return g_system_heap_memory_resource->Deallocate(p, size); }

        [[maybe_unused]] constexpr size_t BufferPoolSize        = 6_MB;
        [[maybe_unused]] constexpr size_t DeviceBufferSize      = 8_MB;
        [[maybe_unused]] constexpr size_t DeviceWorkBufferSize  = os::MemoryPageSize;
        [[maybe_unused]] constexpr size_t BufferManagerHeapSize = 14_MB;

        constexpr size_t DeviceWorkBufferRequiredSize = 0x140;

        static_assert(util::IsAligned(BufferManagerHeapSize, os::MemoryBlockUnitSize));

        //alignas(os::MemoryPageSize) u8 g_buffer_pool[BufferPoolSize];
        alignas(os::MemoryPageSize) u8 g_device_buffer[DeviceBufferSize];
        alignas(os::MemoryPageSize) u8 g_device_work_buffer[DeviceWorkBufferSize];
        //alignas(os::MemoryPageSize) u8 g_buffer_manager_heap[BufferManagerHeapSize];
        //
        //alignas(os::MemoryPageSize) u8 g_buffer_manager_work_buffer[64_KB];
        /* TODO: Other work buffers. */

        /* TODO: Implement pooled threads. */
        // constexpr int    PooledThreadCount     = 12;
        // constexpr size_t PooledThreadStackSize = 32_KB;
        // fssystem::PooledThread g_pooled_threads[PooledThreadCount];

        /* FileSystem creators. */
        constinit util::optional<fssrv::fscreator::LocalFileSystemCreator> g_local_fs_creator;
        constinit util::optional<fssrv::fscreator::SubDirectoryFileSystemCreator> g_subdir_fs_creator;

        constinit fssrv::fscreator::FileSystemCreatorInterfaces g_fs_creator_interfaces = {};

        Result InitializeFileSystemLibraryImpl() {
            /* Set system allocator. */
            fssystem::InitializeAllocator(::ams::fs::impl::Allocate, ::ams::fs::impl::Deallocate);
            fssystem::InitializeAllocatorForSystem(::ams::fs::impl::AllocateForSystem, ::ams::fs::impl::DeallocateForSystem);

            /* TODO: Many things. */
            g_system_heap_memory_resource.emplace(std::addressof(GetSystemHeapAllocator()));

            fssystem::InitializeBufferPool(reinterpret_cast<char *>(g_device_buffer), DeviceBufferSize, reinterpret_cast<char *>(g_device_work_buffer), DeviceWorkBufferRequiredSize);

            /* Setup fscreators/interfaces. */
            g_local_fs_creator.emplace(true);
            g_subdir_fs_creator.emplace();

            g_fs_creator_interfaces.local_fs_creator = std::addressof(*g_local_fs_creator);
            g_fs_creator_interfaces.subdir_fs_creator = std::addressof(*g_subdir_fs_creator);

            /* Initialize fssrv. */
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
            R_SUCCEED();
        }

        class FileSystemLibraryInitializer {
            public:
                FileSystemLibraryInitializer() {
                    R_ABORT_UNLESS(InitializeFileSystemLibraryImpl());
                }
        };

    }
    #endif

    void InitializeFileSystemLibrary() {
        #if !defined(ATMOSPHERE_OS_HORIZON)
        AMS_FUNCTION_LOCAL_STATIC(FileSystemLibraryInitializer, s_library_initializer);
        AMS_UNUSED(s_library_initializer);
        #endif
    }

}
