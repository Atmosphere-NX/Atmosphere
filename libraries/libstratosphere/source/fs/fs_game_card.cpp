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

        const char *GetGameCardMountNameSuffix(GameCardPartition which) {
            switch (which) {
                case GameCardPartition::Update: return impl::GameCardFileSystemMountNameUpdateSuffix;
                case GameCardPartition::Normal: return impl::GameCardFileSystemMountNameNormalSuffix;
                case GameCardPartition::Secure: return impl::GameCardFileSystemMountNameSecureSuffix;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        }

        class GameCardCommonMountNameGenerator : public fsa::ICommonMountNameGenerator, public impl::Newable {
            private:
                const GameCardHandle m_handle;
                const GameCardPartition m_partition;
            public:
                explicit GameCardCommonMountNameGenerator(GameCardHandle h, GameCardPartition p) : m_handle(h), m_partition(p) { /* ... */ }

                virtual Result GenerateCommonMountName(char *dst, size_t dst_size) override {
                    /* Determine how much space we need. */
                    const size_t needed_size = util::Strnlen(impl::GameCardFileSystemMountName, MountNameLengthMax) + util::Strnlen(GetGameCardMountNameSuffix(m_partition), MountNameLengthMax) + sizeof(GameCardHandle) * 2 + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, "%s%s%08x:", impl::GameCardFileSystemMountName, GetGameCardMountNameSuffix(m_partition), m_handle);
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);
                    AMS_UNUSED(size);

                    R_SUCCEED();
                }
        };

    }

    Result GetGameCardHandle(GameCardHandle *out) {
        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the handle. */
        u32 handle;
        AMS_FS_R_TRY(device_operator->GetGameCardHandle(std::addressof(handle)));

        *out = handle;
        R_SUCCEED();
    }

    Result MountGameCardPartition(const char *name, GameCardHandle handle, GameCardPartition partition) {
        auto mount_impl = [=]() -> Result {
            /* Validate the mount name. */
            R_TRY(impl::CheckMountNameAllowingReserved(name));

            /* Open the gamecard filesystem. */
            auto fsp = impl::GetFileSystemProxyServiceObject();
            sf::SharedPointer<fssrv::sf::IFileSystem> fs;
            R_TRY(fsp->OpenGameCardFileSystem(std::addressof(fs), static_cast<u32>(handle), static_cast<u32>(partition)));

            /* Allocate a new filesystem wrapper. */
            auto fsa = std::make_unique<impl::FileSystemServiceObjectAdapter>(std::move(fs));
            R_UNLESS(fsa != nullptr, fs::ResultAllocationMemoryFailedInGameCardC());

            /* Allocate a new mountname generator. */
            auto generator = std::make_unique<GameCardCommonMountNameGenerator>(handle, partition);
            R_UNLESS(generator != nullptr, fs::ResultAllocationMemoryFailedInGameCardD());

            /* Register. */
            R_RETURN(fsa::Register(name, std::move(fsa), std::move(generator)));
        };

        /* Perform the mount. */
        AMS_FS_R_TRY(AMS_FS_IMPL_ACCESS_LOG_SYSTEM_MOUNT(mount_impl(), name, AMS_FS_IMPL_ACCESS_LOG_FORMAT_MOUNT_GAME_CARD_PARTITION(name, handle, partition)));

        /* Enable access logging. */
        AMS_FS_IMPL_ACCESS_LOG_SYSTEM_FS_ACCESSOR_ENABLE(name);

        R_SUCCEED();
    }

    bool IsGameCardInserted() {
        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_ABORT_UNLESS(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get insertion status. */
        bool inserted;
        AMS_FS_R_ABORT_UNLESS(device_operator->IsGameCardInserted(std::addressof(inserted)));

        return inserted;
    }

    Result GetGameCardCid(void *dst, size_t size) {
        /* Check pre-conditions. */
        AMS_FS_R_UNLESS(size >= sizeof(gc::GameCardIdSet), fs::ResultInvalidSize());

        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the id set. */
        gc::GameCardIdSet gc_id_set;
        AMS_FS_R_TRY(device_operator->GetGameCardIdSet(sf::OutBuffer(std::addressof(gc_id_set), sizeof(gc_id_set)), static_cast<s64>(sizeof(gc_id_set))));

        /* Copy the id set to output. */
        std::memcpy(dst, std::addressof(gc_id_set), sizeof(gc_id_set));

        R_SUCCEED();

    }

    Result GetGameCardDeviceId(void *dst, size_t size) {
        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the cid. */
        AMS_FS_R_TRY(device_operator->GetGameCardDeviceId(sf::OutBuffer(dst, size), static_cast<s64>(size)));

        R_SUCCEED();
    }

    Result GetGameCardErrorReportInfo(GameCardErrorReportInfo *out) {
        auto fsp = impl::GetFileSystemProxyServiceObject();

        /* Open a device operator. */
        sf::SharedPointer<fssrv::sf::IDeviceOperator> device_operator;
        AMS_FS_R_TRY(fsp->OpenDeviceOperator(std::addressof(device_operator)));

        /* Get the error report info. */
        AMS_FS_R_TRY(device_operator->GetGameCardErrorReportInfo(out));

        R_SUCCEED();
    }

}
