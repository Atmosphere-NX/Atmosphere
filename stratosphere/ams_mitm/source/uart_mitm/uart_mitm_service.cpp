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
#include "uart_mitm_service.hpp"
#include "uart_mitm_logger.hpp"
#include "../amsmitm_fs_utils.hpp"

namespace ams::mitm::uart {

    /* Helper functions. */
    bool UartPortService::TryGetCurrentTimestamp(u64 *out) {
        /* Clear output. */
        *out = 0;

        /* Check if we have time service. */
        {
            bool has_time_service = false;
            if (R_FAILED(sm::HasService(&has_time_service, sm::ServiceName::Encode("time:s"))) || !has_time_service) {
                return false;
            }
        }

        /* Try to get the current time. */
        {
            sm::ScopedServiceHolder<timeInitialize, timeExit> time_holder;
            return time_holder && R_SUCCEEDED(timeGetCurrentTime(TimeType_LocalSystemClock, out));
        }
    }

    UartPortService::UartPortService(const sm::MitmProcessInfo &cl, std::unique_ptr<::UartPortSession> s) : m_client_info(cl), m_srv(std::move(s)) {
        Result rc=0;
        /* Get a timestamp. */
        u64 timestamp0=0, timestamp1;
        this->TryGetCurrentTimestamp(&timestamp0);
        timestamp1 = armGetSystemTick();

        /* Setup the btsnoop base timestamps. */
        this->m_timestamp_base = timestamp0;
        if (this->m_timestamp_base) {
            this->m_timestamp_base = 0x00E03AB44A676000 + (this->m_timestamp_base - 946684800) * 1000000;
        }
        this->m_tick_base = timestamp1;

        /* Setup/create the logging directory. */
        std::snprintf(this->m_base_path, sizeof(this->m_base_path), "uart_logs/%011lu_%011lu_%016lx", timestamp0, timestamp1, static_cast<u64>(this->m_client_info.program_id));
        ams::mitm::fs::CreateAtmosphereSdDirectory("uart_logs");
        ams::mitm::fs::CreateAtmosphereSdDirectory(this->m_base_path);

        /* Create/initialize the text cmd_log. */
        char tmp_path[256];
        std::snprintf(tmp_path, sizeof(tmp_path), "%s/%s", this->m_base_path, "cmd_log");
        ams::mitm::fs::CreateAtmosphereSdFile(tmp_path, 0, 0);
        this->m_cmdlog_pos = 0;

        /* Initialize the Send cache-buffer. */
        this->m_send_cache_buffer = static_cast<u8 *>(std::malloc(this->CacheBufferSize));
        AMS_ABORT_UNLESS(this->m_send_cache_buffer != nullptr);
        std::memset(this->m_send_cache_buffer, 0, this->CacheBufferSize);
        this->m_send_cache_pos = 0;

        /* Initialize the Receive cache-buffer. */
        this->m_receive_cache_buffer = static_cast<u8 *>(std::malloc(this->CacheBufferSize));
        AMS_ABORT_UNLESS(this->m_receive_cache_buffer != nullptr);
        std::memset(this->m_receive_cache_buffer, 0, this->CacheBufferSize);
        this->m_receive_cache_pos = 0;

        /* Initialize the datalog. */
        std::snprintf(tmp_path, sizeof(tmp_path), "%s/%s", this->m_base_path, "btsnoop_hci.log");
        ams::mitm::fs::CreateAtmosphereSdFile(tmp_path, 0, 0);
        rc = ams::mitm::fs::OpenAtmosphereSdFile(&this->m_datalog_file, tmp_path, FsOpenMode_Read | FsOpenMode_Write | FsOpenMode_Append);
        /* Set datalog_ready to whether initialization was successful. */
        this->m_datalog_ready = R_SUCCEEDED(rc);

        if (this->m_datalog_ready) {
            std::shared_ptr<UartLogger> logger = mitm::uart::g_logger;
            logger->InitializeDataLog(&this->m_datalog_file, &this->m_datalog_pos);
        }

        /* This will be enabled by WriteUartData once a certain command is detected. */
        /* If you want to log all HCI traffic during system-boot initialization, you can change this field to true. */
        /* When changed to true, qlaunch will hang at a black-screen during system-boot, due to the bluetooth slowdown. */
        this->m_data_logging_enabled = false;
    }

