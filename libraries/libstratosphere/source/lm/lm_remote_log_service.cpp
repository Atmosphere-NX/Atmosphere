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
#include "lm_remote_log_service.hpp"
#include "lm_service_name.hpp"

namespace ams::lm {

    namespace {

        struct LmRemoteLogServiceTag;
        using RemoteAllocator     = ams::sf::ExpHeapStaticAllocator<0x80, LmRemoteLogServiceTag>;
        using RemoteObjectFactory = ams::sf::ObjectFactory<typename RemoteAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    RemoteAllocator::Initialize(lmem::CreateOption_None);
                }
        } g_static_allocator_initializer;

    }

    Result RemoteLogService::OpenLogger(sf::Out<sf::SharedPointer<::ams::lm::ILogger>> out, const sf::ClientProcessId &client_process_id) {
        AMS_UNUSED(client_process_id);

        /* Send libnx command. */
        ::Service logger_srv;
        {
            u64 pid_placeholder = 0;

            #define NX_SERVICE_ASSUME_NON_DOMAIN
            R_TRY(serviceDispatchIn(std::addressof(m_srv), 0, pid_placeholder,
                .in_send_pid = true,
                .out_num_objects = 1,
                .out_objects = std::addressof(logger_srv),
            ));
            #undef NX_SERVICE_ASSUME_NON_DOMAIN
        }

        /* Open logger. */
        out.SetValue(RemoteObjectFactory::CreateSharedEmplaced<::ams::lm::ILogger, RemoteLogger>(logger_srv));
        return ResultSuccess();
    }

    sf::SharedPointer<ILogService> CreateLogService() {
        ::Service srv;
        R_ABORT_UNLESS(sm::GetService(std::addressof(srv), LogServiceName));

        return RemoteObjectFactory::CreateSharedEmplaced<::ams::lm::ILogService, RemoteLogService>(srv);
    }

}
