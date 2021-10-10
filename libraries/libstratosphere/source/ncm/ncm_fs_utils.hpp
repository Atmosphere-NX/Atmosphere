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
#pragma once
#include <stratosphere.hpp>

namespace ams::ncm::impl {

    Result CopyFile(const char *dst_path, const char *src_path);

    class PathView {
        private:
            util::string_view m_path;
        public:
            PathView(util::string_view p) : m_path(p) { /* ...*/ }
            bool HasPrefix(util::string_view prefix) const;
            bool HasSuffix(util::string_view suffix) const;
            util::string_view GetFileName() const;
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
