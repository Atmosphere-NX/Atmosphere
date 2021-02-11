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
#include "htcs_manager_service_object.hpp"
#include "htcs_socket_service_object.hpp"

namespace ams::htcs::server {

    namespace {

        struct ServiceObjectAllocatorTag;
        using ServiceObjectAllocator = ams::sf::ExpHeapStaticAllocator<32_KB, ServiceObjectAllocatorTag>;
        using ServiceObjectFactory   = ams::sf::ObjectFactory<typename ServiceObjectAllocator::Policy>;

        class StaticAllocatorInitializer {
            public:
                StaticAllocatorInitializer() {
                    ServiceObjectAllocator::Initialize(lmem::CreateOption_ThreadSafe);
                }
        } g_static_allocator_initializer;

    }

    Result ManagerServiceObject::GetPeerNameAny(sf::Out<htcs::HtcsPeerName> out) {
        AMS_ABORT("ManagerServiceObject::GetPeerNameAny");
    }

    Result ManagerServiceObject::GetDefaultHostName(sf::Out<htcs::HtcsPeerName> out) {
        AMS_ABORT("ManagerServiceObject::GetDefaultHostName");
    }

    Result ManagerServiceObject::CreateSocketOld(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out) {
        return this->CreateSocket(out_err, out, false);
    }

    Result ManagerServiceObject::CreateSocket(sf::Out<s32> out_err, sf::Out<sf::SharedPointer<tma::ISocket>> out, bool enable_disconnection_emulation) {
        AMS_ABORT("ManagerServiceObject::CreateSocket");
    }

    Result ManagerServiceObject::RegisterProcessId(const sf::ClientProcessId &client_pid) {
        /* NOTE: Nintend does nothing here. */
        return ResultSuccess();
    }

    Result ManagerServiceObject::MonitorManager(const sf::ClientProcessId &client_pid) {
        /* NOTE: Nintend does nothing here. */
        return ResultSuccess();
    }

    Result ManagerServiceObject::StartSelect(sf::Out<u32> out_task_id, sf::OutCopyHandle out_event, const sf::InMapAliasArray<s32> &read_handles, const sf::InMapAliasArray<s32> &write_handles, const sf::InMapAliasArray<s32> &exception_handles, s64 tv_sec, s64 tv_usec) {
        AMS_ABORT("ManagerServiceObject::StartSelect");
    }

    Result ManagerServiceObject::EndSelect(sf::Out<s32> out_err, sf::Out<s32> out_res, const sf::OutMapAliasArray<s32> &read_handles, const sf::OutMapAliasArray<s32> &write_handles, const sf::OutMapAliasArray<s32> &exception_handles, u32 task_id) {
        AMS_ABORT("ManagerServiceObject::EndSelect");
    }

}
