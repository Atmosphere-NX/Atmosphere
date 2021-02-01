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
#include "socket_api.hpp"
#include "socket_allocator.hpp"

namespace ams::socket::impl {

    void *Alloc(size_t size) {
        /* TODO: expheap, heap generation. */
        return ams::Malloc(size);
    }

    void *Calloc(size_t num, size_t size) {
        if (void *ptr = Alloc(size * num); ptr != nullptr) {
            std::memset(ptr, 0, size * num);
            return ptr;
        } else {
            return nullptr;
        }
    }

    void Free(void *ptr) {
        /* TODO: expheap, heap generation. */
        return ams::Free(ptr);
    }

    Errno GetLastError() {
        /* TODO: check that client library is initialized. */
        return static_cast<Errno>(errno);
    }

    void SetLastError(Errno err) {
        /* TODO: check that client library is initialized. */
        errno = static_cast<int>(err);
    }

    u32 InetHtonl(u32 host) {
        if constexpr (util::IsBigEndian()) {
            return host;
        } else {
            return util::SwapBytes(host);
        }
    }

    u16 InetHtons(u16 host) {
        if constexpr (util::IsBigEndian()) {
            return host;
        } else {
            return util::SwapBytes(host);
        }
    }

    u32 InetNtohl(u32 net) {
        if constexpr (util::IsBigEndian()) {
            return net;
        } else {
            return util::SwapBytes(net);
        }
    }

    u16 InetNtohs(u16 net) {
        if constexpr (util::IsBigEndian()) {
            return net;
        } else {
            return util::SwapBytes(net);
        }
    }

}
