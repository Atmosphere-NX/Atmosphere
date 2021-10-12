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

#define AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO(C, H)                                                                                                   \
    AMS_SF_METHOD_INFO(C, H, 0, Result, ConvertCurrentObjectToDomain, (ams::sf::Out<ams::sf::cmif::DomainObjectId> out),                     (out))            \
    AMS_SF_METHOD_INFO(C, H, 1, Result, CopyFromCurrentDomain,        (ams::sf::OutMoveHandle out, ams::sf::cmif::DomainObjectId object_id), (out, object_id)) \
    AMS_SF_METHOD_INFO(C, H, 2, Result, CloneCurrentObject,           (ams::sf::OutMoveHandle out),                                          (out))            \
    AMS_SF_METHOD_INFO(C, H, 3, void,   QueryPointerBufferSize,       (ams::sf::Out<u16> out),                                               (out))            \
    AMS_SF_METHOD_INFO(C, H, 4, Result, CloneCurrentObjectEx,         (ams::sf::OutMoveHandle out, u32 tag),                                 (out, tag))

AMS_SF_DEFINE_INTERFACE(ams::sf::hipc::impl, IHipcManager, AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO)

namespace ams::sf::hipc {

    namespace impl {

        class HipcManagerImpl {
            private:
                ServerDomainSessionManager *m_manager;
                ServerSession *m_session;
                const bool m_is_mitm_session;
            private:
                Result CloneCurrentObjectImpl(sf::OutMoveHandle &out_client_handle, ServerSessionManager *tagged_manager) {
                    /* Clone the object. */
                    cmif::ServiceObjectHolder &&clone = m_session->m_srv_obj_holder.Clone();
                    R_UNLESS(clone, sf::hipc::ResultDomainObjectNotFound());

                    /* Create new session handles. */
                    os::NativeHandle server_handle, client_handle;
                    R_ABORT_UNLESS(hipc::CreateSession(std::addressof(server_handle), std::addressof(client_handle)));

                    /* Register with manager. */
                    if (!m_is_mitm_session) {
                        R_ABORT_UNLESS(tagged_manager->RegisterSession(server_handle, std::move(clone)));
                    } else {
                        /* Check that we can create a mitm session. */
                        AMS_ABORT_UNLESS(ServerManagerBase::CanAnyManageMitmServers());

                        /* Clone the forward service. */
                        std::shared_ptr<::Service> new_forward_service = std::move(ServerSession::CreateForwardService());
                        R_ABORT_UNLESS(serviceClone(util::GetReference(m_session->m_forward_service).get(), new_forward_service.get()));
                        R_ABORT_UNLESS(tagged_manager->RegisterMitmSession(server_handle, std::move(clone), std::move(new_forward_service)));
                    }

                    /* Set output client handle. */
                    out_client_handle.SetValue(client_handle, false);
                    return ResultSuccess();
                }
            public:
                explicit HipcManagerImpl(ServerDomainSessionManager *m, ServerSession *s) : m_manager(m), m_session(s), m_is_mitm_session(s->IsMitmSession()) {
                    /* ... */
                }

                Result ConvertCurrentObjectToDomain(sf::Out<cmif::DomainObjectId> out) {
                    /* Allocate a domain. */
                    auto domain = m_manager->AllocateDomainServiceObject();
                    R_UNLESS(domain, sf::hipc::ResultOutOfDomains());

                    /* Set up the new domain object. */
                    cmif::DomainObjectId object_id = cmif::InvalidDomainObjectId;
                    if (m_is_mitm_session) {
                        /* Check that we can create a mitm session. */
                        AMS_ABORT_UNLESS(ServerManagerBase::CanAnyManageMitmServers());

                        /* Make a new shared pointer to manage the allocated domain. */
                        SharedPointer<cmif::MitmDomainServiceObject> cmif_domain(static_cast<cmif::MitmDomainServiceObject *>(domain), false);

                        /* Convert the remote session to domain. */
                        AMS_ABORT_UNLESS(util::GetReference(m_session->m_forward_service)->own_handle);
                        R_TRY(serviceConvertToDomain(util::GetReference(m_session->m_forward_service).get()));

                        /* The object ID reservation cannot fail here, as that would cause desynchronization from target domain. */
                        object_id = cmif::DomainObjectId{util::GetReference(m_session->m_forward_service)->object_id};
                        domain->ReserveSpecificIds(std::addressof(object_id), 1);

                        /* Register the object. */
                        domain->RegisterObject(object_id, std::move(m_session->m_srv_obj_holder));

                        /* Set the new object holder. */
                        m_session->m_srv_obj_holder = cmif::ServiceObjectHolder(std::move(cmif_domain));
                    } else {
                        /* Make a new shared pointer to manage the allocated domain. */
                        SharedPointer<cmif::DomainServiceObject> cmif_domain(domain, false);

                        /* Reserve a new object in the domain. */
                        R_TRY(domain->ReserveIds(std::addressof(object_id), 1));

                        /* Register the object. */
                        domain->RegisterObject(object_id, std::move(m_session->m_srv_obj_holder));

                        /* Set the new object holder. */
                        m_session->m_srv_obj_holder = cmif::ServiceObjectHolder(std::move(cmif_domain));
                    }

                    /* Return the allocated id. */
                    AMS_ABORT_UNLESS(object_id != cmif::InvalidDomainObjectId);
                    *out = object_id;
                    return ResultSuccess();
                }

