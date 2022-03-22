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
#include <stratosphere/socket/socket_config.hpp>

namespace ams::socket {

    class SystemConfigDefault : public Config {
        public:
            static constexpr size_t DefaultTcpInitialSendBufferSize     = 32_KB;
            static constexpr size_t DefaultTcpInitialReceiveBufferSize  = 64_KB;
            static constexpr size_t DefaultTcpAutoSendBufferSizeMax     = 256_KB;
            static constexpr size_t DefaultTcpAutoReceiveBufferSizeMax  = 256_KB;
            static constexpr size_t DefaultUdpSendBufferSize            = 9_KB;
            static constexpr size_t DefaultUdpReceiveBufferSize         = 42240;
            static constexpr auto   DefaultSocketBufferEfficiency       = 2;
            static constexpr auto   DefaultConcurrency                  = 8;
            static constexpr size_t DefaultAllocatorPoolSize            = 128_KB;

            static constexpr size_t PerTcpSocketWorstCaseMemoryPoolSize = [] {
                constexpr size_t WorstCaseTcpSendBufferSize    = AlignMss(std::max(DefaultTcpInitialSendBufferSize, DefaultTcpAutoSendBufferSizeMax));
                constexpr size_t WorstCaseTcpReceiveBufferSize = AlignMss(std::max(DefaultTcpInitialReceiveBufferSize, DefaultTcpAutoReceiveBufferSizeMax));

                return util::AlignUp(WorstCaseTcpSendBufferSize * DefaultSocketBufferEfficiency + WorstCaseTcpReceiveBufferSize * DefaultSocketBufferEfficiency, os::MemoryPageSize);
            }();

            static constexpr size_t PerUdpSocketWorstCaseMemoryPoolSize = [] {
                constexpr size_t WorstCaseUdpSendBufferSize    = AlignMss(DefaultUdpSendBufferSize);
                constexpr size_t WorstCaseUdpReceiveBufferSize = AlignMss(DefaultUdpReceiveBufferSize);

                return util::AlignUp(WorstCaseUdpSendBufferSize * DefaultSocketBufferEfficiency + WorstCaseUdpReceiveBufferSize * DefaultSocketBufferEfficiency, os::MemoryPageSize);
            }();
        public:
            constexpr SystemConfigDefault(void *mp, size_t mp_sz, size_t ap, int c=DefaultConcurrency)
                : Config(mp, mp_sz, ap,
                         DefaultTcpInitialSendBufferSize, DefaultTcpInitialReceiveBufferSize,
                         DefaultTcpAutoSendBufferSizeMax, DefaultTcpAutoReceiveBufferSizeMax,
                         DefaultUdpSendBufferSize, DefaultUdpReceiveBufferSize,
                         DefaultSocketBufferEfficiency, c)
            {
                /* Mark as system. */
                m_system = true;
            }
    };

    class SystemConfigLightDefault : public Config {
        public:
            static constexpr size_t DefaultTcpInitialSendBufferSize     = 16_KB;
            static constexpr size_t DefaultTcpInitialReceiveBufferSize  = 16_KB;
            static constexpr size_t DefaultTcpAutoSendBufferSizeMax     = 0_KB;
            static constexpr size_t DefaultTcpAutoReceiveBufferSizeMax  = 0_KB;
            static constexpr size_t DefaultUdpSendBufferSize            = 9_KB;
            static constexpr size_t DefaultUdpReceiveBufferSize         = 42240;
            static constexpr auto   DefaultSocketBufferEfficiency       = 2;
            static constexpr auto   DefaultConcurrency                  = 2;
            static constexpr size_t DefaultAllocatorPoolSize            = 64_KB;

            static constexpr size_t PerTcpSocketWorstCaseMemoryPoolSize = [] {
                constexpr size_t WorstCaseTcpSendBufferSize    = AlignMss(std::max(DefaultTcpInitialSendBufferSize, DefaultTcpAutoSendBufferSizeMax));
                constexpr size_t WorstCaseTcpReceiveBufferSize = AlignMss(std::max(DefaultTcpInitialReceiveBufferSize, DefaultTcpAutoReceiveBufferSizeMax));

                return util::AlignUp(WorstCaseTcpSendBufferSize * DefaultSocketBufferEfficiency + WorstCaseTcpReceiveBufferSize * DefaultSocketBufferEfficiency, os::MemoryPageSize);
            }();

            static constexpr size_t PerUdpSocketWorstCaseMemoryPoolSize = [] {
                constexpr size_t WorstCaseUdpSendBufferSize    = AlignMss(DefaultUdpSendBufferSize);
                constexpr size_t WorstCaseUdpReceiveBufferSize = AlignMss(DefaultUdpReceiveBufferSize);

                return util::AlignUp(WorstCaseUdpSendBufferSize * DefaultSocketBufferEfficiency + WorstCaseUdpReceiveBufferSize * DefaultSocketBufferEfficiency, os::MemoryPageSize);
            }();
        public:
            constexpr SystemConfigLightDefault(void *mp, size_t mp_sz, size_t ap, int c=DefaultConcurrency)
                : Config(mp, mp_sz, ap,
                         DefaultTcpInitialSendBufferSize, DefaultTcpInitialReceiveBufferSize,
                         DefaultTcpAutoSendBufferSizeMax, DefaultTcpAutoReceiveBufferSizeMax,
                         DefaultUdpSendBufferSize, DefaultUdpReceiveBufferSize,
                         DefaultSocketBufferEfficiency, c)
            {
                /* Mark as system. */
                m_system = true;
            }
    };

}
