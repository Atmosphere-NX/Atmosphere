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

namespace ams::sf::cmif {

    Result DomainServiceObjectDispatchTable::ProcessMessage(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const  {
        return this->ProcessMessageImpl(ctx, static_cast<DomainServiceObject *>(ctx.srv_obj)->GetServerDomain(), in_raw_data);
    }

    Result DomainServiceObjectDispatchTable::ProcessMessageForMitm(ServiceDispatchContext &ctx, const cmif::PointerAndSize &in_raw_data) const {
        return this->ProcessMessageForMitmImpl(ctx, static_cast<DomainServiceObject *>(ctx.srv_obj)->GetServerDomain(), in_raw_data);
    }

    Result DomainServiceObjectDispatchTable::ProcessMessageImpl(ServiceDispatchContext &ctx, ServerDomainBase *domain, const cmif::PointerAndSize &in_raw_data) const {
        const CmifDomainInHeader *in_header = reinterpret_cast<const CmifDomainInHeader *>(in_raw_data.GetPointer());
        R_UNLESS(in_raw_data.GetSize() >= sizeof(*in_header), sf::cmif::ResultInvalidHeaderSize());
        const cmif::PointerAndSize in_domain_raw_data = cmif::PointerAndSize(in_raw_data.GetAddress() + sizeof(*in_header), in_raw_data.GetSize() - sizeof(*in_header));

        const DomainObjectId target_object_id = DomainObjectId{in_header->object_id};
        switch (in_header->type) {
            case CmifDomainRequestType_SendMessage:
            {
                auto target_object = domain->GetObject(target_object_id);
                R_UNLESS(static_cast<bool>(target_object), sf::cmif::ResultTargetNotFound());
                R_UNLESS(in_header->data_size + in_header->num_in_objects * sizeof(DomainObjectId) <= in_domain_raw_data.GetSize(), sf::cmif::ResultInvalidHeaderSize());
                const cmif::PointerAndSize in_message_raw_data = cmif::PointerAndSize(in_domain_raw_data.GetAddress(), in_header->data_size);
                DomainObjectId in_object_ids[8];
                R_UNLESS(in_header->num_in_objects <= util::size(in_object_ids), sf::cmif::ResultInvalidNumInObjects());
                std::memcpy(in_object_ids, reinterpret_cast<DomainObjectId *>(in_message_raw_data.GetAddress() + in_message_raw_data.GetSize()), sizeof(DomainObjectId) * in_header->num_in_objects);
                DomainServiceObjectProcessor domain_processor(domain, in_object_ids, in_header->num_in_objects);
                if (ctx.processor == nullptr) {
                    ctx.processor = std::addressof(domain_processor);
                } else {
                    ctx.processor->SetImplementationProcessor(std::addressof(domain_processor));
                }
                ctx.srv_obj = target_object.GetServiceObjectUnsafe();
                return target_object.ProcessMessage(ctx, in_message_raw_data);
            }
            case CmifDomainRequestType_Close:
                /* TODO: N doesn't error check here. Should we? */
                domain->UnregisterObject(target_object_id);
                return ResultSuccess();
            default:
                return sf::cmif::ResultInvalidInHeader();
        }
    }

    Result DomainServiceObjectDispatchTable::ProcessMessageForMitmImpl(ServiceDispatchContext &ctx, ServerDomainBase *domain, const cmif::PointerAndSize &in_raw_data) const {
        const CmifDomainInHeader *in_header = reinterpret_cast<const CmifDomainInHeader *>(in_raw_data.GetPointer());
        R_UNLESS(in_raw_data.GetSize() >= sizeof(*in_header), sf::cmif::ResultInvalidHeaderSize());
        const cmif::PointerAndSize in_domain_raw_data = cmif::PointerAndSize(in_raw_data.GetAddress() + sizeof(*in_header), in_raw_data.GetSize() - sizeof(*in_header));

        const DomainObjectId target_object_id = DomainObjectId{in_header->object_id};
        switch (in_header->type) {
            case CmifDomainRequestType_SendMessage:
            {
                auto target_object = domain->GetObject(target_object_id);

                /* Mitm. If we don't have a target object, we should forward to let the server handle. */
                if (!target_object) {
                    return ctx.session->ForwardRequest(ctx);
                }

                R_UNLESS(in_header->data_size + in_header->num_in_objects * sizeof(DomainObjectId) <= in_domain_raw_data.GetSize(), sf::cmif::ResultInvalidHeaderSize());
                const cmif::PointerAndSize in_message_raw_data = cmif::PointerAndSize(in_domain_raw_data.GetAddress(), in_header->data_size);
                DomainObjectId in_object_ids[8];
                R_UNLESS(in_header->num_in_objects <= util::size(in_object_ids), sf::cmif::ResultInvalidNumInObjects());
                std::memcpy(in_object_ids, reinterpret_cast<DomainObjectId *>(in_message_raw_data.GetAddress() + in_message_raw_data.GetSize()), sizeof(DomainObjectId) * in_header->num_in_objects);
                DomainServiceObjectProcessor domain_processor(domain, in_object_ids, in_header->num_in_objects);
                if (ctx.processor == nullptr) {
                    ctx.processor = std::addressof(domain_processor);
                } else {
                    ctx.processor->SetImplementationProcessor(std::addressof(domain_processor));
                }
                ctx.srv_obj = target_object.GetServiceObjectUnsafe();
                return target_object.ProcessMessage(ctx, in_message_raw_data);
            }
            case CmifDomainRequestType_Close:
            {
                auto target_object = domain->GetObject(target_object_id);

                /* If the object is not in the domain, tell the server to close it. */
                if (!target_object) {
                    return ctx.session->ForwardRequest(ctx);
                }

                /* If the object is in the domain, close our copy of it. Mitm objects are required to close their associated domain id, so this shouldn't cause desynch. */
                domain->UnregisterObject(target_object_id);
                return ResultSuccess();
            }
            default:
                return sf::cmif::ResultInvalidInHeader();
        }
    }

    Result DomainServiceObjectProcessor::PrepareForProcess(const ServiceDispatchContext &ctx, const ServerMessageRuntimeMetadata runtime_metadata) const {
        /* Validate in object count. */
        R_UNLESS(m_impl_metadata.GetInObjectCount() == this->GetInObjectCount(), sf::cmif::ResultInvalidNumInObjects());

        /* Nintendo reserves domain object IDs here. We do this later, to support mitm semantics. */

        /* Pass onwards. */
        return m_impl_processor->PrepareForProcess(ctx, runtime_metadata);
    }

    Result DomainServiceObjectProcessor::GetInObjects(ServiceObjectHolder *in_objects) const {
        for (size_t i = 0; i < this->GetInObjectCount(); i++) {
            in_objects[i] = m_domain->GetObject(m_in_object_ids[i]);
        }
        return ResultSuccess();
    }

    HipcRequest DomainServiceObjectProcessor::PrepareForReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const ServerMessageRuntimeMetadata runtime_metadata) {
        /* Call into impl processor, get request. */
        PointerAndSize raw_data;
        HipcRequest request = m_impl_processor->PrepareForReply(ctx, raw_data, runtime_metadata);

        /* Write out header. */
        constexpr size_t out_header_size = sizeof(CmifDomainOutHeader);
        const size_t impl_out_data_total_size = this->GetImplOutDataTotalSize();
        AMS_ABORT_UNLESS(out_header_size + impl_out_data_total_size + sizeof(DomainObjectId) * this->GetOutObjectCount() <= raw_data.GetSize());
        *reinterpret_cast<CmifDomainOutHeader *>(raw_data.GetPointer()) = CmifDomainOutHeader{ .num_out_objects = static_cast<u32>(this->GetOutObjectCount()), };

        /* Set output raw data. */
        out_raw_data = cmif::PointerAndSize(raw_data.GetAddress() + out_header_size, raw_data.GetSize() - out_header_size);
        m_out_object_ids = reinterpret_cast<DomainObjectId *>(out_raw_data.GetAddress() + impl_out_data_total_size);

        return request;
    }

