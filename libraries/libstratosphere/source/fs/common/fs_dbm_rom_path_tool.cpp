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

namespace ams::fs::RomPathTool {

    Result PathParser::Initialize(const RomPathChar *path) {
        AMS_ASSERT(path != nullptr);

        /* Require paths start with a separator, and skip repeated separators. */
        R_UNLESS(IsSeparator(path[0]), fs::ResultDbmInvalidPathFormat());
        while (IsSeparator(path[1])) {
            path++;
        }

        this->prev_path_start = path;
        this->prev_path_end   = path;
        for (this->next_path = path + 1; IsSeparator(this->next_path[0]); ++this->next_path) {
            /* ... */
        }

        return ResultSuccess();
    }

    void PathParser::Finalize() {
        this->prev_path_start = nullptr;
        this->prev_path_end   = nullptr;
        this->next_path       = nullptr;
        this->finished        = false;
    }

    bool PathParser::IsParseFinished() const {
        return this->finished;
    }

    bool PathParser::IsDirectoryPath() const {
        AMS_ASSERT(this->next_path != nullptr);

        if (IsNullTerminator(this->next_path[0]) && IsSeparator(this->next_path[-1])) {
            return true;
        }

        if (IsCurrentDirectory(this->next_path)) {
            return true;
        }

        return IsParentDirectory(this->next_path);
    }

    Result PathParser::GetNextDirectoryName(RomEntryName *out) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(this->prev_path_start != nullptr);
        AMS_ASSERT(this->prev_path_end   != nullptr);
        AMS_ASSERT(this->next_path       != nullptr);

        /* Set the current path to output. */
        out->length = this->prev_path_end - this->prev_path_start;
        out->path   = this->prev_path_start;

        /* Parse the next path. */
        this->prev_path_start = this->next_path;
        const RomPathChar *cur = this->next_path;
        for (size_t name_len = 0; true; name_len++) {
            if (IsSeparator(cur[name_len])) {
                R_UNLESS(name_len < MaxPathLength, fs::ResultDbmDirectoryNameTooLong());

                this->prev_path_end = cur + name_len;
                this->next_path     = this->prev_path_end + 1;

                while (IsSeparator(this->next_path[0])) {
                    ++this->next_path;
                }
                if (IsNullTerminator(this->next_path[0])) {
                    this->finished = true;
                }
                break;
            }

            if (IsNullTerminator(cur[name_len])) {
                this->finished      = true;
                this->next_path     = cur + name_len;
                this->prev_path_end = cur + name_len;
                break;
            }
        }

        return ResultSuccess();
    }

    Result PathParser::GetAsDirectoryName(RomEntryName *out) const {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(this->prev_path_start != nullptr);
        AMS_ASSERT(this->prev_path_end   != nullptr);
        AMS_ASSERT(this->next_path       != nullptr);

        const size_t len = this->prev_path_end - this->prev_path_start;
        R_UNLESS(len <= MaxPathLength, fs::ResultDbmDirectoryNameTooLong());

        out->length = len;
        out->path   = this->prev_path_start;
        return ResultSuccess();
    }

    Result PathParser::GetAsFileName(RomEntryName *out) const {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(this->prev_path_start != nullptr);
        AMS_ASSERT(this->prev_path_end   != nullptr);
        AMS_ASSERT(this->next_path       != nullptr);

        const size_t len = this->prev_path_end - this->prev_path_start;
        R_UNLESS(len <= MaxPathLength, fs::ResultDbmFileNameTooLong());

        out->length = len;
        out->path   = this->prev_path_start;
        return ResultSuccess();
    }

    Result GetParentDirectoryName(RomEntryName *out, const RomEntryName &cur, const RomPathChar *p) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(p != nullptr);

        const RomPathChar *start = cur.path;
        const RomPathChar *end   = cur.path + cur.length - 1;

        s32 depth = 1;
        if (IsParentDirectory(cur)) {
            ++depth;
        }

        if (cur.path > p) {
            size_t len = 0;
            const RomPathChar *head = cur.path - 1;
            while (head >= p) {
                if (IsSeparator(*head)) {
                    if (IsCurrentDirectory(head + 1, len)) {
                        ++depth;
                    }

                    if (IsParentDirectory(head + 1, len)) {
                        depth += 2;
                    }

                    if (depth == 0) {
                        start = head + 1;
                        break;
                    }

                    while (IsSeparator(*head)) {
                        --head;
                    }

                    end = head;
                    len = 0;
                    --depth;
                }

                ++len;
                --head;
            }

            R_UNLESS(depth == 0, fs::ResultDirectoryUnobtainable());

            if (head == p) {
                start = p + 1;
            }
        }

        if (end <= p) {
            out->path   = p;
            out->length = 0;
        } else {
            out->path   = start;
            out->length = end - start + 1;
        }

        return ResultSuccess();
    }

}