    /* Append the specified string to the text cmd_log file. */
    void UartPortService::WriteCmdLog(const char *str) {
        char tmp_path[256];
        std::snprintf(tmp_path, sizeof(tmp_path), "%s/%s", this->m_base_path, "cmd_log");
        std::shared_ptr<UartLogger> logger = mitm::uart::g_logger;
        logger->SendTextLogData(tmp_path, &this->m_cmdlog_pos, str);
    }

    /* Log data from Send/Receive. */
    /* dir: false = Send (host->controller), true = Receive (controller->host). */
    void UartPortService::WriteUartData(bool dir, const void* buffer, size_t size) {
        /* Select which cache buffer/pos to use via dir. */
        u8 *cache_buffer = !dir ? this->m_send_cache_buffer : this->m_receive_cache_buffer;
        size_t *cache_pos = !dir ? &this->m_send_cache_pos : &this->m_receive_cache_pos;

        /* Verify that the input size is non-zero, and within cache buffer bounds. */
        if (size && *cache_pos + size <= this->CacheBufferSize) {
            struct {
                u8 opcode[0x2];
                u8 param_len;
            } *hci_cmd = reinterpret_cast<decltype(hci_cmd)>(&cache_buffer[0x1]);
            static_assert(sizeof(*hci_cmd) == 0x3);

            struct {
                u8 handle_flags[0x2];
                u16 data_len;
            } *hci_acl_data = reinterpret_cast<decltype(hci_acl_data)>(&cache_buffer[0x1]);
            static_assert(sizeof(*hci_acl_data) == 0x4);

            struct {
                u8 handle_flags[0x2];
                u8 data_len;
            } *hci_sco_data = reinterpret_cast<decltype(hci_sco_data)>(&cache_buffer[0x1]);
            static_assert(sizeof(*hci_sco_data) == 0x3);

            struct {
                u8 event_code;
                u8 param_len;
            } *hci_event = reinterpret_cast<decltype(hci_event)>(&cache_buffer[0x1]);
            static_assert(sizeof(*hci_event) == 0x2);

            struct {
                u8 handle_flags[0x2];
                u16 data_load_len : 14;
                u8 rfu1 : 2;
            } *hci_iso_data = reinterpret_cast<decltype(hci_iso_data)>(&cache_buffer[0x1]);
            static_assert(sizeof(*hci_iso_data) == 0x4);

            /* Copy the input data into the cache and update the pos. */
            std::memcpy(&cache_buffer[*cache_pos], buffer, size);
            (*cache_pos)+= size;

            /* Process the packets in the cache. */
            do {
                size_t orig_pkt_len = 0x0;
                size_t pkt_len = 0x1;

                /* Determine which HCI packet this is, via the packet indicator. */
                /* These are supported regardless of whether the official bluetooth-sysmodule supports it. */

                if (cache_buffer[0] == 0x1) { /* HCI Command */
                    if (*cache_pos >= 0x1+sizeof(*hci_cmd)) {
                        orig_pkt_len = sizeof(*hci_cmd) + hci_cmd->param_len;
                        /* Check for the first command used in the port which is opened last by bluetooth-sysmodule. */
                        /* This is a vendor command. */
                        /* Once detected, data-logging will be enabled. */
                        if (!this->m_data_logging_enabled && hci_cmd->opcode[1] == 0xFC && hci_cmd->opcode[0] == 0x16) {
                            this->m_data_logging_enabled = true;
                        }
                    }
                }
                else if (cache_buffer[0] == 0x2) { /* HCI ACL Data */
                    if (*cache_pos >= 0x1+sizeof(*hci_acl_data)) {
                        orig_pkt_len = sizeof(*hci_acl_data) + hci_acl_data->data_len;
                    }
                }
                else if (cache_buffer[0] == 0x3) { /* HCI Synchronous Data (SCO) */
                    if (*cache_pos >= 0x1+sizeof(*hci_sco_data)) {
                        orig_pkt_len = sizeof(*hci_sco_data) + hci_sco_data->data_len;
                    }
                }
                else if (cache_buffer[0] == 0x4) { /* HCI Event */
                    if (*cache_pos >= 0x1+sizeof(*hci_event)) {
                        orig_pkt_len = sizeof(*hci_event) + hci_event->param_len;
                    }
                }
                else if (cache_buffer[0] == 0x5) { /* HCI ISO Data */
                    if (*cache_pos >= 0x1+sizeof(*hci_iso_data)) {
                        orig_pkt_len = sizeof(*hci_iso_data) + hci_iso_data->data_load_len;
                    }
                }
                else { /* Unknown HCI packet */
                    char str[256];
                    std::snprintf(str, sizeof(str), "WriteUartData(dir = %s): Unknown HCI packet indicator 0x%x, ignoring the packet and emptying the cache.\n", !dir ? "send" : "receive", cache_buffer[0]);
                    this->WriteCmdLog(str);
                    *cache_pos = 0;
                }

                /* If a full packet is available in the cache, update pkt_len. */
                if (orig_pkt_len) {
                    if (*cache_pos >= 0x1+orig_pkt_len) {
                        pkt_len+= orig_pkt_len;
                    }
                }

                /* If a packet is available, log it and update the cache. */
                if (pkt_len>0x1) {
                    /* Only write to the file if data-logging is enabled and initialized. */
                    if (this->m_data_logging_enabled && this->m_datalog_ready) {
                        std::shared_ptr<UartLogger> logger = mitm::uart::g_logger;
                        if (!logger->SendLogData(&this->m_datalog_file, &this->m_datalog_pos, this->m_timestamp_base, this->m_tick_base, dir, cache_buffer, pkt_len)) {
                            char str[256];
                            std::snprintf(str, sizeof(str), "WriteUartData(): SendLogData dropped packet with size = 0x%lx\n", pkt_len);
                            this->WriteCmdLog(str);
                        }
                    }
                    (*cache_pos)-= pkt_len;
                    if (*cache_pos) {
                        std::memmove(cache_buffer, &cache_buffer[pkt_len], *cache_pos);
                    }
                }
                /* Otherwise, exit the loop. */
                else break;
            } while(*cache_pos);
        }
    }

