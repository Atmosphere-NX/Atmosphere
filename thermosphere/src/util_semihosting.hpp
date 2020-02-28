/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

/* Initially from Arm TF */

/*
 * Copyright (c) 2013-2014, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#pragma once
#include "defines.hpp"
#include <string_view>

namespace ams::util {

    namespace impl {

        enum class SemihostingOperation : unsigned int {
            /* Only the operations we implement below are listed. */
            Open    = 1,
            Close   = 2,
            Write0  = 3,
            WriteC  = 4,
            Write   = 5,
            Read    = 6,
            ReadC   = 7,
            Seek    = 0xA,
            FLen    = 0xC,
            Remove  = 0xE,
            System  = 0x12,
            Errno   = 0x13
        };

        ALWAYS_INLINE long SemihostingCall(impl::SemihostingOperation op, const void *blk)
        {
            register long ret asm("x0");
            register impl::SemihostingOperation arg1 asm("w0") = op;
            register const void *arg2 asm("x1") = blk;
            __asm__ __volatile__("hlt 0xF000" : "=r" (ret) : "0" (arg1), "r" (arg2) : "memory");
            return ret;
        }
    }

    class SemihostingFile {
        NON_COPYABLE(SemihostingFile);
        private:
            /* A call to open can't return a 0 handle */
            long m_handle = 0;
        public:
            enum class OpenMode : unsigned long {
                R       = 0,
                RB      = 1,
                RPlus   = 2,
                RPlusB  = 3,
                W       = 4,
                WB      = 5,
                WPlus   = 6,
                WPlusB  = 7,
                A       = 8,
                AB      = 9,
                APlus   = 10,
                APlusB  = 11
            };

            constexpr bool IsOpen() const { return m_handle > 0; }

            long Open(std::string_view name, OpenMode mode)
            {
                /* std::string_view is implicitely constructible from const char* */
                struct {
                    const char *name;
                    OpenMode mode;
                    size_t nameSize;
                } blk = { name.data(), mode, name.size() };

                int ret = impl::SemihostingCall(impl::SemihostingOperation::Open, &blk);
                if (ret > 0) {
                    m_handle = ret;
                    return 0;
                } else {
                    m_handle = 0;
                    return -1;
                }
            }

            long Seek(ssize_t offset) const
            {
                struct {
                    long handle;
                    ssize_t offset;
                } blk = { m_handle, offset };

                return impl::SemihostingCall(impl::SemihostingOperation::Seek, &blk);
            }

            size_t Read(void *buf, size_t len) const
            {
                struct {
                    long handle;
                    void *buf;
                    size_t len;
                } blk = { m_handle, buf, len };
                long nrem = impl::SemihostingCall(impl::SemihostingOperation::Read, &blk);
                return len - nrem;
            }

            size_t Write(const void *buf, size_t len) const
            {
                struct {
                    long handle;
                    const void *buf;
                    size_t len;
                } blk = { m_handle, buf, len };
                long nrem = impl::SemihostingCall(impl::SemihostingOperation::Write, &blk);
                return len - nrem;
            }

            ssize_t GetSize() const
            {
                return impl::SemihostingCall(impl::SemihostingOperation::FLen, &m_handle);
            }

            long Close()
            {
                long ret = impl::SemihostingCall(impl::SemihostingOperation::Close, &m_handle);
                m_handle = 0;
                return ret;
            }

        public:
            constexpr SemihostingFile() = default;

            SemihostingFile(const char *name, OpenMode mode = OpenMode::RB)
            {
                Open(name, mode);
            }

            SemihostingFile(SemihostingFile &&) = default;
            SemihostingFile &operator=(SemihostingFile &&) = default;

            ~SemihostingFile()
            {
                Close();
            }

        public:
            static ssize_t GetFileSize(const char *name)
            {
                SemihostingFile file{name};
                return file.IsOpen() ? file.GetSize() : -1;
            }

            static ssize_t DownloadFile(void *buf, const char *name, size_t maxLen)
            {
                SemihostingFile file{name};
                return file.IsOpen() ? file.Read(buf, maxLen) : -1;
            }
    };

    long SemihostingSystem(std::string_view cmdLine)
    {
        /* std::string_view is implicitely constructible from const char* */
        struct {
            const char *str;
            size_t len;
        } blk = { cmdLine.data(), cmdLine.size() };
        return impl::SemihostingCall(impl::SemihostingOperation::System, &blk);
    }

    void SemihostingWriteChar(char c)
    {
        impl::SemihostingCall(impl::SemihostingOperation::WriteC, &c);
    }

    void SemihostingWriteString(const char *str)
    {
        /* Has to be null-terminated, sadly. */
        impl::SemihostingCall(impl::SemihostingOperation::Write0, str);
    }

    char SemihostingReadChar(void)
    {
        return static_cast<char>(impl::SemihostingCall(impl::SemihostingOperation::ReadC, nullptr));
    }

}