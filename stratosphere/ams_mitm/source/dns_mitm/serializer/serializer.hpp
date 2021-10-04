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

namespace ams::mitm::socket::resolver::serializer {

    class DNSSerializer {
        public:
            static ssize_t CheckToBufferArguments(const u8 *dst, size_t dst_size, size_t required, int error_id);

            static u32 InternalHton(const u32 &v);
            static u16 InternalHton(const u16 &v);
            static u32 InternalNtoh(const u32 &v);
            static u16 InternalNtoh(const u16 &v);
        public:
            template<typename T>
            static size_t SizeOf(const T &in);

            template<typename T>
            static size_t SizeOf(const T *in);

            template<typename T>
            static size_t SizeOf(const T *in, size_t count);

            template<typename T>
            static size_t SizeOf(const T **arr, u32 &out_count);

            template<typename T>
            static ssize_t ToBuffer(u8 * const dst, size_t dst_size, const T &in);

            template<typename T>
            static ssize_t FromBuffer(T &out, const u8 *src, size_t src_size);

            template<typename T>
            static ssize_t ToBuffer(u8 * const dst, size_t dst_Size, T *in);

            template<typename T>
            static ssize_t ToBuffer(u8 * const dst, size_t dst_size, T **arr);

            template<typename T>
            static ssize_t FromBuffer(T *&out, const u8 *src, size_t src_size);

            template<typename T>
            static ssize_t FromBuffer(T **&out_arr, const u8 *src, size_t src_size);

            template<typename T>
            static ssize_t ToBuffer(u8 * const dst, size_t dst_size, const T * const arr, size_t count);

            template<typename T>
            static ssize_t FromBuffer(T * const arr, size_t arr_size, const u8 *src, size_t src_size, size_t count);
    };

    void FreeHostent(ams::socket::HostEnt &ent);
    void FreeHostent(struct hostent &ent);

    void FreeAddrInfo(ams::socket::AddrInfo &addr_info);
    void FreeAddrInfo(struct addrinfo &addr_info);

}
