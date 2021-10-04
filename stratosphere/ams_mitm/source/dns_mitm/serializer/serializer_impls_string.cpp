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
#include "../dnsmitm_debug.hpp"
#include "../socket_allocator.hpp"
#include "serializer.hpp"

namespace ams::mitm::socket::resolver::serializer {

    template<>  size_t DNSSerializer::SizeOf(const char *str) {
        if (str == nullptr) {
            return sizeof(char);
        }
        return std::strlen(str) + 1;
    }

    template<>  size_t DNSSerializer::SizeOf(const char **str, u32 &out_count) {
        size_t rc = sizeof(u32);
        out_count = 0;

        if (str != nullptr) {
            for (const char **cur = str; *cur != nullptr; ++cur) {
                ++out_count;
                rc += SizeOf(*cur);
            }
        }

        return rc;
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, char *str) {
        ssize_t rc = -1;
        u8 *cur = dst;

        if (str == nullptr && dst_size == 0) {
            return -1;
        } else if (str == nullptr) {
            *cur = '\x00';
            return 1;
        } else if ((rc = SizeOf(static_cast<const char *>(str))) == 0) {
            *cur = '\x00';
            return 1;
        } else if (CheckToBufferArguments(cur, dst_size, rc + 1, __LINE__) == -1) {
            return -1;
        }

        std::memmove(cur, str, rc);
        return rc;
    }

    template<> ssize_t DNSSerializer::FromBuffer(char *&out, const u8 *src, size_t src_size) {
        size_t len = 0;

        if (src == nullptr) {
            return 0;
        } else if (src_size == 0) {
            return 0;
        } else if (src_size < (len = SizeOf(reinterpret_cast<const char *>(src)))) {
            return 1;
        } else if (src[0] == '\x00') {
            return 1;
        }

        out = static_cast<char *>(ams::socket::impl::Alloc(len));
        if (out == nullptr) {
            return -1;
        }

        std::memmove(out, src, len);
        return len;
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, char **str) {
        ssize_t rc = -1;
        u8 *cur = dst;
        u32 count = 0;

        if (dst_size == 0) {
            return -1;
        }

        const size_t total_size = SizeOf(const_cast<const char **>(str), count);
        AMS_UNUSED(total_size);

        if ((rc = CheckToBufferArguments(cur, dst_size, sizeof(u32), __LINE__)) == -1) {
            return rc;
        } else if ((rc = ToBuffer(cur, dst_size, count)) == -1) {
            return rc;
        }
        cur      += rc;
        dst_size -= rc;

        if (str != nullptr) {
            for (char **cur_str = str; *cur_str != nullptr; ++cur_str) {
                const auto tmp = ToBuffer(cur, dst_size, *cur_str);
                if (tmp == -1) {
                    return rc;
                }

                cur      += tmp;
                dst_size -= tmp;
                rc       += tmp;
            }
        }

        rc = cur - dst;
        return rc;
    }

    template<> ssize_t DNSSerializer::FromBuffer(char **&out, const u8 *src, size_t src_size) {
        ssize_t rc = -1;
        const u8 *cur = src;
        u32 count = 0;

        out = nullptr;

        ON_SCOPE_EXIT {
            if (rc < 0 && out != nullptr) {
                u32 i = 0;
                for (char *str = *out; str != nullptr; str = out[++i]) {
                    ams::socket::impl::Free(str);
                }
                ams::socket::impl::Free(out);
                out = nullptr;
            }
        };

        if (src == nullptr) {
            rc = 0;
            return rc;
        } else if (src_size == 0) {
            rc = 0;
            return rc;
        } else if ((rc = FromBuffer(count, cur, src_size)) == -1) {
            rc = -1;
            return rc;
        }
        cur += rc;

        out = static_cast<char **>(ams::socket::impl::Alloc((count + 1) * sizeof(char *)));
        if (out == nullptr) {
            rc = -1;
            return rc;
        }
        std::memset(out, 0, (count + 1) * sizeof(char *));

        u32 i;
        for (i = 0; i < count; ++i) {
            const size_t len = std::strlen(reinterpret_cast<const char *>(cur));
            out[i] = static_cast<char *>(ams::socket::impl::Alloc(len + 1));
            if (out[i] == nullptr) {
                rc = -1;
                return rc;
            }

            std::memmove(out[i], cur, len + 1);
            cur += len + 1;
        }

        out[i] = nullptr;
        rc = cur - src;

        return rc;
    }


}