    void DomainServiceObjectProcessor::PrepareForErrorReply(const cmif::ServiceDispatchContext &ctx, PointerAndSize &out_raw_data, const ServerMessageRuntimeMetadata runtime_metadata) {
        /* Call into impl processor, get request. */
        PointerAndSize raw_data;
        m_impl_processor->PrepareForErrorReply(ctx, raw_data, runtime_metadata);

        /* Write out header. */
        constexpr size_t out_header_size = sizeof(CmifDomainOutHeader);
        const size_t impl_out_headers_size = this->GetImplOutHeadersSize();
        AMS_ABORT_UNLESS(out_header_size + impl_out_headers_size <= raw_data.GetSize());
        *reinterpret_cast<CmifDomainOutHeader *>(raw_data.GetPointer()) = CmifDomainOutHeader{ .num_out_objects = 0, };

        /* Set output raw data. */
        out_raw_data = cmif::PointerAndSize(raw_data.GetAddress() + out_header_size, raw_data.GetSize() - out_header_size);

        /* Nintendo unreserves domain entries here, but we haven't reserved them yet. */
    }

    void DomainServiceObjectProcessor::SetOutObjects(const cmif::ServiceDispatchContext &ctx, const HipcRequest &response, ServiceObjectHolder *out_objects, DomainObjectId *selected_ids) {
        AMS_UNUSED(ctx, response);

        const size_t num_out_objects = this->GetOutObjectCount();

        /* Copy input object IDs from command impl (normally these are Invalid, in mitm they should be set). */
        DomainObjectId object_ids[8];
        bool is_reserved[8];
        for (size_t i = 0; i < num_out_objects; i++) {
            object_ids[i] = selected_ids[i];
            is_reserved[i] = false;
        }

        /* Reserve object IDs as necessary. */
        {
            DomainObjectId reservations[8];
            {
                size_t num_unreserved_ids = 0;
                DomainObjectId specific_ids[8];
                size_t num_specific_ids = 0;
                for (size_t i = 0; i < num_out_objects; i++) {
                    /* In the mitm case, we must not reserve IDs in use by other objects, so mitm objects will set this. */
                    if (object_ids[i] == InvalidDomainObjectId) {
                        num_unreserved_ids++;
                    } else {
                        specific_ids[num_specific_ids++] = object_ids[i];
                    }
                }
                /* TODO: Can we make this error non-fatal? It isn't for N, since they can reserve IDs earlier due to not having to worry about mitm. */
                R_ABORT_UNLESS(m_domain->ReserveIds(reservations, num_unreserved_ids));
                m_domain->ReserveSpecificIds(specific_ids, num_specific_ids);
            }

            size_t reservation_index = 0;
            for (size_t i = 0; i < num_out_objects; i++) {
                if (object_ids[i] == InvalidDomainObjectId) {
                    object_ids[i] = reservations[reservation_index++];
                    is_reserved[i] = true;
                }
            }
        }

        /* Actually set out objects. */
        for (size_t i = 0; i < num_out_objects; i++) {
            if (!out_objects[i]) {
                if (is_reserved[i]) {
                    m_domain->UnreserveIds(object_ids + i, 1);
                }
                object_ids[i] = InvalidDomainObjectId;
                continue;
            }
            m_domain->RegisterObject(object_ids[i], std::move(out_objects[i]));
        }

        /* Set out object IDs in message. */
        for (size_t i = 0; i < num_out_objects; i++) {
            m_out_object_ids[i] = object_ids[i];
        }
    }

}