    /* Forward OpenPort and write to the cmd_log. */
    Result UartPortService::OpenPort(sf::Out<bool> out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, sf::CopyHandle send_handle, sf::CopyHandle receive_handle, u64 send_buffer_length, u64 receive_buffer_length) {
        Result rc = uartPortSessionOpenPortFwd(this->m_srv.get(), reinterpret_cast<bool *>(out.GetPointer()), port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_handle.GetValue(), receive_handle.GetValue(), send_buffer_length, receive_buffer_length);
        svcCloseHandle(send_handle.GetValue());
        svcCloseHandle(receive_handle.GetValue());

        char str[256];
        std::snprintf(str, sizeof(str), "OpenPort(port = 0x%x, baud_rate = %u, flow_control_mode = %u, device_variation = %u, is_invert_tx = %d, is_invert_rx = %d, is_invert_rts = %d, is_invert_cts = %d, send_buffer_length = 0x%lx, receive_buffer_length = 0x%lx): rc = 0x%x, out = %d\n", port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_buffer_length, receive_buffer_length, rc.GetValue(), out.GetValue());
        this->WriteCmdLog(str);
        return rc;
    }

    /* Forward OpenPortForDev and write to the cmd_log. */
    Result UartPortService::OpenPortForDev(sf::Out<bool> out, u32 port, u32 baud_rate, UartFlowControlMode flow_control_mode, u32 device_variation, bool is_invert_tx, bool is_invert_rx, bool is_invert_rts, bool is_invert_cts, sf::CopyHandle send_handle, sf::CopyHandle receive_handle, u64 send_buffer_length, u64 receive_buffer_length) {
        Result rc = uartPortSessionOpenPortForDevFwd(this->m_srv.get(), reinterpret_cast<bool *>(out.GetPointer()), port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_handle.GetValue(), receive_handle.GetValue(), send_buffer_length, receive_buffer_length);
        svcCloseHandle(send_handle.GetValue());
        svcCloseHandle(receive_handle.GetValue());

        char str[256];
        std::snprintf(str, sizeof(str), "OpenPortForDev(port = 0x%x, baud_rate = %u, flow_control_mode = %u, device_variation = %u, is_invert_tx = %d, is_invert_rx = %d, is_invert_rts = %d, is_invert_cts = %d, send_buffer_length = 0x%lx, receive_buffer_length = 0x%lx): rc = 0x%x, out = %d\n", port, baud_rate, flow_control_mode, device_variation, is_invert_tx, is_invert_rx, is_invert_rts, is_invert_cts, send_buffer_length, receive_buffer_length, rc.GetValue(), out.GetValue());
        this->WriteCmdLog(str);
        return rc;
    }

