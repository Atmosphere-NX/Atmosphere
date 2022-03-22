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
                    const size_t needed_size = strnlen(impl::GameCardFileSystemMountName, MountNameLengthMax) + strnlen(GetGameCardMountNameSuffix(m_partition), MountNameLengthMax) + sizeof(GameCardHandle) * 2 + 2;
                    AMS_ABORT_UNLESS(dst_size >= needed_size);

                    /* Generate the name. */
                    const auto size = util::SNPrintf(dst, dst_size, "%s%s%08x:", impl::GameCardFileSystemMountName, GetGameCardMountNameSuffix(m_partition), m_handle);
                    AMS_ASSERT(static_cast<size_t>(size) == needed_size - 1);
                    AMS_UNUSED(size);

                    return ResultSuccess();
                }
        };

    }

    Result GetGameCardHandle(GameCardHandle *out) {
        /* TODO: fs::DeviceOperator */
        /* Open a DeviceOperator. */
        ::FsDeviceOperator d;
        R_TRY(fsOpenDeviceOperator(std::addressof(d)));
        ON_SCOPE_EXIT { fsDeviceOperatorClose(std::addressof(d)); };

        /* Get the handle. */
        static_assert(sizeof(GameCardHandle) == sizeof(::FsGameCardHandle));
        return fsDeviceOperatorGetGameCardHandle(std::addressof(d), reinterpret_cast<::FsGameCardHandle *>(out));
    }

    Result MountGameCardPartition(const char *name, GameCardHandle handle, GameCardPartition partition) {
        /* Validate the mount name. */
        R_TRY(impl::CheckMountNameAllowingReserved(name));

        /* Open gamecard filesystem. This uses libnx bindings. */
        ::FsFileSystem fs;
        const ::FsGameCardHandle _hnd = {handle};
        R_TRY(fsOpenGameCardFileSystem(std::addressof(fs), std::addressof(_hnd), static_cast<::FsGameCardPartition>(partition)));

        /* Allocate a new filesystem wrapper. */
        auto fsa = std::make_unique<RemoteFileSystem>(fs);
        R_UNLESS(fsa != nullptr, fs::ResultAllocationFailureInGameCardC());

        /* Allocate a new mountname generator. */
        auto generator = std::make_unique<GameCardCommonMountNameGenerator>(handle, partition);
        R_UNLESS(generator != nullptr, fs::ResultAllocationFailureInGameCardD());

        /* Register. */
        return fsa::Register(name, std::move(fsa), std::move(generator));
    }

}
