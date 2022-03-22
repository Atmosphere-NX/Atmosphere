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

namespace ams::htcfs {

    class FileServiceObject {
        private:
            s32 m_handle;
        public:
            explicit FileServiceObject(s32 handle);
            ~FileServiceObject();
        public:
            Result ReadFile(ams::sf::Out<s64> out, s64 offset, const ams::sf::OutNonSecureBuffer &buffer, ams::fs::ReadOption option);
            Result WriteFile(s64 offset, const ams::sf::InNonSecureBuffer &buffer, ams::fs::WriteOption option);
            Result GetFileSize(ams::sf::Out<s64> out);
            Result SetFileSize(s64 size);
            Result FlushFile();
            Result SetPriorityForFile(s32 priority);
            Result GetPriorityForFile(ams::sf::Out<s32> out);
    };
    static_assert(tma::IsIFileAccessor<FileServiceObject>);

}
