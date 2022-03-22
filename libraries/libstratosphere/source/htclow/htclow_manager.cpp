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
#include "htclow_manager.hpp"
#include "htclow_manager_impl.hpp"

namespace ams::htclow {

    HtclowManager::HtclowManager(mem::StandardAllocator *allocator) : m_allocator(allocator), m_impl(static_cast<HtclowManagerImpl *>(allocator->Allocate(sizeof(HtclowManagerImpl), alignof(HtclowManagerImpl)))) {
        std::construct_at(m_impl, m_allocator);
    }

    HtclowManager::~HtclowManager() {
        std::destroy_at(m_impl);
        m_allocator->Free(m_impl);
    }

    Result HtclowManager::OpenDriver(impl::DriverType driver_type) {
        return m_impl->OpenDriver(driver_type);
    }

    void HtclowManager::CloseDriver() {
        return m_impl->CloseDriver();
    }

    Result HtclowManager::Open(impl::ChannelInternalType channel) {
        return m_impl->Open(channel);
    }

    Result HtclowManager::Close(impl::ChannelInternalType channel) {
        return m_impl->Close(channel);
    }

    void HtclowManager::Resume() {
        return m_impl->Resume();
    }

    void HtclowManager::Suspend() {
        return m_impl->Suspend();
    }

    Result HtclowManager::ConnectBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        return m_impl->ConnectBegin(out_task_id, channel);
    }

    Result HtclowManager::ConnectEnd(impl::ChannelInternalType channel, u32 task_id) {
        return m_impl->ConnectEnd(channel, task_id);
    }

    void HtclowManager::Disconnect() {
        return m_impl->Disconnect();
    }

    Result HtclowManager::FlushBegin(u32 *out_task_id, impl::ChannelInternalType channel) {
        return m_impl->FlushBegin(out_task_id, channel);
    }

    Result HtclowManager::FlushEnd(u32 task_id) {
        return m_impl->FlushEnd(task_id);
    }

    ChannelState HtclowManager::GetChannelState(impl::ChannelInternalType channel) {
        return m_impl->GetChannelState(channel);
    }

    os::EventType *HtclowManager::GetChannelStateEvent(impl::ChannelInternalType channel) {
        return m_impl->GetChannelStateEvent(channel);
    }

    impl::DriverType HtclowManager::GetDriverType() {
        return m_impl->GetDriverType();
    }

    os::EventType *HtclowManager::GetTaskEvent(u32 task_id) {
        return m_impl->GetTaskEvent(task_id);
    }

    void HtclowManager::NotifyAsleep() {
        return m_impl->NotifyAsleep();
    }

    void HtclowManager::NotifyAwake() {
        return m_impl->NotifyAwake();
    }

    Result HtclowManager::ReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size) {
        return m_impl->ReceiveBegin(out_task_id, channel, size);
    }

    Result HtclowManager::ReceiveEnd(size_t *out, void *dst, size_t dst_size, impl::ChannelInternalType channel, u32 task_id) {
        return m_impl->ReceiveEnd(out, dst, dst_size, channel, task_id);
    }

    Result HtclowManager::SendBegin(u32 *out_task_id, size_t *out, const void *src, size_t src_size, impl::ChannelInternalType channel) {
        return m_impl->SendBegin(out_task_id, out, src, src_size, channel);
    }

    Result HtclowManager::SendEnd(u32 task_id) {
        return m_impl->SendEnd(task_id);
    }

    Result HtclowManager::WaitReceiveBegin(u32 *out_task_id, impl::ChannelInternalType channel, size_t size) {
        return m_impl->WaitReceiveBegin(out_task_id, channel, size);
    }

    Result HtclowManager::WaitReceiveEnd(u32 task_id) {
        return m_impl->WaitReceiveEnd(task_id);
    }

    void HtclowManager::SetConfig(impl::ChannelInternalType channel, const ChannelConfig &config) {
        return m_impl->SetConfig(channel, config);
    }

    void HtclowManager::SetDebugDriver(driver::IDriver *driver) {
        return m_impl->SetDebugDriver(driver);
    }

    void HtclowManager::SetReceiveBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size) {
        return m_impl->SetReceiveBuffer(channel, buf, buf_size);
    }

    void HtclowManager::SetSendBuffer(impl::ChannelInternalType channel, void *buf, size_t buf_size) {
        return m_impl->SetSendBuffer(channel, buf, buf_size);
    }

    void HtclowManager::SetSendBufferWithData(impl::ChannelInternalType channel, const void *buf, size_t buf_size) {
        return m_impl->SetSendBufferWithData(channel, buf, buf_size);
    }

    Result HtclowManager::Shutdown(impl::ChannelInternalType channel) {
        return m_impl->Shutdown(channel);
    }

}
