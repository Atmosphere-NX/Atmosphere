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
#include "fs_mount_table.hpp"

namespace ams::fs::impl {

    namespace {

        bool MatchesName(const FileSystemAccessor &accessor, const char *name) {
            return std::strncmp(accessor.GetName(), name, sizeof(MountName)) == 0;
        }

    }

    bool MountTable::CanAcceptMountName(const char *name) {
        for (const auto &fs : this->fs_list) {
            if (MatchesName(fs, name)) {
                return false;
            }
        }
        return true;
    }

    Result MountTable::Mount(std::unique_ptr<FileSystemAccessor> &&fs) {
        std::scoped_lock lk(this->mutex);

        R_UNLESS(this->CanAcceptMountName(fs->GetName()), fs::ResultMountNameAlreadyExists());

        this->fs_list.push_back(*fs.release());
        return ResultSuccess();
    }

    Result MountTable::Find(FileSystemAccessor **out, const char *name) {
        std::scoped_lock lk(this->mutex);

        for (auto &fs : this->fs_list) {
            if (MatchesName(fs, name)) {
                *out = std::addressof(fs);
                return ResultSuccess();
            }
        }

        return fs::ResultNotMounted();
    }

    void MountTable::Unmount(const char *name) {
        std::scoped_lock lk(this->mutex);

        for (auto it = this->fs_list.cbegin(); it != this->fs_list.cend(); it++) {
            if (MatchesName(*it, name)) {
                auto p = std::addressof(*it);
                this->fs_list.erase(it);
                delete p;
                return;
            }
        }

        R_ABORT_UNLESS(fs::ResultNotMounted());
    }

}
