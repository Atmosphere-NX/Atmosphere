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
#include "lm_log_service_impl.hpp"
#include "lm_logger_impl.hpp"

namespace ams::lm::srv {

    namespace {

        struct LoggerImplAllocatorTag;
        using LoggerAllocator     = ams::sf::ExpHeapStaticAllocator<3_KB, LoggerImplAllocatorTag>;
        using LoggerObjectFactory = ams::sf::ObjectFactory<typename LoggerAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    LoggerAllocator::Initialize(lmem::CreateOption_None);
                }
        } g_static_allocator_initializer;

    }

    Result LogServiceImpl::OpenLogger(sf::Out<sf::SharedPointer<::ams::lm::ILogger>> out, const sf::ClientProcessId &client_process_id) {
        /*  Open logger. */
        out.SetValue(LoggerObjectFactory::CreateSharedEmplaced<::ams::lm::ILogger, LoggerImpl>(this, client_process_id.GetValue()));
        return ResultSuccess();
    }

}
