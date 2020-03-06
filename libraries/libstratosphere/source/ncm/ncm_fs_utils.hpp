/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#pragma once
#include <stratosphere.hpp>

namespace ams::ncm::impl {

    Result HasFile(bool *out, const char *path);
    Result HasDirectory(bool *out, const char *path);

    Result EnsureDirectoryRecursively(const char *path);
    Result EnsureParentDirectoryRecursively(const char *path);

    Result CopyFile(const char *dst_path, const char *src_path);

    class PathView {
        private:
            std::string_view path; /* Nintendo uses util::string_view here. */
        public:
            PathView(std::string_view p) : path(p) { /* ...*/ }
            bool HasPrefix(std::string_view prefix) const;
            bool HasSuffix(std::string_view suffix) const;
            std::string_view GetFileName() const;
    };

    struct MountName {
        char str[fs::MountNameLengthMax + 1];
    };

    struct RootDirectoryPath {
        char str[fs::MountNameLengthMax + 3]; /* mount name + :/ */
    };

    MountName CreateUniqueMountName();
    RootDirectoryPath GetRootDirectoryPath(const MountName &mount_name);

}
