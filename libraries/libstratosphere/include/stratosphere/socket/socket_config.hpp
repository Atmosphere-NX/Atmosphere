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
#include <vapours.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/socket/socket_constants.hpp>

namespace ams::socket {

    constexpr ALWAYS_INLINE size_t AlignMss(size_t size) {
        return util::DivideUp(size, static_cast<size_t>(1500)) * static_cast<size_t>(1500);
    }

    class Config {
        private:
            u32 m_version;
        protected:
            bool m_system;
            bool m_smbp;
            void *m_memory_pool;
            size_t m_memory_pool_size;
            size_t m_allocator_pool_size;
            size_t m_tcp_initial_send_buffer_size;
            size_t m_tcp_initial_receive_buffer_size;
            size_t m_tcp_auto_send_buffer_size_max;
            size_t m_tcp_auto_receive_buffer_size_max;
            size_t m_udp_send_buffer_size;
            size_t m_udp_receive_buffer_size;
            int m_sb_efficiency;
            int m_concurrency_count_max;
        public:
            constexpr Config(void *mp, size_t mp_sz, size_t ap, size_t is, size_t ir, size_t as, size_t ar, size_t us, size_t ur, int sbe, int c)
                : m_version(LibraryVersion),
                  m_system(false),
                  m_smbp(false),
                  m_memory_pool(mp),
                  m_memory_pool_size(mp_sz),
                  m_allocator_pool_size(ap),
                  m_tcp_initial_send_buffer_size(is),
                  m_tcp_initial_receive_buffer_size(ir),
                  m_tcp_auto_send_buffer_size_max(as),
                  m_tcp_auto_receive_buffer_size_max(ar),
                  m_udp_send_buffer_size(us),
                  m_udp_receive_buffer_size(ur),
                  m_sb_efficiency(sbe),
                  m_concurrency_count_max(c)
            {
                /* ... */
            }

            constexpr u32 GetVersion() const { return m_version; }
            constexpr bool IsSystemClient() const { return m_system; }
            constexpr bool IsSmbpClient() const { return m_smbp; }
            constexpr void *GetMemoryPool() const { return m_memory_pool; }
            constexpr size_t GetMemoryPoolSize() const { return m_memory_pool_size; }
            constexpr size_t GetAllocatorPoolSize() const { return m_allocator_pool_size; }
            constexpr size_t GetTcpInitialSendBufferSize() const { return m_tcp_initial_send_buffer_size; }
            constexpr size_t GetTcpInitialReceiveBufferSize() const { return m_tcp_initial_receive_buffer_size; }
            constexpr size_t GetTcpAutoSendBufferSizeMax() const { return m_tcp_auto_send_buffer_size_max; }
            constexpr size_t GetTcpAutoReceiveBufferSizeMax() const { return m_tcp_auto_receive_buffer_size_max; }
            constexpr size_t GetUdpSendBufferSize() const { return m_udp_send_buffer_size; }
            constexpr size_t GetUdpReceiveBufferSize() const { return m_udp_receive_buffer_size; }
            constexpr int GetSocketBufferEfficiency() const { return m_sb_efficiency; }
            constexpr int GetConcurrencyCountMax() const { return m_concurrency_count_max; }

            constexpr void SetTcpInitialSendBufferSize(size_t size) { m_tcp_initial_send_buffer_size = size; }
            constexpr void SetTcpInitialReceiveBufferSize(size_t size) { m_tcp_initial_receive_buffer_size = size; }
            constexpr void SetTcpAutoSendBufferSizeMax(size_t size) { m_tcp_auto_send_buffer_size_max = size; }
            constexpr void SetTcpAutoReceiveBufferSizeMax(size_t size) { m_tcp_auto_receive_buffer_size_max = size; }
            constexpr void SetUdpSendBufferSize(size_t size) { m_udp_send_buffer_size = size; }
            constexpr void SetUdpReceiveBufferSize(size_t size) { m_udp_receive_buffer_size = size; }
            constexpr void SetSocketBufferEfficiency(int sb) { AMS_ABORT_UNLESS(1 <= sb && sb <= 8); m_sb_efficiency = sb; }
            constexpr void SetConcurrencyCountMax(int c) { m_concurrency_count_max = c; }
    };

}
