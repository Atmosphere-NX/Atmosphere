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
#include <exosphere.hpp>
#include "fusee_ini.hpp"
#include "fusee_malloc.hpp"
#include "fs/fusee_fs_api.hpp"

namespace ams::nxboot {

    namespace {

        constexpr s64 IniFileSizeMax = 64_KB;

        constexpr bool IsWhiteSpace(char c) {
            return c == ' ' || c == '\r';
        }

    }

    ParseIniResult ParseIniFile(IniSectionList &out_sections, const char *ini_path) {
        /* Open the ini file. */
        fs::FileHandle file;
        if (R_FAILED(fs::OpenFile(std::addressof(file), ini_path, fs::OpenMode_Read))) {
            return ParseIniResult_NoFile;
        }

        ON_SCOPE_EXIT { fs::CloseFile(file); };

        /* Get file size. */
        s64 file_size;
        if (R_FAILED(fs::GetFileSize(std::addressof(file_size), file))) {
            return ParseIniResult_NoFile;
        }

        /* Cap file size. */
        file_size = std::min(IniFileSizeMax, file_size);

        /* Allocate memory for file. */
        char *buffer = static_cast<char *>(AllocateAligned(util::AlignUp(file_size + 1, 0x10), 0x10));
        buffer[file_size] = '\x00';

        /* Read file. */
        if (R_FAILED(fs::ReadFile(file, 0, buffer, file_size))) {
            return ParseIniResult_NoFile;
        }

        /* Parse the file. */
        enum class State {
            Newline,
            Comment,
            SectionName,
            Key,
            KvSpace,
            KvSpace2,
            Value,
            TrailingSpace,
        };

        char *sec_start, *key_start, *val_start, *val_end;
        IniSection *cur_sec = nullptr;

        State state = State::Newline;
        for (int i = 0; i < file_size; ++i) {
            const char c = buffer[i];

            switch (state) {
                case State::Newline:
                    if (c == '[') {
                        sec_start = buffer + i + 1;
                        state = State::SectionName;
                    } else if (c == ';' || c == '#') {
                        state = State::Comment;
                    } else if (IsWhiteSpace(c) || c == '\n') {
                        state = State::Newline;
                    } else if (cur_sec != nullptr) {
                        key_start = buffer + i;
                        state = State::Key;
                    } else {
                        return ParseIniResult_InvalidFormat;
                    }
                    break;
                case State::Comment:
                    if (c == '\n') {
                        state = State::Newline;
                    }
                    break;
                case State::SectionName:
                    if (c == '\n') {
                        return ParseIniResult_InvalidFormat;
                    } else if (c == ']') {
                        cur_sec = AllocateObject<IniSection>();

                        cur_sec->name = sec_start;
                        buffer[i] = '\x00';

                        out_sections.push_back(*cur_sec);

                        state = State::TrailingSpace;
                    }
                    break;
                case State::Key:
                    if (c == '\n') {
                        return ParseIniResult_InvalidFormat;
                    } else if (IsWhiteSpace(c)) {
                        buffer[i] = '\x00';

                        state = State::KvSpace;
                    } else if (c == '=') {
                        buffer[i] = '\x00';

                        state = State::KvSpace2;
                    }
                    break;
                case State::KvSpace:
                    if (c == '=') {
                        state = State::KvSpace2;
                    } else if (!IsWhiteSpace(c)) {
                        return ParseIniResult_InvalidFormat;
                    }
                    break;
                case State::KvSpace2:
                    if (c == '\n') {
                        buffer[i] = '\x00';

                        auto *entry = AllocateObject<IniKeyValueEntry>();
                        entry->key   = key_start;
                        entry->value = buffer + i;

                        cur_sec->kv_list.push_back(*entry);

                        state = State::Newline;
                    } else if (!IsWhiteSpace(c)) {
                        val_start = buffer + i;
                        val_end   = buffer + i + 1;

                        state = State::Value;
                    }
                    break;
                case State::Value:
                    if (c == '\r' || c == '\n') {
                        buffer[i] = '\x00';
                        *val_end  = '\x00';

                        auto *entry = AllocateObject<IniKeyValueEntry>();
                        entry->key   = key_start;
                        entry->value = val_start;

                        cur_sec->kv_list.push_back(*entry);

                        state = (c == '\n') ? State::Newline : State::TrailingSpace;
                    } else if (c != ' ') {
                        val_end = buffer + i + 1;
                    }
                    break;
                case State::TrailingSpace:
                    if (c == '\n') {
                        state = State::Newline;
                    } else if (!IsWhiteSpace(c)) {
                        return ParseIniResult_InvalidFormat;
                    }
                    break;
            }
        }

        /* Accept value-state. */
        if (state == State::Value) {
            auto *entry = AllocateObject<IniKeyValueEntry>();
            entry->key   = key_start;
            entry->value = val_start;

            cur_sec->kv_list.push_back(*entry);

            return ParseIniResult_Success;
        } else if (state == State::TrailingSpace || state == State::Comment || state == State::Newline) {
            return ParseIniResult_Success;
        } else {
            return ParseIniResult_InvalidFormat;
        }


    }

}
