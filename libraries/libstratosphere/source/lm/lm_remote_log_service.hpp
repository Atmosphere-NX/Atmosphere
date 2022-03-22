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
#include "sf/lm_i_log_service.hpp"

namespace ams::lm {

    /* TODO: Real libnx primitives? */

    #define NX_SERVICE_ASSUME_NON_DOMAIN

    class RemoteLogger {
        private:
            ::Service m_srv;
        public:
            RemoteLogger(::Service &s) : m_srv(s) { /* ... */ }
            ~RemoteLogger() { ::serviceClose(std::addressof(m_srv)); }
        public:
            /* Actual commands. */
            Result Log(const sf::InAutoSelectBuffer &message) {
                return serviceDispatch(std::addressof(m_srv), 0,
                    .buffer_attrs = { SfBufferAttr_In | SfBufferAttr_HipcAutoSelect },
                    .buffers = { { message.GetPointer(), message.GetSize() } },
                );
            }

            Result SetDestination(u32 destination) {
                return serviceDispatchIn(std::addressof(m_srv), 1, destination);
            }
    };
    static_assert(lm::IsILogger<RemoteLogger>);

    class RemoteLogService {
        private:
            ::Service m_srv;
        public:
            RemoteLogService(::Service &s) : m_srv(s) { /* ... */ }
            ~RemoteLogService() { ::serviceClose(std::addressof(m_srv)); }
        public:
            /* Actual commands. */
            Result OpenLogger(sf::Out<sf::SharedPointer<::ams::lm::ILogger>> out, const sf::ClientProcessId &client_process_id);
    };
    static_assert(lm::IsILogService<RemoteLogService>);

    #undef NX_SERVICE_ASSUME_NON_DOMAIN

    sf::SharedPointer<ILogService> CreateLogService();

}
