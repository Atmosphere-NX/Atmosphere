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
#include "htclow_manager_impl.hpp"
#include "htclow_default_channel_config.hpp"

namespace ams::htclow {

    HtclowManagerImpl::HtclowManagerImpl(mem::StandardAllocator *allocator)
        : m_packet_factory(allocator), m_driver_manager(allocator), m_mux(std::addressof(m_packet_factory), std::addressof(m_ctrl_state_machine)),
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
        /* Close the driver, if we're open. */
        if (m_is_driver_open) {
            /* Cancel the driver. */
            m_driver_manager.Cancel();

            /* Stop our listener. */
            m_listener.Cancel();
            m_listener.Wait();

            /* Close the driver. */
            m_driver_manager.CloseDriver();

            /* Set the driver type to unknown. */
            m_ctrl_service.SetDriverType(impl::DriverType::Unknown);

            /* Note the driver as closed. */
            m_is_driver_open = false;
        }
    }

    Result HtclowManagerImpl::Open(impl::ChannelInternalType channel) {
        return m_mux.Open(channel);
    }

    Result HtclowManagerImpl::Close(impl::ChannelInternalType channel) {
        return m_mux.Close(channel);
    }

    void HtclowManagerImpl::Resume() {
        /* Get our driver. */
        auto *driver = m_driver_manager.GetCurrentDriver();

        /* Resume our driver. */
        driver->Resume();

        /* Start the listener. */
        m_listener.Start(driver);

        /* Resume our control service. */
        m_ctrl_service.Resume();
    }

    void HtclowManagerImpl::Suspend() {
        /* Suspend our control service. */
        m_ctrl_service.Suspend();

        /* Stop our listener. */
        m_listener.Cancel();
        m_listener.Wait();

        /* Suspend our driver. */
        m_driver_manager.GetCurrentDriver()->Suspend();
    }

    Result HtclowManagerImpl::ConnectBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        /* Begin connecting. */
        R_TRY(m_mux.ConnectBegin(out_task_id, channel));

        /* Try to ready ourselves. */
        m_ctrl_service.TryReady();
        return ResultSuccess();
    }

    Result HtclowManagerImpl::ConnectEnd(impl::ChannelInternalType channel, u32 task_id) {
        return m_mux.ConnectEnd(channel, task_id);
    }

    void HtclowManagerImpl::Disconnect() {
        return m_ctrl_service.Disconnect();
    }

    Result HtclowManagerImpl::FlushBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        return m_mux.FlushBegin(out_task_id, channel);
    }

    Result HtclowManagerImpl::FlushEnd(u32 task_id) {
        return m_mux.FlushEnd(task_id);
    }

    ChannelState HtclowManagerImpl::GetChannelState(impl::ChannelInternalType channel) {
        return m_mux.GetChannelState(channel);
    }

    os::EventType *HtclowManagerImpl::GetChannelStateEvent(impl::ChannelInternalType channel) {
        return m_mux.GetChannelStateEvent(channel);
    }

    impl::DriverType HtclowManagerImpl::GetDriverType() {
        return m_driver_manager.GetDriverType();
    }

    os::EventType *HtclowManagerImpl::GetTaskEvent(u32 task_id) {
        return m_mux.GetTaskEvent(task_id);
    }

    void HtclowManagerImpl::NotifyAsleep() {
        return m_ctrl_service.NotifyAsleep();
    }

    void HtclowManagerImpl::NotifyAwake() {
        return m_ctrl_service.NotifyAwake();
    }

    Result HtclowManagerImpl::ReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size) {
        return m_mux.ReceiveBegin(out_task_id, channel, size);
    }

    Result HtclowManagerImpl::ReceiveEnd(size_t *out, void *dst, size_t dst_size, impl::ChannelInternalType channel, u32 task_id) {
        return m_mux.ReceiveEnd(out, dst, dst_size, channel, task_id);
    }

    Result HtclowManagerImpl::SendBegin(u32 *out_task_id, size_t *out, const void *src, size_t src_size, impl::ChannelInternalType channel) {
        return m_mux.SendBegin(out_task_id, out, src, src_size, channel);
    }

    Result HtclowManagerImpl::SendEnd(u32 task_id) {
        return m_mux.SendEnd(task_id);
    }

    Result HtclowManagerImpl::WaitReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size) {
        return m_mux.WaitReceiveBegin(out_task_id, channel, size);
    }

    Result HtclowManagerImpl::WaitReceiveEnd(u32 task_id) {
        return m_mux.WaitReceiveEnd(task_id);
    }

    void HtclowManagerImpl::SetConfig(impl::ChannelInternalType channel, const ChannelConfig &config) {
        return m_mux.SetConfig(channel, config);
    }

    void HtclowManagerImpl::SetDebugDriver(driver::IDriver *driver) {
        m_driver_manager.SetDebugDriver(driver);
    }

    void HtclowManagerImpl::SetReceiveBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size) {
        return m_mux.SetReceiveBuffer(channel, buf, buf_size);
    }

    void HtclowManagerImpl::SetSendBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size) {
        return m_mux.SetSendBuffer(channel, buf, buf_size, m_driver_manager.GetDriverType() == impl::DriverType::Usb ? sizeof(PacketBody) : DefaultChannelConfig.max_packet_size);
    }

    void HtclowManagerImpl::SetSendBufferWithData(impl::ChannelInternalType channel, const void *buf, size_t buf_size) {
        return m_mux.SetSendBufferWithData(channel, buf, buf_size, m_driver_manager.GetDriverType() == impl::DriverType::Usb ? sizeof(PacketBody) : DefaultChannelConfig.max_packet_size);
    }

    Result HtclowManagerImpl::Shutdown(impl::ChannelInternalType channel) {
        return m_mux.Shutdown(channel);
    }

}
