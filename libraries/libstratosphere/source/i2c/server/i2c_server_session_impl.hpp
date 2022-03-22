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

namespace ams::i2c::server {

    class ManagerImpl;

    class SessionImpl {
        private:
            ManagerImpl *m_parent; /* NOTE: this is an sf::SharedPointer<> in Nintendo's code. */
            i2c::driver::I2cSession m_internal_session;
            bool m_has_session;
        public:
            explicit SessionImpl(ManagerImpl *p) : m_parent(p), m_has_session(false) { /* ... */ }

            ~SessionImpl() {
                if (m_has_session) {
                    i2c::driver::CloseSession(m_internal_session);
                }
            }

            Result OpenSession(DeviceCode device_code) {
                AMS_ABORT_UNLESS(!m_has_session);

                R_TRY(i2c::driver::OpenSession(std::addressof(m_internal_session), device_code));
                m_has_session = true;
                return ResultSuccess();
            }
        public:
            /* Actual commands. */
            Result SendOld(const ams::sf::InBuffer &in_data, i2c::TransactionOption option) {
                return i2c::driver::Send(m_internal_session, in_data.GetPointer(), in_data.GetSize(), option);
            }

            Result ReceiveOld(const ams::sf::OutBuffer &out_data, i2c::TransactionOption option) {
                return i2c::driver::Receive(out_data.GetPointer(), out_data.GetSize(), m_internal_session, option);
            }

            Result ExecuteCommandListOld(const ams::sf::OutBuffer &rcv_buf, const ams::sf::InPointerArray<i2c::I2cCommand> &command_list){
                return i2c::driver::ExecuteCommandList(rcv_buf.GetPointer(), rcv_buf.GetSize(), m_internal_session, command_list.GetPointer(), command_list.GetSize() * sizeof(i2c::I2cCommand));
            }

            Result Send(const ams::sf::InAutoSelectBuffer &in_data, i2c::TransactionOption option) {
                return i2c::driver::Send(m_internal_session, in_data.GetPointer(), in_data.GetSize(), option);
            }

            Result Receive(const ams::sf::OutAutoSelectBuffer &out_data, i2c::TransactionOption option) {
                return i2c::driver::Receive(out_data.GetPointer(), out_data.GetSize(), m_internal_session, option);
            }

            Result ExecuteCommandList(const ams::sf::OutAutoSelectBuffer &rcv_buf, const ams::sf::InPointerArray<i2c::I2cCommand> &command_list) {
                return i2c::driver::ExecuteCommandList(rcv_buf.GetPointer(), rcv_buf.GetSize(), m_internal_session, command_list.GetPointer(), command_list.GetSize() * sizeof(i2c::I2cCommand));
            }

            Result SetRetryPolicy(s32 max_retry_count, s32 retry_interval_us) {
                return i2c::driver::SetRetryPolicy(m_internal_session, max_retry_count, retry_interval_us);
            }
    };
    static_assert(i2c::sf::IsISession<SessionImpl>);

}
