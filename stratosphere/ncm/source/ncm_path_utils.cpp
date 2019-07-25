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

#include "ncm_path_utils.hpp"
#include "ncm_utils.hpp"

namespace sts::ncm::path {

    void GetContentMetaPath(char* out, ContentId content_id, MakeContentPathFunc path_func, const char* root_path) {
        char tmp_path[FS_MAX_PATH-1] = {0};
        char content_path[FS_MAX_PATH-1] = {0};
        path_func(content_path, content_id, root_path);
        const size_t len = strnlen(content_path, FS_MAX_PATH-1);
        const size_t len_no_extension = len - 4;

        if (len_no_extension > len || len_no_extension >= FS_MAX_PATH-1) {
            std::abort();
        }

        strncpy(tmp_path, content_path, len_no_extension);
        memcpy(out, tmp_path, FS_MAX_PATH-1);
        const size_t out_len = strnlen(out, FS_MAX_PATH-1);

        if (out_len + 9 >= FS_MAX_PATH-1) {
            std::abort();
        }

        strncat(out, ".cnmt.nca", 0x2ff - out_len);
    }

    void GetContentFileName(char* out, ContentId content_id) {
        char content_name[sizeof(ContentId)*2+1] = {0};
        GetStringFromContentId(content_name, content_id);
        snprintf(out, FS_MAX_PATH-1, "%s%s", content_name, ".nca");
    }

    void GetPlaceHolderFileName(char* out, PlaceHolderId placeholder_id) {
        char placeholder_name[sizeof(PlaceHolderId)*2+1] = {0};
        GetStringFromPlaceHolderId(placeholder_name, placeholder_id);
        snprintf(out, FS_MAX_PATH-1, "%s%s", placeholder_name, ".nca");
    }

    bool IsNcaPath(const char* path) {
        PathView path_view(path);

        if (!path_view.HasSuffix(".nca")) {
            return false;
        }

        std::string_view file_name = path_view.GetFileName();

        if (file_name.length() != 0x24) {
            return false;
        }

        for (size_t i = 0; i < sizeof(Uuid)*2; i++) {
            if (!std::isxdigit(file_name.at(i))) {
                return false;
            }
        }

        return true;
    }

    bool PathView::HasPrefix(std::string_view prefix) const {
        return this->path.compare(0, prefix.length(), prefix) == 0;
    }

    bool PathView::HasSuffix(std::string_view suffix) const {
        return this->path.compare(this->path.length() - suffix.length(), suffix.length(), suffix) == 0;
    }

    std::string_view PathView::GetFileName() const {
        return this->path.substr(this->path.find_last_of("/") + 1);
    }

}