    /* Forward GetWritableLength and write to the cmd_log. */
    Result UartPortService::GetWritableLength(sf::Out<u64> out) {
        Result rc = uartPortSessionGetWritableLength(this->m_srv.get(), reinterpret_cast<u64 *>(out.GetPointer()));

        char str[256];
        std::snprintf(str, sizeof(str), "GetWritableLength(): rc = 0x%x, out = 0x%lx\n", rc.GetValue(), out.GetValue());
        this->WriteCmdLog(str);

        return rc;
    }

    /* Forward Send and log the data if the out_size is non-zero. */
    Result UartPortService::Send(sf::Out<u64> out_size, const sf::InAutoSelectBuffer &data) {
        Result rc = uartPortSessionSend(this->m_srv.get(), data.GetPointer(), data.GetSize(), reinterpret_cast<u64 *>(out_size.GetPointer()));

        if (R_SUCCEEDED(rc) && out_size.GetValue()) {
            this->WriteUartData(false, data.GetPointer(), out_size.GetValue());
        }
        return rc;
    }

    /* Forward GetReadableLength and write to the cmd_log. */
    Result UartPortService::GetReadableLength(sf::Out<u64> out) {
        Result rc = uartPortSessionGetReadableLength(this->m_srv.get(), reinterpret_cast<u64 *>(out.GetPointer()));

        char str[256];
        std::snprintf(str, sizeof(str), "GetReadableLength(): rc = 0x%x, out = 0x%lx\n", rc.GetValue(), out.GetValue());
        this->WriteCmdLog(str);

        return rc;
    }

    /* Forward Receive and log the data if the out_size is non-zero. */
    Result UartPortService::Receive(sf::Out<u64> out_size, const sf::OutAutoSelectBuffer &data) {
        Result rc = uartPortSessionReceive(this->m_srv.get(), data.GetPointer(), data.GetSize(), reinterpret_cast<u64 *>(out_size.GetPointer()));

        if (R_SUCCEEDED(rc) && out_size.GetValue()) {
            this->WriteUartData(true, data.GetPointer(), out_size.GetValue());
        }
        return rc;
    }

    /* Forward BindPortEvent and write to the cmd_log. */
    Result UartPortService::BindPortEvent(sf::Out<bool> out, sf::OutCopyHandle out_event_handle, UartPortEventType port_event_type, s64 threshold) {
        Result rc = uartPortSessionBindPortEventFwd(this->m_srv.get(), port_event_type, threshold, reinterpret_cast<bool *>(out.GetPointer()), out_event_handle.GetHandlePointer());

        char str[256];
        std::snprintf(str, sizeof(str), "BindPortEvent(port_event_type = 0x%x, threshold = 0x%lx): rc = 0x%x, out = %d\n", port_event_type, threshold, rc.GetValue(), out.GetValue());
        this->WriteCmdLog(str);
        return rc;
    }

    /* Forward UnbindPortEvent and write to the cmd_log. */
    Result UartPortService::UnbindPortEvent(sf::Out<bool> out, UartPortEventType port_event_type) {
        Result rc = uartPortSessionUnbindPortEvent(this->m_srv.get(), port_event_type, reinterpret_cast<bool *>(out.GetPointer()));

        char str[256];
        std::snprintf(str, sizeof(str), "UnbindPortEvent(port_event_type = 0x%x): rc = 0x%x, out = %d\n", port_event_type, rc.GetValue(), out.GetValue());
        this->WriteCmdLog(str);

        return rc;
    }

    Result UartMitmService::CreatePortSession(sf::Out<sf::SharedPointer<impl::IPortSession>> out) {
        /* Open a port interface. */
        UartPortSession port;
        R_TRY(uartCreatePortSessionFwd(this->forward_service.get(), &port));
        const sf::cmif::DomainObjectId target_object_id{serviceGetObjectId(&port.s)};

        out.SetValue(sf::CreateSharedObjectEmplaced<impl::IPortSession, UartPortService>(this->client_info, std::make_unique<UartPortSession>(port)), target_object_id);
        return ResultSuccess();
    }

}
