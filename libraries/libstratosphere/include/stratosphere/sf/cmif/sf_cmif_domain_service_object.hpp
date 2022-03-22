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
#include <stratosphere/sf/cmif/sf_cmif_service_dispatch.hpp>
#include <stratosphere/sf/cmif/sf_cmif_domain_api.hpp>
#include <stratosphere/sf/cmif/sf_cmif_server_message_processor.hpp>

namespace ams::sf::cmif {

    class DomainServiceObjectDispatchTable : public impl::ServiceDispatchTableBase {
        private:
            Result ProcessMessageImpl(ServiceDispatchContext &ctx, ServerDomainBase *domain, const cmif::PointerAndSize &in_raw_data) const;
            Result ProcessMessageForMitmImpl(ServiceDispatchContext &ctx, ServerDomainBase *domain, const cmif::PointerAndSize &in_raw_data) const;
        public:
            Result ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const;
            Result ProcessMessageForMitm(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const;
    };


    class DomainServiceObjectProcessor : public ServerMessageProcessor {
        private:
            ServerMessageProcessor *m_impl_processor;
            ServerDomainBase *m_domain;
            DomainObjectId *m_in_object_ids;
            DomainObjectId *m_out_object_ids;
            size_t m_num_in_objects;
            ServerMessageRuntimeMetadata m_impl_metadata;
        public:
            DomainServiceObjectProcessor(ServerDomainBase *d, DomainObjectId *in_obj_ids, size_t num_in_objs) : m_domain(d), m_in_object_ids(in_obj_ids), m_num_in_objects(num_in_objs) {
                AMS_ABORT_UNLESS(m_domain != nullptr);
                AMS_ABORT_UNLESS(m_in_object_ids != nullptr);
                m_impl_processor = nullptr;
                m_out_object_ids = nullptr;
                m_impl_metadata = {};
            }

            constexpr size_t GetInObjectCount() const {
                return m_num_in_objects;
            }

            constexpr size_t GetOutObjectCount() const {
                return m_impl_metadata.GetOutObjectCount();
            }

            constexpr size_t GetImplOutHeadersSize() const {
                return m_impl_metadata.GetOutHeadersSize();
            }

            constexpr size_t GetImplOutDataTotalSize() const {
                return m_impl_metadata.GetOutDataSize() + m_impl_metadata.GetOutHeadersSize();
            }
        public:
            /* Used to enabled templated message processors. */
            virtual void SetImplementationProcessor(ServerMessageProcessor *impl) override final {
                if (m_impl_processor == nullptr) {
                    m_impl_processor = impl;
                } else {
                    m_impl_processor->SetImplementationProcessor(impl);
                }

                m_impl_metadata = m_impl_processor->GetRuntimeMetadata();
            }

            virtual const ServerMessageRuntimeMetadata GetRuntimeMetadata() const override final {
                const auto runtime_metadata = m_impl_processor->GetRuntimeMetadata();

                return ServerMessageRuntimeMetadata {
                    .in_data_size      = static_cast<u16>(runtime_metadata.GetInDataSize()  + runtime_metadata.GetInObjectCount() * sizeof(DomainObjectId)),
                    .out_data_size     = static_cast<u16>(runtime_metadata.GetOutDataSize() + runtime_metadata.GetOutObjectCount() * sizeof(DomainObjectId)),
                    .in_headers_size   = static_cast<u8>(runtime_metadata.GetInHeadersSize()  + sizeof(CmifDomainInHeader)),
                    .out_headers_size  = static_cast<u8>(runtime_metadata.GetOutHeadersSize() + sizeof(CmifDomainOutHeader)),
                    .in_object_count   = 0,
                    .out_object_count  = 0,
                };
            }

            virtual Result PrepareForProcess(const ServiceDispatchContext &ctx, const ServerMessageRuntimeMetadata runtime_metadata) const override final;
            virtual Result GetInObjects(ServiceObjectHolder *in_objects) const override final;
            virtual HipcRequest PrepareForReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const ServerMessageRuntimeMetadata runtime_metadata) override final;
            virtual void PrepareForErrorReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const ServerMessageRuntimeMetadata runtime_metadata) override final;
            virtual void SetOutObjects(const cmif::ServiceDispatchContext &ctx, const HipcRequest &response, ServiceObjectHolder *out_objects, DomainObjectId *ids) override final;
    };

    class DomainServiceObject : public IServiceObject, public ServerDomainBase {
        friend class DomainServiceObjectDispatchTable;
        public:
            static constexpr inline DomainServiceObjectDispatchTable s_CmifServiceDispatchTable{};
        private:
            virtual ServerDomainBase *GetServerDomain() = 0;
    };

    class MitmDomainServiceObject : public DomainServiceObject{};

    static_assert(sizeof(DomainServiceObject) == sizeof(MitmDomainServiceObject));

    template<>
    struct ServiceDispatchTraits<DomainServiceObject> {
        static_assert(std::is_base_of<sf::IServiceObject, DomainServiceObject>::value, "DomainServiceObject must derive from sf::IServiceObject");
        static_assert(!std::is_base_of<sf::IMitmServiceObject, DomainServiceObject>::value, "DomainServiceObject must not derive from sf::IMitmServiceObject");
        using ProcessHandlerType = decltype(ServiceDispatchMeta::ProcessHandler);

        using DispatchTableType = DomainServiceObjectDispatchTable;
        static constexpr ProcessHandlerType ProcessHandlerImpl = &impl::ServiceDispatchTableBase::ProcessMessage<DispatchTableType>;

        static constexpr inline ServiceDispatchMeta Meta{std::addressof(DomainServiceObject::s_CmifServiceDispatchTable), ProcessHandlerImpl};
    };

    template<>
    struct ServiceDispatchTraits<MitmDomainServiceObject> {
        static_assert(std::is_base_of<DomainServiceObject, MitmDomainServiceObject>::value, "MitmDomainServiceObject must derive from DomainServiceObject");
        using ProcessHandlerType = decltype(ServiceDispatchMeta::ProcessHandler);

        using DispatchTableType = DomainServiceObjectDispatchTable;
        static constexpr ProcessHandlerType ProcessHandlerImpl = &impl::ServiceDispatchTableBase::ProcessMessageForMitm<DispatchTableType>;

        static constexpr inline ServiceDispatchMeta Meta{std::addressof(DomainServiceObject::s_CmifServiceDispatchTable), ProcessHandlerImpl};
    };


}
