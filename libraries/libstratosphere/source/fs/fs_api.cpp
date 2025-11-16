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
#include "fs_remote_file_system_proxy.hpp"
#include "fs_remote_file_system_proxy_for_loader.hpp"
#include "impl/fs_library.hpp"

namespace ams::fs {

    #if defined(ATMOSPHERE_OS_HORIZON)
    namespace {

        alignas(0x10) constinit std::byte g_fsp_service_object_buffer[0x80] = {};
        alignas(0x10) constinit std::byte g_fsp_ldr_service_object_buffer[0x80] = {};
        constinit bool g_use_static_fsp_service_object_buffer = false;
        constinit bool g_use_static_fsp_ldr_service_object_buffer = false;

        class HipcClientAllocator {
            public:
                using Policy = sf::StatelessAllocationPolicy<HipcClientAllocator>;
            public:
                constexpr HipcClientAllocator() = default;

                void *Allocate(size_t size) {
                    if (g_use_static_fsp_service_object_buffer) {
                        return std::addressof(g_fsp_service_object_buffer);
                    } else if (g_use_static_fsp_ldr_service_object_buffer) {
                        return std::addressof(g_fsp_ldr_service_object_buffer);
                    } else {
                        return ::ams::fs::impl::Allocate(size);
                    }
                }

                void Deallocate(void *ptr, size_t size) {
                    if (ptr == std::addressof(g_fsp_service_object_buffer)) {
                        return;
                    } else if (ptr == std::addressof(g_fsp_ldr_service_object_buffer)) {
                        return;
                    } else {
                        return ::ams::fs::impl::Deallocate(ptr, size);
                    }
                }
        };

        enum class FileSystemProxySessionSetting {
            SystemNormal = 0,
            Application  = 1,
            SystemMulti  = 2,
        };

