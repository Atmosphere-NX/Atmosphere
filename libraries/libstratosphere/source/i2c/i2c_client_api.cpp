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

namespace ams::i2c {

    namespace {

        constinit os::SdkMutex g_init_mutex;
        constinit int g_initialize_count = 0;

        constinit os::SdkMutex g_i2c_mutex;
        std::shared_ptr<sf::IManager> g_i2c_manager;
        constinit int g_i2c_count = 0;

        constinit os::SdkMutex g_i2c_pcv_mutex;
        std::shared_ptr<sf::IManager> g_i2c_pcv_manager;
        constinit int g_i2c_pcv_count = 0;

        using InternalSession = std::shared_ptr<i2c::sf::ISession>;

        InternalSession &GetInterface(const I2cSession &session) {
            AMS_ASSERT(session._session != nullptr);
            return *static_cast<InternalSession *>(session._session);
        }

        std::shared_ptr<sf::IManager> GetManager(DeviceCode device_code) {
            if (IsPowerBusDeviceCode(device_code)) {
                return g_i2c_pcv_manager;
            } else {
                return g_i2c_manager;
            }
        }

    }

    void InitializeWith(std::shared_ptr<i2c::sf::IManager> &&sp, std::shared_ptr<i2c::sf::IManager> &&sp_pcv) {
        std::scoped_lock lk(g_init_mutex);

        AMS_ABORT_UNLESS(g_initialize_count == 0);

        {
            std::scoped_lock lk(g_i2c_mutex);
            g_i2c_manager = std::move(sp);
            AMS_ABORT_UNLESS(g_i2c_count == 0);
            g_i2c_count = 1;
        }

        {
            std::scoped_lock lk(g_i2c_pcv_mutex);
            g_i2c_pcv_manager = std::move(sp);
            AMS_ABORT_UNLESS(g_i2c_pcv_count == 0);
            g_i2c_pcv_count = 1;
        }

        g_initialize_count = 1;
    }

    void InitializeEmpty() {
        std::scoped_lock lk(g_init_mutex);

        ++g_initialize_count;
    }

    void Finalize() {
        std::scoped_lock lk(g_init_mutex);

        AMS_ASSERT(g_initialize_count > 0);

        if ((--g_initialize_count) == 0) {
            {
                std::scoped_lock lk(g_i2c_mutex);
                AMS_ASSERT(g_i2c_count > 0);
                if (g_i2c_count > 0) {
                    if ((--g_i2c_count) == 0) {
                        g_i2c_manager.reset();
                    }
                }
            }
            {
                std::scoped_lock lk(g_i2c_pcv_mutex);
                AMS_ASSERT(g_i2c_pcv_count > 0);
                if (g_i2c_pcv_count > 0) {
                    if ((--g_i2c_pcv_count) == 0) {
                        g_i2c_pcv_manager.reset();
                    }
                }
            }
        }
    }

    Result OpenSession(I2cSession *out, DeviceCode device_code) {
        /* Allocate the session. */
        InternalSession *internal_session = new (std::nothrow) InternalSession;
        AMS_ABORT_UNLESS(internal_session != nullptr);
        auto session_guard = SCOPE_GUARD { delete internal_session; };

        /* Get manager for the device. */
        auto manager = GetManager(device_code);

        /* Get the session. */
        {
            ams::sf::cmif::ServiceObjectHolder object_holder;
            if (hos::GetVersion() >= hos::Version_6_0_0) {
                R_TRY(manager->OpenSession2(std::addressof(object_holder), device_code));
            } else {
                R_TRY(manager->OpenSession(std::addressof(object_holder), ConvertToI2cDevice(device_code)));
            }
            *internal_session = object_holder.GetServiceObject<sf::ISession>();
        }

        /* Set output. */
        out->_session = internal_session;

        /* We succeeded. */
        session_guard.Cancel();
        return ResultSuccess();
    }

    void CloseSession(I2cSession &session) {
        /* Close the session. */
        delete std::addressof(GetInterface(session));
        session._session = nullptr;
    }

    Result Send(const I2cSession &session, const void *src, size_t src_size, TransactionOption option) {
        const ams::sf::InAutoSelectBuffer buf(src, src_size);

        return GetInterface(session)->Send(buf, option);
    }

    Result Receive(void *dst, size_t dst_size, const I2cSession &session, TransactionOption option) {
        const ams::sf::OutAutoSelectBuffer buf(dst, dst_size);

        return GetInterface(session)->Receive(buf, option);
    }

    Result ExecuteCommandList(void *dst, size_t dst_size, const I2cSession &session, const void *src, size_t src_size) {
        const ams::sf::OutAutoSelectBuffer buf(dst, dst_size);
        const ams::sf::InPointerArray<i2c::I2cCommand> arr(static_cast<const i2c::I2cCommand *>(src), src_size);

        return GetInterface(session)->ExecuteCommandList(buf, arr);
    }

    void SetRetryPolicy(const I2cSession &session, int max_retry_count, int retry_interval_us) {
        AMS_ASSERT(max_retry_count >= 0);
        AMS_ASSERT(retry_interval_us >= 0);

        R_ABORT_UNLESS(GetInterface(session)->SetRetryPolicy(max_retry_count, retry_interval_us));
    }

}
