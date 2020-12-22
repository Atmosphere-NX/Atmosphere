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
#pragma once
#include <stratosphere.hpp>

namespace ams::erpt::srv {

    enum StreamMode {
        StreamMode_Write   = 0,
        StreamMode_Read    = 1,
        StreamMode_Invalid = 2,
    };

    class Stream {
        private:
            static bool s_can_access_fs;
        private:
            u32 buffer_size;
            u32 file_position;
            u32 buffer_position;
            u32 buffer_count;
            u8 *buffer;
            StreamMode stream_mode;
            bool initialized;
            fs::FileHandle file_handle;
            char file_name[ReportFileNameLength];
        public:
            Stream();
            ~Stream();

            Result OpenStream(const char *path, StreamMode mode, u32 buffer_size);
            Result ReadStream(u32 *out, u8 *dst, u32 dst_size);
            Result WriteStream(const u8 *src, u32 src_size);
            void CloseStream();

            Result GetStreamSize(s64 *out) const;
        private:
            Result Flush();
        public:
            static void EnableFsAccess(bool en);
            static Result DeleteStream(const char *path);
            static Result CommitStream();
            static Result GetStreamSize(s64 *out, const char *path);
    };

}