        constexpr ALWAYS_INLINE int GetSessionCount(FileSystemProxySessionSetting setting) {
            switch (setting) {
                case FileSystemProxySessionSetting::Application:  return 3;
                case FileSystemProxySessionSetting::SystemNormal: return 1;
                case FileSystemProxySessionSetting::SystemMulti:  return 2;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        constinit bool g_is_fsp_object_initialized = false;
        constinit FileSystemProxySessionSetting g_fsp_session_setting = FileSystemProxySessionSetting::SystemNormal;

        /* TODO: SessionResourceManager */

    }
    #endif

    namespace {

        constinit sf::SharedPointer<fssrv::sf::IFileSystemProxy> g_fsp_service_object;

        sf::SharedPointer<fssrv::sf::IFileSystemProxy> GetFileSystemProxyServiceObjectImpl() {
            /* Ensure the library is initialized. */
            ::ams::fs::impl::InitializeFileSystemLibrary();

            sf::SharedPointer<fssrv::sf::IFileSystemProxy> fsp_object;

            #if defined(ATMOSPHERE_BOARD_NINTENDO_NX) && defined(ATMOSPHERE_OS_HORIZON)
            /* Try to use the custom object. */
            fsp_object = g_fsp_service_object;

            /* If we don't have one, create a remote object. */
            if (fsp_object == nullptr) {
                /* Make our next allocation use our static reserved buffer for the service object. */
                g_use_static_fsp_service_object_buffer = true;
                ON_SCOPE_EXIT { g_use_static_fsp_service_object_buffer = false; };

                using ObjectFactory = sf::ObjectFactory<HipcClientAllocator::Policy>;

                /* Create the object. */
                fsp_object = ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystemProxy, RemoteFileSystemProxy>(GetSessionCount(g_fsp_session_setting));
                AMS_ABORT_UNLESS(fsp_object != nullptr);

                /* Set the current process. */
                const auto scp_res = fsp_object->SetCurrentProcess({});
                R_ASSERT(scp_res);
                AMS_UNUSED(scp_res);
            }
            #else
            /* On non-horizon, use the system object. */
            fsp_object = fssrv::impl::GetFileSystemProxyServiceObject();
            AMS_ABORT_UNLESS(fsp_object != nullptr);

            /* Set the current process. */
            fsp_object->SetCurrentProcess({});
            #endif


            /* Return the object. */
            return fsp_object;
        }

        sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader> GetFileSystemProxyForLoaderServiceObjectImpl() {
            /* Ensure the library is initialized. */
            ::ams::fs::impl::InitializeFileSystemLibrary();

            sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader> fsp_ldr_object;

            #if defined(ATMOSPHERE_BOARD_NINTENDO_NX) && defined(ATMOSPHERE_OS_HORIZON)
            /* Make our next allocation use our static reserved buffer for the service object. */
            g_use_static_fsp_ldr_service_object_buffer = true;
            ON_SCOPE_EXIT { g_use_static_fsp_ldr_service_object_buffer = false; };

            using ObjectFactory = sf::ObjectFactory<HipcClientAllocator::Policy>;

            /* Create the object. */
            fsp_ldr_object = ObjectFactory::CreateSharedEmplaced<fssrv::sf::IFileSystemProxyForLoader, RemoteFileSystemProxyForLoader>();
            AMS_ABORT_UNLESS(fsp_ldr_object != nullptr);
            #else
            /* On non-horizon, use the system object. */
            fsp_ldr_object = fssrv::impl::GetFileSystemProxyForLoaderServiceObject();
            AMS_ABORT_UNLESS(fsp_ldr_object != nullptr);
            #endif

            /* Return the object. */
            return fsp_ldr_object;
        }

    }

    namespace impl {

        sf::SharedPointer<fssrv::sf::IFileSystemProxy> GetFileSystemProxyServiceObject() {
            AMS_FUNCTION_LOCAL_STATIC(sf::SharedPointer<fssrv::sf::IFileSystemProxy>, s_fsp_service_object, GetFileSystemProxyServiceObjectImpl());
            return s_fsp_service_object;
        }

        sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader> GetFileSystemProxyForLoaderServiceObject() {
            AMS_FUNCTION_LOCAL_STATIC(sf::SharedPointer<fssrv::sf::IFileSystemProxyForLoader>, s_fsp_ldr_service_object, GetFileSystemProxyForLoaderServiceObjectImpl());
            return s_fsp_ldr_service_object;
        }

    }

    void InitializeForHostTool() {
        #if !defined(ATMOSPHERE_OS_HORIZON)
        AMS_ABORT_UNLESS(impl::GetFileSystemProxyServiceObject() != nullptr);
        R_ABORT_UNLESS(::ams::fs::MountHostRoot());
        #endif
    }

    void InitializeForSystem() {
        #if defined(ATMOSPHERE_OS_HORIZON)
        AMS_ABORT_UNLESS(!g_is_fsp_object_initialized);
        AMS_ABORT_UNLESS(g_fsp_session_setting == FileSystemProxySessionSetting::SystemNormal);
        g_fsp_session_setting = FileSystemProxySessionSetting::SystemNormal;

        /* Nintendo doesn't do this, but we have to for timing reasons. */
        AMS_ABORT_UNLESS(impl::GetFileSystemProxyServiceObject() != nullptr);
        #endif
    }

    void InitializeWithMultiSessionForSystem() {
        #if defined(ATMOSPHERE_OS_HORIZON)
        AMS_ABORT_UNLESS(!g_is_fsp_object_initialized);
        AMS_ABORT_UNLESS(g_fsp_session_setting == FileSystemProxySessionSetting::SystemNormal);
        g_fsp_session_setting = FileSystemProxySessionSetting::SystemMulti;

        /* Nintendo doesn't do this, but we have to for timing reasons. */
        AMS_ABORT_UNLESS(impl::GetFileSystemProxyServiceObject() != nullptr);
        #endif
    }

}
