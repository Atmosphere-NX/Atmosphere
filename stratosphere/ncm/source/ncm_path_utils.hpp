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
#include <switch.h>
#include <stratosphere.hpp>

namespace ams::ncm::path {

    inline void GetContentRootPath(PathString *content_root, const PathString &root_path) {
        content_root->SetFormat("%s%s", root_path.Get(), "/registered");
    }

    inline void GetPlaceHolderRootPath(PathString *placeholder_root, const PathString &root_path) {
        placeholder_root->SetFormat("%s%s", root_path.Get(), "/placehld");
    }

    void GetContentMetaPath(PathString *out, ContentId content_id, MakeContentPathFunc path_func, const PathString &root_path);
    void GetContentFileName(char *out, ContentId content_id);
    void GetPlaceHolderFileName(char *out, PlaceHolderId placeholder_id);
    bool IsNcaPath(const char *path);

    class PathView {
        private:
            std::string_view path; /* Nintendo uses nn::util::string_view here. */
        public:
            PathView(std::string_view p) : path(p) { /* ...*/ }
            bool HasPrefix(std::string_view prefix) const;
            bool HasSuffix(std::string_view suffix) const;
            std::string_view GetFileName() const;
    };

}
