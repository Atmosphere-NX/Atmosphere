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

namespace ams::pwm {

    namespace {

        constinit os::SdkMutex g_init_mutex;
        constinit int g_initialize_count = 0;

        ams::sf::SharedPointer<pwm::sf::IManager> g_pwm_manager;

        pwm::sf::IChannelSession *GetInterface(const ChannelSession &session) {
            AMS_ASSERT(session._session != nullptr);
            return static_cast<pwm::sf::IChannelSession *>(session._session);
        }
    }

    void InitializeWith(ams::sf::SharedPointer<pwm::sf::IManager> sp) {
        std::scoped_lock lk(g_init_mutex);

        AMS_ABORT_UNLESS(g_initialize_count == 0);

        g_pwm_manager = sp;

        g_initialize_count = 1;
    }

    void Finalize() {
        std::scoped_lock lk(g_init_mutex);

        AMS_ASSERT(g_initialize_count > 0);

        if ((--g_initialize_count) == 0) {
            g_pwm_manager.Reset();
        }
    }

    Result OpenSession(ChannelSession *out, DeviceCode device_code) {
        /* Get the session. */
        ams::sf::SharedPointer<pwm::sf::IChannelSession> session;
        {
            if (hos::GetVersion() >= hos::Version_6_0_0) {
                R_TRY(g_pwm_manager->OpenSession2(std::addressof(session), device_code));
            } else {
                R_TRY(g_pwm_manager->OpenSession(std::addressof(session), ConvertToChannelName(device_code)));
            }
        }

        /* Set output. */
        out->_session = session.Detach();

        /* We succeeded. */
        return ResultSuccess();
    }

    void CloseSession(ChannelSession &session) {
        /* Close the session. */
        ams::sf::ReleaseSharedObject(GetInterface(session));
        session._session = nullptr;
    }

    void SetPeriod(ChannelSession &session, TimeSpan period) {
        R_ABORT_UNLESS(GetInterface(session)->SetPeriod(period));
    }

    TimeSpan GetPeriod(ChannelSession &session) {
        TimeSpanType out_val;
        R_ABORT_UNLESS(GetInterface(session)->GetPeriod(std::addressof(out_val)));
        return out_val;
    }

    void SetDuty(ChannelSession &session, int duty) {
        R_ABORT_UNLESS(GetInterface(session)->SetDuty(duty));
    }

    int GetDuty(ChannelSession &session) {
        int out_val;
        R_ABORT_UNLESS(GetInterface(session)->GetDuty(std::addressof(out_val)));
        return out_val;
    }

    void SetEnabled(ChannelSession &session, bool en) {
        R_ABORT_UNLESS(GetInterface(session)->SetEnabled(en));
    }

    bool GetEnabled(ChannelSession &session) {
        bool out_val;
        R_ABORT_UNLESS(GetInterface(session)->GetEnabled(std::addressof(out_val)));
        return out_val;
    }

    void SetScale(ChannelSession &session, double scale) {
        R_ABORT_UNLESS(GetInterface(session)->SetScale(scale));
    }

    double GetScale(ChannelSession &session) {
        double out_val;
        R_ABORT_UNLESS(GetInterface(session)->GetScale(std::addressof(out_val)));
        return out_val;
    }

}