                Result CopyFromCurrentDomain(sf::OutMoveHandle out, cmif::DomainObjectId object_id) {
                    /* Get domain. */
                    auto domain = m_session->m_srv_obj_holder.GetServiceObject<cmif::DomainServiceObject>();
                    R_UNLESS(domain != nullptr, sf::hipc::ResultTargetNotDomain());

                    /* Get domain object. */
                    auto &&object = domain->GetObject(object_id);
                    if (!object) {
                        R_UNLESS(m_is_mitm_session, sf::hipc::ResultDomainObjectNotFound());

                        /* Check that we can create a mitm session. */
                        AMS_ABORT_UNLESS(ServerManagerBase::CanAnyManageMitmServers());

                        os::NativeHandle handle;
                        R_TRY(cmifCopyFromCurrentDomain(util::GetReference(m_session->m_forward_service)->session, object_id.value, std::addressof(handle)));

                        out.SetValue(handle, false);
                        return ResultSuccess();
                    }

                    if (!m_is_mitm_session || (ServerManagerBase::CanAnyManageMitmServers() && object_id.value != serviceGetObjectId(util::GetReference(m_session->m_forward_service).get()))) {
                        /* Create new session handles. */
                        os::NativeHandle server_handle, client_handle;
                        R_ABORT_UNLESS(hipc::CreateSession(std::addressof(server_handle), std::addressof(client_handle)));

                        /* Register. */
                        R_ABORT_UNLESS(m_manager->RegisterSession(server_handle, std::move(object)));

                        /* Set output client handle. */
                        out.SetValue(client_handle, false);
                    } else {
                        /* Check that we can create a mitm session. */
                        AMS_ABORT_UNLESS(ServerManagerBase::CanAnyManageMitmServers());

                        /* Copy from the target domain. */
                        os::NativeHandle new_forward_target;
                        R_TRY(cmifCopyFromCurrentDomain(util::GetReference(m_session->m_forward_service)->session, object_id.value, std::addressof(new_forward_target)));

                        /* Create new session handles. */
                        os::NativeHandle server_handle, client_handle;
                        R_ABORT_UNLESS(hipc::CreateSession(std::addressof(server_handle), std::addressof(client_handle)));

                        /* Register. */
                        std::shared_ptr<::Service> new_forward_service = std::move(ServerSession::CreateForwardService());
                        serviceCreate(new_forward_service.get(), new_forward_target);
                        R_ABORT_UNLESS(m_manager->RegisterMitmSession(server_handle, std::move(object), std::move(new_forward_service)));

                        /* Set output client handle. */
                        out.SetValue(client_handle, false);
                    }

                    return ResultSuccess();
                }

                Result CloneCurrentObject(sf::OutMoveHandle out) {
                    return this->CloneCurrentObjectImpl(out, m_manager);
                }

                void QueryPointerBufferSize(sf::Out<u16> out) {
                    out.SetValue(m_session->m_pointer_buffer.GetSize());
                }

                Result CloneCurrentObjectEx(sf::OutMoveHandle out, u32 tag) {
                    return this->CloneCurrentObjectImpl(out, m_manager->GetSessionManagerByTag(tag));
                }
        };
        static_assert(IsIHipcManager<HipcManagerImpl>);

    }

    Result ServerDomainSessionManager::DispatchManagerRequest(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) {
        /* Make a stack object, and pass a shared pointer to it to DispatchRequest. */
        /* Note: This is safe, as no additional references to the hipc manager can ever be stored. */
        /* The shared pointer to stack object is definitely gross, though. */
        UnmanagedServiceObject<impl::IHipcManager, impl::HipcManagerImpl> hipc_manager(this, session);
        return this->DispatchRequest(cmif::ServiceObjectHolder(hipc_manager.GetShared()), session, in_message, out_message);
    }

}
