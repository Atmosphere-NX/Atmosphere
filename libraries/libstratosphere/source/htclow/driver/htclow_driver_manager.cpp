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
#include "htclow_driver_manager.hpp"

namespace ams::htclow::driver {

    Result DriverManager::OpenDriver(impl::DriverType driver_type) {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Check that we're not already open. */
        R_UNLESS(m_open_driver == nullptr, htclow::ResultDriverOpened());

        /* Open the driver. */
        switch (driver_type) {
            case impl::DriverType::Debug:
                R_TRY(m_debug_driver->Open());
                m_open_driver = m_debug_driver;
                break;
            case impl::DriverType::Socket:
                R_TRY(m_socket_driver.Open());
                m_open_driver = std::addressof(m_socket_driver);
                break;
            case impl::DriverType::Usb:
                R_TRY(m_usb_driver.Open());
                m_open_driver = std::addressof(m_usb_driver);
                break;
            case impl::DriverType::PlainChannel:
                //R_TRY(m_plain_channel_driver.Open());
                //m_open_driver = std::addressof(m_plain_channel_driver);
                //break;
                R_THROW(htclow::ResultUnknownDriverType());
            default:
                R_THROW(htclow::ResultUnknownDriverType());
        }

        /* Set the driver type. */
        m_driver_type = driver_type;

        R_SUCCEED();
    }

    void DriverManager::CloseDriver() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        /* Clear our driver type. */
        m_driver_type = util::nullopt;

        /* Close our driver. */
        if (m_open_driver != nullptr) {
            m_open_driver->Close();
            m_open_driver = nullptr;
        }
    }

    impl::DriverType DriverManager::GetDriverType() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return m_driver_type.value_or(impl::DriverType::Unknown);
    }

    IDriver *DriverManager::GetCurrentDriver() {
        /* Lock ourselves. */
        std::scoped_lock lk(m_mutex);

        return m_open_driver;
    }

    void DriverManager::Cancel() {
        m_open_driver->CancelSendReceive();
    }

    void DriverManager::SetDebugDriver(IDriver *driver) {
        m_debug_driver = driver;
        m_driver_type  = impl::DriverType::Debug;
    }

}
