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

#include "uart_shim.h"

namespace ams::mitm::uart {

    namespace impl {

        #define AMS_UART_IPORTSESSION_MITM_INTERFACE_INFO(C, H)                                                                                                                             \
            AMS_SF_METHOD_INFO(C, H, 0, Result, OpenPort,                 (sf::Out<bool> out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, sf::CopyHandle send_handle, sf::CopyHandle receive_handle, u64 send_buffer_length, u64 receive_buffer_length))                     \
            AMS_SF_METHOD_INFO(C, H, 1, Result, OpenPortForDev,           (sf::Out<bool> out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, sf::CopyHandle send_handle, sf::CopyHandle receive_handle, u64 send_buffer_length, u64 receive_buffer_length))                     \
            AMS_SF_METHOD_INFO(C, H, 2, Result, GetWritableLength,        (sf::Out<u64> out))                     \
            AMS_SF_METHOD_INFO(C, H, 3, Result, Send,      (sf::Out<u64> out_size, const sf::InAutoSelectBuffer &data))                     \
            AMS_SF_METHOD_INFO(C, H, 4, Result, GetReadableLength,        (sf::Out<u64> out))                     \
            AMS_SF_METHOD_INFO(C, H, 5, Result, Receive,  (sf::Out<u64> out_size, const sf::OutAutoSelectBuffer &data))        \
            AMS_SF_METHOD_INFO(C, H, 6, Result, BindPortEvent,            (sf::Out<bool> out, sf::OutCopyHandle out_event_handle, UartPortEventType port_event_type, s64 threshold))                     \
            AMS_SF_METHOD_INFO(C, H, 7, Result, UnbindPortEvent,          (sf::Out<bool> out, UartPortEventType port_event_type))                     \

        AMS_SF_DEFINE_INTERFACE(IPortSession, AMS_UART_IPORTSESSION_MITM_INTERFACE_INFO)

        #define AMS_UART_MITM_INTERFACE_INFO(C, H)                                                                         \
            AMS_SF_METHOD_INFO(C, H, 6, Result, CreatePortSession, (sf::Out<std::shared_ptr<IPortSession>> out))

        AMS_SF_DEFINE_MITM_INTERFACE(IUartMitmInterface, AMS_UART_MITM_INTERFACE_INFO)

    }

    class UartPortService {
        private:
            sm::MitmProcessInfo client_info;
            std::unique_ptr<::UartPortSession> srv;

            static constexpr inline size_t CacheBufferSize = 0x1000;

            char base_path[256];

            size_t cmdlog_pos;
            size_t datalog_pos;

            bool datalog_ready;
            bool data_logging_enabled;

            FsFile datalog_file;

            u8 *send_cache_buffer;
            u8 *receive_cache_buffer;
            size_t send_cache_pos;
            size_t receive_cache_pos;

            bool TryGetCurrentTimestamp(u64 *out);
            void SaveFile(const char *path, const void* buffer, size_t size);
            void WriteCmdLog(const char *str);
            void WriteLog(const void* buffer, size_t size);
            void WriteLogPacket(bool dir, const void* buffer, size_t size);
            void WriteUartData(bool dir, const void* buffer, size_t size);
        public:
            UartPortService(const sm::MitmProcessInfo &cl, std::unique_ptr<::UartPortSession> s);

            virtual ~UartPortService() {
                uartPortSessionClose(this->srv.get());
                fsFileClose(&this->datalog_file);
                std::free(this->send_cache_buffer);
                std::free(this->receive_cache_buffer);
            }
        public:
            /* Actual command API. */
            Result OpenPort(sf::Out<bool> out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, sf::CopyHandle send_handle, sf::CopyHandle receive_handle, u64 send_buffer_length, u64 receive_buffer_length);
            Result OpenPortForDev(sf::Out<bool> out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, sf::CopyHandle send_handle, sf::CopyHandle receive_handle, u64 send_buffer_length, u64 receive_buffer_length);
            Result GetWritableLength(sf::Out<u64> out);
            Result Send(sf::Out<u64> out_size, const sf::InAutoSelectBuffer &data);
            Result GetReadableLength(sf::Out<u64> out);
            Result Receive(sf::Out<u64> out_size, const sf::OutAutoSelectBuffer &data);
            Result BindPortEvent(sf::Out<bool> out, sf::OutCopyHandle out_event_handle, UartPortEventType port_event_type, s64 threshold);
            Result UnbindPortEvent(sf::Out<bool> out, UartPortEventType port_event_type);
    };
    static_assert(impl::IsIPortSession<UartPortService>);

    class UartMitmService : public sf::MitmServiceImplBase {
        public:
            using MitmServiceImplBase::MitmServiceImplBase;
        public:
            static bool ShouldMitm(const sm::MitmProcessInfo &client_info) {
                /* We will mitm:
                 * - bluetooth, for logging HCI.
                 */
                return client_info.program_id == ncm::SystemProgramId::Bluetooth;
            }
        public:
            Result CreatePortSession(sf::Out<std::shared_ptr<impl::IPortSession>> out);
    };
    static_assert(impl::IsIUartMitmInterface<UartMitmService>);

}
