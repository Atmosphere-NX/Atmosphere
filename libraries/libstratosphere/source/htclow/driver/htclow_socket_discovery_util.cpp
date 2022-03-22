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
#include "htclow_socket_discovery_util.hpp"
#include "../ctrl/htclow_ctrl_settings_holder.hpp"

namespace ams::htclow::driver {

    namespace {

        constexpr inline u32 AutoConnectIpv4RequestServiceId = 0x834C775A;
        constexpr inline u32 BeaconResponseServiceId         = 0xA6F7FA96;

        constexpr const char MakeAutoConnectIpv4RequestPacketFormat[] =
          "{\r\n"
          "  \"Address\" : \"%u.%u.%u.%u\",\r\n"
          "  \"Port\" : %u,\r\n"
          "  \"HW\" : \"%s\",\r\n"
          "  \"SN\" : \"%s\"\r\n"
          "}\r\n";

        constexpr const char BeaconResponsePacketFormat[] =
          "{\r\n"
          "  \"Gen\" : 2,\r\n"
          "  \"Spec \": \"%s\",\r\n"
          "  \"MAC\" : \"00:00:00:00:00:00\",\r\n"
          "  \"Conn\" : \"TCP\",\r\n"
          "  \"HW\" : \"%s\",\r\n"
          "  \"Name\" : \"%s\",\r\n"
          "  \"SN\" : \"%s\",\r\n"
          "  \"FW\" : \"%s\"\r\n"
          "}\r\n";

        constinit os::SdkMutex g_settings_holder_mutex;
        constinit bool g_settings_holder_initialized = false;
        constinit htclow::ctrl::SettingsHolder g_settings_holder;

        void InitializeSettingsHolder() {
            std::scoped_lock lk(g_settings_holder_mutex);

            if (!g_settings_holder_initialized) {
                g_settings_holder.LoadSettings();
                g_settings_holder_initialized = true;
            }
        }

    }

    s32 MakeAutoConnectIpv4RequestPacket(char *dst, size_t dst_size, const socket::SockAddrIn &sockaddr) {
        /* Initialize the settings holder. */
        InitializeSettingsHolder();

        /* Create the packet header. */
        TmipcHeader header = {
            .service_id = AutoConnectIpv4RequestServiceId,
            .version    = TmipcVersion,
        };

        /* Create the packet body. */
        std::scoped_lock lk(g_settings_holder_mutex);
        const auto addr = sockaddr.sin_addr.s_addr;

        char packet_body[0x100];
        const auto ideal_len = util::SNPrintf(packet_body, sizeof(packet_body), MakeAutoConnectIpv4RequestPacketFormat,
                                              (addr >> 0) & 0xFF, (addr >> 8) & 0xFF, (addr >> 16) & 0xFF, (addr >> 24) & 0xFF,
                                              socket::InetNtohs(sockaddr.sin_port),
                                              g_settings_holder.GetHardwareType(),
                                              "" /* Nintendo passes empty string as serial number here. */
                                              );

        /* Determine actual usable body length. */
        header.data_len = std::max<u32>(ideal_len, sizeof(packet_body));

        /* Check that the packet will fit. */
        AMS_ABORT_UNLESS(sizeof(header) + header.data_len <= dst_size);

        /* Copy the formatted header. */
        std::memcpy(dst, std::addressof(header), sizeof(header));
        std::memcpy(dst + sizeof(header), packet_body, header.data_len);

        return header.data_len;
    }

    s32 MakeBeaconResponsePacket(char *dst, size_t dst_size) {
        /* Initialize the settings holder. */
        InitializeSettingsHolder();

        /* Create the packet header. */
        TmipcHeader header = {
            .service_id = BeaconResponseServiceId,
            .version    = TmipcVersion,
        };

        /* Create the packet body. */
        std::scoped_lock lk(g_settings_holder_mutex);

        char packet_body[0x100];
        const auto ideal_len = util::SNPrintf(packet_body, sizeof(packet_body), BeaconResponsePacketFormat,
                                              g_settings_holder.GetSpec(),
                                              g_settings_holder.GetHardwareType(),
                                              g_settings_holder.GetTargetName(),
                                              g_settings_holder.GetSerialNumber(),
                                              g_settings_holder.GetFirmwareVersion()
                                              );

        /* Determine actual usable body length. */
        header.data_len = std::max<u32>(ideal_len, sizeof(packet_body));

        /* Check that the packet will fit. */
        AMS_ABORT_UNLESS(sizeof(header) + header.data_len <= dst_size);

        /* Copy the formatted header. */
        std::memcpy(dst, std::addressof(header), sizeof(header));
        std::memcpy(dst + sizeof(header), packet_body, header.data_len);

        return header.data_len;
    }

}
