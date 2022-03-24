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
#include <stratosphere/fs/fs_common.hpp>
#include <stratosphere/fs/fs_path.hpp>

namespace ams::fs {

    /* ACCURATE_TO_VERSION: 13.4.0.0 */
    class DirectoryPathParser {
        NON_COPYABLE(DirectoryPathParser);
        NON_MOVEABLE(DirectoryPathParser);
        private:
            char *m_buffer;
            char m_replaced_char;
            s32 m_position;
            fs::Path m_current_path;
        public:
            DirectoryPathParser() : m_buffer(nullptr), m_replaced_char(StringTraits::NullTerminator), m_position(0), m_current_path() { /* ... */ }

            const fs::Path &GetCurrentPath() const {
                return m_current_path;
            }

            Result Initialize(fs::Path *path) {
                /* Declare a default buffer, in case the path has no write buffer. */
                static constinit char EmptyBuffer[1] = "";

                /* Get a usable buffer. */
                char *buf = path->GetWriteBufferLength() > 0 ? path->GetWriteBuffer() : EmptyBuffer;

                /* Get the windows skip length. */
                const auto windows_skip_len = fs::GetWindowsSkipLength(buf);

                /* Set our buffer. */
                m_buffer = buf + windows_skip_len;

                /* Set up our initial state. */
                if (windows_skip_len) {
                    R_TRY(m_current_path.InitializeWithNormalization(buf, windows_skip_len + 1));
                } else {
                    if (char * const first = this->ReadNextImpl(); first != nullptr) {
                        R_TRY(m_current_path.InitializeWithNormalization(first));
                    }
                }

                R_SUCCEED();
            }

            Result ReadNext(bool *out_finished) {
                /* Default to not finished. */
                *out_finished = false;

                /* Get the next child component. */
                if (auto * const child = this->ReadNextImpl(); child != nullptr) {
                    /* Append the child component to our current path. */
                    R_TRY(m_current_path.AppendChild(child));
                } else {
                    /* We have no child component, so we're finished. */
                    *out_finished = true;
                }

                R_SUCCEED();
            }
        private:
            char *ReadNextImpl() {
                /* Check that we have characters to read. */
                if (m_position < 0 || m_buffer[0] == StringTraits::NullTerminator) {
                    return nullptr;
                }

                /* If we have a replaced character, restore it. */
                if (m_replaced_char != StringTraits::NullTerminator) {
                    m_buffer[m_position] = m_replaced_char;
                    if (m_replaced_char == StringTraits::DirectorySeparator) {
                        ++m_position;
                    }
                }

                /* If we're at the start of a root-relative path, begin by returning the root. */
                if (m_position == 0 && m_buffer[0] == StringTraits::DirectorySeparator) {
                    m_replaced_char = m_buffer[1];
                    m_buffer[1]     = StringTraits::NullTerminator;
                    m_position      = 1;
                    return m_buffer;
                }

                /* Otherwise, find the end of the next path component. */
                s32 i;
                for (i = m_position; m_buffer[i] != StringTraits::DirectorySeparator; ++i) {
                    if (m_buffer[i] == StringTraits::NullTerminator) {
                        char * const ret = (i != m_position) ? m_buffer + m_position : nullptr;
                        m_position = -1;
                        return ret;
                    }
                }

                /* Sanity check that we're not ending on a separator. */
                AMS_ASSERT(m_buffer[i + 1] != StringTraits::NullTerminator);

                char * const ret = m_buffer + m_position;

                m_replaced_char = StringTraits::DirectorySeparator;
                m_buffer[i]     = 0;
                m_position      = i;

                return ret;
            }
    };

}
