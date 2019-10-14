/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include "sf_cmif_service_dispatch.hpp"
#include "sf_cmif_domain_api.hpp"
#include "sf_cmif_server_message_processor.hpp"

namespace sts::sf::cmif {

    class DomainServiceObjectDispatchTable : public impl::ServiceDispatchTableBase {
        private:
            Result ProcessMessageImpl(ServiceDispatchContext &ctx, ServerDomainBase *domain, const cmif::PointerAndSize &in_raw_data) const;
        public:
            Result ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const;
    };


    class DomainServiceObjectProcessor : public ServerMessageProcessor {
        private:
            ServerMessageProcessor *impl_processor;
            ServerDomainBase *domain;
        public:
            /* Used to enabled templated message processors. */
            virtual void SetImplementationProcessor(ServerMessageProcessor *impl) override final {
                if (this->impl_processor == nullptr) {
                    this->impl_processor = impl;
                } else {
                    this->impl_processor->SetImplementationProcessor(impl);
                }
            }

            virtual Result PrepareForProcess(const ServiceDispatchContext &ctx, const size_t headers_size) const override final;
            virtual Result GetInObjects(ServiceObjectHolder *in_objects) const override final;
            virtual HipcRequest PrepareForReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const size_t headers_size, size_t &num_out_object_handles) override final;
            virtual void PrepareForErrorReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const size_t headers_size) override final;
            virtual void SetOutObjects(const cmif::ServiceDispatchContext &ctx, const HipcRequest &response, ServiceObjectHolder *out_objects, DomainObjectId *ids) override final;
    };

    class DomainServiceObject : public IServiceObject, public ServerDomainBase {
        public:
            static constexpr inline DomainServiceObjectDispatchTable s_CmifServiceDispatchTable{};
        private:
            virtual ServerDomainBase *GetServerDomain() = 0;
        public:
            /* TODO: Implement to use domain object processor. */
    };

    template<>
    struct ServiceDispatchTraits<DomainServiceObject> {
        static_assert(std::is_base_of<sf::IServiceObject, DomainServiceObject>::value, "DomainServiceObject must derive from sf::IServiceObject");
        static_assert(!std::is_base_of<sf::IMitmServiceObject, DomainServiceObject>::value, "DomainServiceObject must not derive from sf::IMitmServiceObject");
        using ProcessHandlerType = decltype(ServiceDispatchMeta::ProcessHandler);

        using DispatchTableType = DomainServiceObjectDispatchTable;
        static constexpr ProcessHandlerType ProcessHandlerImpl = &impl::ServiceDispatchTableBase::ProcessMessage<DispatchTableType>;

        static constexpr inline ServiceDispatchMeta Meta{&DomainServiceObject::s_CmifServiceDispatchTable, ProcessHandlerImpl};
    };

}
