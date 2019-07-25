/*
 * Copyright (c) 2019 Adubbz
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

#include "ncm_types.hpp"

namespace sts::ncm::path {

    inline void GetContentRootPath(char* out_content_root, const char* root_path) {
        /* TODO: Replace with BoundedString? */
        if (snprintf(out_content_root, FS_MAX_PATH-1, "%s%s", root_path, "/registered") < 0) {
            std::abort();
        }
    }

    inline void GetPlaceHolderRootPath(char* out_placeholder_root, const char* root_path) {
        /* TODO: Replace with BoundedString? */
        if (snprintf(out_placeholder_root, FS_MAX_PATH, "%s%s", root_path, "/placehld") < 0) {
            std::abort();
        }
    }

    void GetContentMetaPath(char* out, ContentId content_id, MakeContentPathFunc path_func, const char* root_path);
    void GetContentFileName(char* out, ContentId content_id);
    void GetPlaceHolderFileName(char* out, PlaceHolderId placeholder_id);
    bool IsNcaPath(const char* path);

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