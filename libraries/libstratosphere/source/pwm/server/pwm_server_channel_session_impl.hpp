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

namespace ams::pwm::server {

    class ManagerImpl;

    class ChannelSessionImpl {
        private:
            ManagerImpl *m_parent; /* NOTE: this is an sf::SharedPointer<> in Nintendo's code. */
            pwm::driver::ChannelSession m_internal_session;
            bool m_has_session;
        public:
            explicit ChannelSessionImpl(ManagerImpl *p) : m_parent(p), m_has_session(false) { /* ... */ }

            ~ChannelSessionImpl() {
                if (m_has_session) {
                    pwm::driver::CloseSession(m_internal_session);
                }
            }

            Result OpenSession(DeviceCode device_code) {
                AMS_ABORT_UNLESS(!m_has_session);

                R_TRY(pwm::driver::OpenSession(std::addressof(m_internal_session), device_code));
                m_has_session = true;
                return ResultSuccess();
            }
        public:
            /* Actual commands. */
            Result SetPeriod(TimeSpanType period) {
                pwm::driver::SetPeriod(m_internal_session, period);
                return ResultSuccess();
            }

            Result GetPeriod(ams::sf::Out<TimeSpanType> out) {
                out.SetValue(pwm::driver::GetPeriod(m_internal_session));
                return ResultSuccess();
            }

            Result SetDuty(int duty) {
                pwm::driver::SetDuty(m_internal_session, duty);
                return ResultSuccess();
            }

            Result GetDuty(ams::sf::Out<int> out) {
                out.SetValue(pwm::driver::GetDuty(m_internal_session));
                return ResultSuccess();
            }

            Result SetEnabled(bool enabled) {
                pwm::driver::SetEnabled(m_internal_session, enabled);
                return ResultSuccess();
            }

            Result GetEnabled(ams::sf::Out<bool> out) {
                out.SetValue(pwm::driver::GetEnabled(m_internal_session));
                return ResultSuccess();
            }

            Result SetScale(double scale) {
                pwm::driver::SetScale(m_internal_session, scale);
                return ResultSuccess();
            }

            Result GetScale(ams::sf::Out<double> out) {
                out.SetValue(pwm::driver::GetScale(m_internal_session));
                return ResultSuccess();
            }
    };
    static_assert(pwm::sf::IsIChannelSession<ChannelSessionImpl>);

}
