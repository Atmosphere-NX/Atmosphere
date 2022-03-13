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
#include "fsa/fs_mount_utils.hpp"
#include "impl/fs_file_system_proxy_service_object.hpp"
#include "impl/fs_file_system_service_object_adapter.hpp"

namespace ams::fs {

    namespace {

        class HostRootCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            public:
                explicit HostRootCommonMountNameGenerator() { /* ... */ }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    constexpr size_t RequiredNameBufferSizeSize = AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + 1 + 1;
                    AMS_ASSERT(dst_size >= RequiredNameBufferSizeSize);
                    AMS_UNUSED(RequiredNameBufferSizeSize);

                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME ":");
                    AMS_ASSERT(static_cast<size_t>(size) == RequiredNameBufferSizeSize - 1);
                    AMS_UNUSED(size);

                    R_SUCCEED();
                }
        };

        class HostCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            private:
                char m_path[fs::EntryNameLengthMax + 1];
            public:
                HostCommonMountNameGenerator(const char *path) {
                    util::Strlcpy<char>(m_path, path, sizeof(m_path));
                }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    const size_t required_size = AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME_LEN + 1 + util::Strnlen<char>(m_path, sizeof(m_path)) + 1; /* @Host:%s */
                    R_UNLESS(dst_size >= required_size, fs::ResultTooLongPath());


                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, AMS_FS_IMPL_HOST_ROOT_FILE_SYSTEM_MOUNT_NAME ":%s", m_path);
                    AMS_ASSERT(static_cast<size_t>(size) == required_size - 1);
                    AMS_UNUSED(size);

                    R_SUCCEED();
                }
        };

        Result OpenHostFileSystemImpl(std::unique_ptr<fs::fsa::IFileSystem> *out, const fssrv::sf::FspPath &path, const MountHostOption &option) {
            /* Open the filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            if (option != MountHostOption::None) {
                R_TRY(fsp->OpenHostFileSystemWithOption(std::addressof(fs), path, option._value));
            } else {
                R_TRY(fsp->OpenHostFileSystem(std::addressof(fs), path));
            }

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInHostA());

            /* Set the output. */
            *out = std::move(fsa);
            R_SUCCEED();
        }

        Result PreMountHost(std::unique_ptr<fs::HostCommonMountNameGenerator> *out, const char *name, const char *path) {
            /* Check pre-conditions. */
            AMS_ASSERT(out != nullptr);

            /* Check the mount name. */
            R_TRY(impl::CheckMountName(name));

            /* Check that path is valid. */
            R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

            /* Create a new HostCommonMountNameGenerator. */
            *out = std::make_unique<HostCommonMountNameGenerator>(path);
            R_UNLESS(out->get() != nullptr, fs::ResultAllocationMemoryFailedInHostB());

            R_SUCCEED();
        }

    }

    namespace impl {

        Result OpenHostFileSystem(std::unique_ptr<fs::fsa::IFileSystem> *out, const char *name, const char *path, const fs::MountHostOption &option) {
            /* Validate arguments. */
            R_UNLESS(out != nullptr,  fs::ResultNullptrArgument());
            R_UNLESS(name != nullptr, fs::ResultNullptrArgument());
            R_UNLESS(path != nullptr, fs::ResultNullptrArgument());

            /* Check mount name isn't windows path or reserved. */
            R_UNLESS(!fs::IsWindowsDrive(name),            fs::ResultInvalidMountName());
            R_UNLESS(!fs::impl::IsReservedMountName(name), fs::ResultInvalidMountName());

            /* Convert the path for fsp. */
            fssrv::sf::FspPath sf_path;
            R_TRY(fs::ConvertToFspPath(std::addressof(sf_path), path));

            /* Ensure that the path doesn't correspond to the root. */
            if (sf_path.str[0] == 0) {
                sf_path.str[0] = '.';
                sf_path.str[1] = 0;
            }

            /* Open the host file system. */
            R_RETURN(OpenHostFileSystemImpl(out, sf_path, option));
        }

    }

    Result MountHost(const char *name, const char *root_path) {
        /* Pre-mount host. */
        std::unique_ptr<fs::HostCommonMountNameGenerator> generator;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(PreMountHost(std::addressof(generator), name, root_path), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST(name, root_path)));

        /* Open the filesystem. */
        std::unique_ptr<fs::fsa::IFileSystem> fsa;
        R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(impl::OpenHostFileSystem(std::addressof(fsa), name, root_path, MountHostOption::None), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST(name, root_path)));

        /* Declare registration helper. */
        auto register_impl = [&]() -> Result {
            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa), std::move(generator)));
        };

        /* Mount the filesystem. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT(register_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST(name, root_path)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    Result MountHost(const char *name, const char *root_path, const MountHostOption &option) {
        /* Pre-mount host. */
        std::unique_ptr<fs::HostCommonMountNameGenerator> generator;
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(PreMountHost(std::addressof(generator), name, root_path), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_WITH_OPTION(name, root_path, option)));

        /* Open the filesystem. */
        std::unique_ptr<fs::fsa::IFileSystem> fsa;
        R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(impl::OpenHostFileSystem(std::addressof(fsa), name, root_path, option), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_WITH_OPTION(name, root_path, option)));

        /* Declare registration helper. */
        auto register_impl = [&]() -> Result {
            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa), std::move(generator)));
        };

        /* Mount the filesystem. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT(register_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_WITH_OPTION(name, root_path, option)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    Result MountHostRoot() {
        /* Create host root path. */
        fssrv::sf::FspPath sf_path;
        sf_path.str[0] = 0;

        /* Open the filesystem. */
        std::unique_ptr<fs::fsa::IFileSystem> fsa;
        R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(OpenHostFileSystemImpl(std::addressof(fsa), sf_path, MountHostOption::None), impl::HostRootFileSystemMountName, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_ROOT()));

        /* Declare registration helper. */
        auto register_impl = [&]() -> Result {
            /* Allocate a new mountname generator. */
            auto generator = std::make_unique<HostRootCommonMountNameGenerator>();
            R_UNLESS(generator != nullptr, fs::ResultAllocationMemoryFailedInHostC());

            /* Register. */
            R_RETURN(fsa::Register(impl::HostRootFileSystemMountName, std::move(fsa), std::move(generator)));
        };

        /* Mount the filesystem. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT(register_impl(), impl::HostRootFileSystemMountName, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_ROOT()));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE(impl::HostRootFileSystemMountName);

        R_SUCCEED();
    }

    Result MountHostRoot(const MountHostOption &option) {
        /* Create host root path. */
        fssrv::sf::FspPath sf_path;
        sf_path.str[0] = 0;

        /* Open the filesystem. */
        std::unique_ptr<fs::fsa::IFileSystem> fsa;
        R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT_UNLESS_R_SUCCEEDED(OpenHostFileSystemImpl(std::addressof(fsa), sf_path, option), impl::HostRootFileSystemMountName, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_ROOT_WITH_OPTION(option)));

        /* Declare registration helper. */
        auto register_impl = [&]() -> Result {
            /* Allocate a new mountname generator. */
            auto generator = std::make_unique<HostRootCommonMountNameGenerator>();
            R_UNLESS(generator != nullptr, fs::ResultAllocationMemoryFailedInHostC());

            /* Register. */
            R_RETURN(fsa::Register(impl::HostRootFileSystemMountName, std::move(fsa), std::move(generator)));
        };

        /* Mount the filesystem. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_MOUNT(register_impl(), impl::HostRootFileSystemMountName, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_HOST_ROOT_WITH_OPTION(option)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_FS_ACCESSOR_ENABLE(impl::HostRootFileSystemMountName);

        R_SUCCEED();
    }

    void UnmountHostRoot() {
        return Unmount(impl::HostRootFileSystemMountName);
    }

}
