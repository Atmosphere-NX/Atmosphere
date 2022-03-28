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

namespace ams::fs::RomPathTool {

    Result PathParser::Initialize(const RomPathChar *path) {
        AMS_ASSERT(path != nullptr);

        /* Require paths start with a separator, and skip repeated separators. */
        R_UNLESS(RomPathTool::IsSeparator(path[0]), fs::ResultDbmInvalidPathFormat());
        while (RomPathTool::IsSeparator(path[1])) {
            ++path;
        }

        m_prev_path_start = path;
        m_prev_path_end   = path;
        m_next_path       = path + 1;

        R_SUCCEED();
    }

    void PathParser::Finalize() {
        m_prev_path_start = nullptr;
        m_prev_path_end   = nullptr;
        m_next_path       = nullptr;
        m_finished        = false;
    }

    bool PathParser::IsParseFinished() const {
        return m_finished;
    }

    bool PathParser::IsDirectoryPath() const {
        AMS_ASSERT(m_next_path != nullptr);

        if (RomPathTool::IsNullTerminator(m_next_path[0]) && RomPathTool::IsSeparator(m_next_path[-1])) {
            return true;
        }

        if (RomPathTool::IsCurrentDirectory(m_next_path)) {
            return true;
        }

        return RomPathTool::IsParentDirectory(m_next_path);
    }

    Result PathParser::GetNextDirectoryName(RomEntryName *out) {
        AMS_ASSERT(m_prev_path_start != nullptr);
        AMS_ASSERT(m_prev_path_end   != nullptr);
        AMS_ASSERT(m_next_path       != nullptr);
        AMS_ASSERT(out != nullptr);

        /* Get as directory name. */
        R_TRY(this->GetAsDirectoryName(out));

        /* Parse the next path. */
        const RomPathChar *cur = m_next_path;
        size_t name_len;
        for (name_len = 0; !RomPathTool::IsSeparator(cur[name_len]); ++name_len) {
            if (RomPathTool::IsNullTerminator(cur[name_len])) {
                m_finished        = true;
                m_prev_path_start = m_next_path;
                m_next_path       = cur + name_len;
                m_prev_path_end   = cur + name_len;
                R_SUCCEED();
            }
        }

        /* Advance past separators. */
        m_prev_path_start = m_next_path;
        m_prev_path_end   = cur + name_len;
        for (m_next_path = m_prev_path_end + 1; RomPathTool::IsSeparator(m_next_path[0]); ++m_next_path) {
            /* ... */
        }

        /* Check if we're finished. */
        if (RomPathTool::IsNullTerminator(m_next_path[0])) {
            m_finished = true;
        }

        R_SUCCEED();
    }

    Result PathParser::GetAsDirectoryName(RomEntryName *out) const {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(m_prev_path_start != nullptr);
        AMS_ASSERT(m_prev_path_end   != nullptr);
        AMS_ASSERT(m_next_path       != nullptr);

        AMS_ASSERT(m_prev_path_start <= m_prev_path_end);

        const size_t len = m_prev_path_end - m_prev_path_start;
        R_UNLESS(len <= MaxPathLength, fs::ResultDbmDirectoryNameTooLong());

        out->Initialize(m_prev_path_start, len);
        R_SUCCEED();
    }

    Result PathParser::GetAsFileName(RomEntryName *out) const {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(m_prev_path_start != nullptr);
        AMS_ASSERT(m_prev_path_end   != nullptr);
        AMS_ASSERT(m_next_path       != nullptr);

        AMS_ASSERT(m_prev_path_start <= m_prev_path_end);

        const size_t len = m_prev_path_end - m_prev_path_start;
        R_UNLESS(len <= MaxPathLength, fs::ResultDbmFileNameTooLong());

        out->Initialize(m_prev_path_start, len);
        R_SUCCEED();
    }

    Result GetParentDirectoryName(RomEntryName *out, const RomEntryName &cur, const RomPathChar *p) {
        AMS_ASSERT(out != nullptr);
        AMS_ASSERT(p != nullptr);

        const RomPathChar *start = cur.begin();
        const RomPathChar *end   = cur.end() - 1;

        s32 depth = 1;
        if (cur.IsParentDirectory()) {
            ++depth;
        }

        if (start > p) {
            size_t len = 0;
            for (const RomPathChar *head = start - 1; head >= p; --head) {
                if (RomPathTool::IsSeparator(*head)) {
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

                    do {
                        --head;
                    } while (head > p && RomPathTool::IsSeparator(*head));

                    end = head;
                    len = 0;
                    --depth;
                }

                ++len;
            }

            R_UNLESS(depth == 0, fs::ResultDirectoryUnobtainable());
        }

        if (end <= p) {
            out->Initialize(p, 0);
        } else {
            out->Initialize(start, end - start + 1);
        }

        R_SUCCEED();
    }

}
