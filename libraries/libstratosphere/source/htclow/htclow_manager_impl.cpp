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
#include "htclow_manager_impl.hpp"

namespace ams::htclow {

    HtclowManagerImpl::HtclowManagerImpl(mem::StandardAllocator *allocator)
        : m_packet_factory(allocator), m_driver_manager(), m_mux(std::addressof(m_packet_factory), std::addressof(m_ctrl_state_machine)),
          m_ctrl_packet_factory(allocator), m_ctrl_state_machine(), m_ctrl_service(std::addressof(m_ctrl_packet_factory), std::addressof(m_ctrl_state_machine), std::addressof(m_mux)),
          m_worker(allocator, std::addressof(m_mux), std::addressof(m_ctrl_service)),
          m_listener(allocator, std::addressof(m_mux), std::addressof(m_ctrl_service), std::addressof(m_worker)),
          m_is_driver_open(false)
    {
        /* ... */
    }

    HtclowManagerImpl::~HtclowManagerImpl() {
        /* ... */
    }

    Result HtclowManagerImpl::OpenDriver(impl::DriverType driver_type) {
        /* Set the driver type. */
        m_ctrl_service.SetDriverType(driver_type);

        /* Ensure that we don't end up in an invalid state. */
        auto drv_guard = SCOPE_GUARD { m_ctrl_service.SetDriverType(impl::DriverType::Unknown); };

        /* Try to open the driver. */
        R_TRY(m_driver_manager.OpenDriver(driver_type));

        /* Start the listener. */
        m_listener.Start(m_driver_manager.GetCurrentDriver());

        /* Note the driver as open. */
        m_is_driver_open = true;

        drv_guard.Cancel();
        return ResultSuccess();
    }

    void HtclowManagerImpl::CloseDriver() {
        AMS_ABORT("HtclowManagerImpl::CloseDriver");
    }

    Result HtclowManagerImpl::Open(impl::ChannelInternalType channel) {
        return m_mux.Open(channel);
    }

    Result HtclowManagerImpl::Close(impl::ChannelInternalType channel) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::Close");
    }

    void HtclowManagerImpl::Resume() {
        AMS_ABORT("HtclowManagerImpl::Resume");
    }

    void HtclowManagerImpl::Suspend() {
        AMS_ABORT("HtclowManagerImpl::Suspend");
    }

    Result HtclowManagerImpl::ConnectBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::ConnectBegin");
    }

    Result HtclowManagerImpl::ConnectEnd(impl::ChannelInternalType channel, u32 task_id) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::ConnectEnd");
    }

    void HtclowManagerImpl::Disconnect() {
        AMS_ABORT("HtclowManagerImpl::Disconnect");
    }

    Result HtclowManagerImpl::FlushBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        AMS_ABORT("HtclowManagerImpl::FlushBegin");
    }

    Result HtclowManagerImpl::FlushEnd(u32 task_id) {
        AMS_ABORT("HtclowManagerImpl::FlushEnd");
    }

    ChannelState HtclowManagerImpl::GetChannelState(impl::ChannelInternalType channel) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::GetChannelState");
    }

    os::EventType *HtclowManagerImpl::GetChannelStateEvent(impl::ChannelInternalType channel) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::GetChannelStateEvent");
    }

    impl::DriverType HtclowManagerImpl::GetDriverType() {
        AMS_ABORT("HtclowManagerImpl::GetDriverType");
    }

    os::EventType *HtclowManagerImpl::GetTaskEvent(u32 task_id) {
        return m_mux.GetTaskEvent(task_id);
    }

    void HtclowManagerImpl::NotifyAsleep() {
        AMS_ABORT("HtclowManagerImpl::NotifyAsleep");
    }

    void HtclowManagerImpl::NotifyAwake() {
        AMS_ABORT("HtclowManagerImpl::NotifyAwake");
    }

    Result HtclowManagerImpl::ReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, bool blocking) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::ReceiveBegin");
    }

    Result HtclowManagerImpl::ReceiveEnd(size_t *out, void *dst, size_t dst_size, impl::ChannelInternalType channel, u32 task_id) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::ReceiveEnd");
    }

    Result HtclowManagerImpl::SendBegin(u32 *out_task_id, size_t *out, const void *src, size_t src_size, impl::ChannelInternalType channel) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::SendBegin");
    }

    Result HtclowManagerImpl::SendEnd(u32 task_id) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::SendEnd");
    }

    void HtclowManagerImpl::SetConfig(impl::ChannelInternalType channel, const ChannelConfig &config) {
        AMS_ABORT("HtclowManagerImpl::SetConfig");
    }

    void HtclowManagerImpl::SetDebugDriver(driver::IDriver *driver) {
        m_driver_manager.SetDebugDriver(driver);
    }

    void HtclowManagerImpl::SetReceiveBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::SetReceiveBuffer");
    }

    void HtclowManagerImpl::SetSendBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::SetSendBuffer");
    }

    void HtclowManagerImpl::SetSendBufferWithData(impl::ChannelInternalType channel, const void *buf, size_t buf_size) {
        AMS_ABORT("HtclowManagerImpl::SetSendBufferWithData");
    }

    Result HtclowManagerImpl::Shutdown(impl::ChannelInternalType channel) {
        /* TODO: Used by HtclowDriver */
        AMS_ABORT("HtclowManagerImpl::Shutdown");
    }

}
