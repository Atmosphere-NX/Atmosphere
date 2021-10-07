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

    namespace {

        template<typename T>
        concept IsInAddr = std::same_as<T, ams::socket::InAddr> || std::same_as<T, struct in_addr>;

        template<typename T> requires IsInAddr<T>
        size_t SizeOfImpl(const T &) {
            return sizeof(u32);
        }

        template<typename T> requires IsInAddr<T>
        size_t SizeOfImpl(const T **in, u32 &out_count) {
            size_t rc = sizeof(u32);
            out_count = 0;

            if (in != nullptr) {
                for (const T ** cur_addr = in; *cur_addr != nullptr; ++cur_addr) {
                    ++out_count;
                    rc += sizeof(u32);
                }
            }

            return rc;
        }

        template<typename T> requires IsInAddr<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, const T &in) {
            ssize_t rc = -1;
            u8 *cur = dst;

            const u32 val = DNSSerializer::InternalHton(in.s_addr);

            if ((rc = DNSSerializer::CheckToBufferArguments(cur, dst_size, sizeof(in), __LINE__)) == -1) {
                return rc;
            }

            std::memcpy(cur, std::addressof(val), sizeof(val));
            rc += sizeof(val);

            return rc;
        }

        template<typename T> requires IsInAddr<T>
        ssize_t FromBufferImpl(T &out, const u8 *src, size_t src_size) {
            ssize_t rc = -1;
            if (src_size < sizeof(out)) {
                return rc;
            }

            std::memset(std::addressof(out), 0, sizeof(out));
            out.s_addr = DNSSerializer::InternalNtoh(*reinterpret_cast<const u32 *>(src));

            rc = sizeof(u32);
            return rc;
        }

        template<typename T> requires IsInAddr<T>
        ssize_t ToBufferImpl(u8 * const dst, size_t dst_size, T **arr) {
            ssize_t rc = -1;
            u8 *cur = dst;

            if (arr == nullptr && dst_size < sizeof(u32)) {
                return rc;
            } else if (arr == nullptr) {
                const u32 null = 0;
                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), null)) == -1) {
                    return rc;
                }
                cur += rc;
            } else {
                u32 count = 0;
                for (auto *tmp = arr; *tmp != nullptr; ++tmp) {
                    ++count;
                }

                if ((rc = DNSSerializer::CheckToBufferArguments(cur, dst_size, (count + 1) * sizeof(**arr), __LINE__)) == -1) {
                    return rc;
                }

                if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), count)) == -1) {
                    return rc;
                }
                cur += rc;

                rc = 0;
                for (auto i = 0; arr[i] != nullptr; ++i) {
                    const T addr = *arr[i];
                    if ((rc = DNSSerializer::ToBuffer(cur, dst_size - (cur - dst), addr)) == -1) {
                        return rc;
                    }

                    cur += rc;
                }

                rc = cur - dst;
            }

            return rc;
        }

        template<typename T> requires IsInAddr<T>
        ssize_t FromBufferImpl(T **&out, const u8 *src, size_t src_size) {
            ssize_t rc = -1;
            const u8 *cur = src;

            out = nullptr;

            ON_SCOPE_EXIT {
                if (rc == -1 && out != nullptr) {
                    for (auto i = 0; out[i] != nullptr; ++i) {
                        ams::socket::impl::Free(out[i]);
                        out[i] = nullptr;
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
            }

            u32 count = 0;
            if ((rc = DNSSerializer::FromBuffer(count, cur, src_size)) == -1) {
                return rc;
            }
            cur += rc;

            if (count == 0) {
                return rc;
            }

            out = static_cast<T **>(ams::socket::impl::Alloc((count + 1) * sizeof(T *)));
            if (out == nullptr) {
                rc = -1;
                return rc;
            }
            std::memset(out, 0, (count + 1) * sizeof(T *));

            for (u32 i = 0; i < count; ++i) {
                out[i] = static_cast<T *>(ams::socket::impl::Alloc(sizeof(T)));
                if (out[i] == nullptr) {
                    rc = -1;
                    return rc;
                }

                u32 s_addr = 0;
                if ((rc = DNSSerializer::FromBuffer(s_addr, cur, src_size - (cur - src))) == -1) {
                    return rc;
                }
                out[i]->s_addr = s_addr;
                cur += rc;
            }
            out[count] = nullptr;

            rc = cur - src;
            return rc;
        }

    }

    template<> size_t DNSSerializer::SizeOf(const struct in_addr &in) {
        return SizeOfImpl(in);
    }

    template<> size_t DNSSerializer::SizeOf(const ams::socket::InAddr &in) {
        return SizeOfImpl(in);
    }

    template<> size_t DNSSerializer::SizeOf(const struct in_addr **in, u32 &out_count) {
        return SizeOfImpl(in, out_count);
    }

    template<> size_t DNSSerializer::SizeOf(const ams::socket::InAddr **in, u32 &out_count) {
        return SizeOfImpl(in, out_count);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const struct in_addr &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, const ams::socket::InAddr &in) {
        return ToBufferImpl(dst, dst_size, in);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct in_addr &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct ams::socket::InAddr &out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, struct in_addr **arr) {
        return ToBufferImpl(dst, dst_size, arr);
    }

    template<> ssize_t DNSSerializer::ToBuffer(u8 * const dst, size_t dst_size, ams::socket::InAddr **arr) {
        return ToBufferImpl(dst, dst_size, arr);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct in_addr **&out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

    template<> ssize_t DNSSerializer::FromBuffer(struct ams::socket::InAddr **&out, const u8 *src, size_t src_size) {
        return FromBufferImpl(out, src, src_size);
    }

}
