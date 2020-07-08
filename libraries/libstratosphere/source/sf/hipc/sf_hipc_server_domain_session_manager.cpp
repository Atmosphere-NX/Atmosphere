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

namespace ams::sf::hipc {

    namespace impl {

        #define AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO(C, H)                                                                                 \
            AMS_SF_METHOD_INFO(C, H, 0, Result, ConvertCurrentObjectToDomain, (ams::sf::Out<ams::sf::cmif::DomainObjectId> out))                     \
            AMS_SF_METHOD_INFO(C, H, 1, Result, CopyFromCurrentDomain,        (ams::sf::OutMoveHandle out, ams::sf::cmif::DomainObjectId object_id)) \
            AMS_SF_METHOD_INFO(C, H, 2, Result, CloneCurrentObject,           (ams::sf::OutMoveHandle out))                                          \
            AMS_SF_METHOD_INFO(C, H, 3, void,   QueryPointerBufferSize,       (ams::sf::Out<u16> out))                                               \
            AMS_SF_METHOD_INFO(C, H, 4, Result, CloneCurrentObjectEx,         (ams::sf::OutMoveHandle out, u32 tag))

        AMS_SF_DEFINE_INTERFACE(IHipcManager, AMS_SF_HIPC_IMPL_I_HIPC_MANAGER_INTERFACE_INFO)

        class HipcManager final {
            private:
                ServerDomainSessionManager *manager;
                ServerSession *session;
                bool is_mitm_session;
            private:
                Result CloneCurrentObjectImpl(Handle *out_client_handle, ServerSessionManager *tagged_manager) {
                    /* Clone the object. */
                    cmif::ServiceObjectHolder &&clone = this->session->srv_obj_holder.Clone();
                    R_UNLESS(clone, sf::hipc::ResultDomainObjectNotFound());

                    /* Create new session handles. */
                    Handle server_handle;
                    R_ABORT_UNLESS(hipc::CreateSession(&server_handle, out_client_handle));

                    /* Register with manager. */
                    if (!is_mitm_session) {
                        R_ABORT_UNLESS(tagged_manager->RegisterSession(server_handle, std::move(clone)));
                    } else {
                        /* Clone the forward service. */
                        std::shared_ptr<::Service> new_forward_service = std::move(ServerSession::CreateForwardService());
                        R_ABORT_UNLESS(serviceClone(this->session->forward_service.get(), new_forward_service.get()));
                        R_ABORT_UNLESS(tagged_manager->RegisterMitmSession(server_handle, std::move(clone), std::move(new_forward_service)));
                    }

                    return ResultSuccess();
                }
            public:
                explicit HipcManager(ServerDomainSessionManager *m, ServerSession *s) : manager(m), session(s), is_mitm_session(s->forward_service != nullptr) {
                    /* ... */
                }

                Result ConvertCurrentObjectToDomain(sf::Out<cmif::DomainObjectId> out) {
                    /* Allocate a domain. */
                    auto domain = this->manager->AllocateDomainServiceObject();
                    R_UNLESS(domain, sf::hipc::ResultOutOfDomains());
                    auto domain_guard = SCOPE_GUARD { cmif::ServerDomainManager::DestroyDomainServiceObject(static_cast<cmif::DomainServiceObject *>(domain)); };

                    cmif::DomainObjectId object_id = cmif::InvalidDomainObjectId;

                    cmif::ServiceObjectHolder new_holder;

                    if (this->is_mitm_session) {
                        /* If we're a mitm session, we need to convert the remote session to domain. */
                        AMS_ABORT_UNLESS(session->forward_service->own_handle);
                        R_TRY(serviceConvertToDomain(session->forward_service.get()));

                        /* The object ID reservation cannot fail here, as that would cause desynchronization from target domain. */
                        object_id = cmif::DomainObjectId{session->forward_service->object_id};
                        domain->ReserveSpecificIds(&object_id, 1);

                        /* Create new object. */
                        cmif::MitmDomainServiceObject *domain_ptr = static_cast<cmif::MitmDomainServiceObject *>(domain);
                        new_holder = cmif::ServiceObjectHolder(std::move(std::shared_ptr<cmif::MitmDomainServiceObject>(domain_ptr, cmif::ServerDomainManager::DestroyDomainServiceObject)));
                    } else {
                        /* We're not a mitm session. Reserve a new object in the domain. */
                        R_TRY(domain->ReserveIds(&object_id, 1));

                        /* Create new object. */
                        cmif::DomainServiceObject *domain_ptr = static_cast<cmif::DomainServiceObject *>(domain);
                        new_holder = cmif::ServiceObjectHolder(std::move(std::shared_ptr<cmif::DomainServiceObject>(domain_ptr, cmif::ServerDomainManager::DestroyDomainServiceObject)));
                    }

                    AMS_ABORT_UNLESS(object_id != cmif::InvalidDomainObjectId);
                    AMS_ABORT_UNLESS(static_cast<bool>(new_holder));

                    /* We succeeded! */
                    domain_guard.Cancel();
                    domain->RegisterObject(object_id, std::move(session->srv_obj_holder));
                    session->srv_obj_holder = std::move(new_holder);
                    out.SetValue(object_id);

                    return ResultSuccess();
                }

                Result CopyFromCurrentDomain(sf::OutMoveHandle out, cmif::DomainObjectId object_id) {
                    /* Get domain. */
                    auto domain = this->session->srv_obj_holder.GetServiceObject<cmif::DomainServiceObject>();
                    R_UNLESS(domain != nullptr, sf::hipc::ResultTargetNotDomain());

                    /* Get domain object. */
                    auto &&object = domain->GetObject(object_id);
                    if (!object) {
                        R_UNLESS(this->is_mitm_session, sf::hipc::ResultDomainObjectNotFound());
                        return cmifCopyFromCurrentDomain(this->session->forward_service->session, object_id.value, out.GetHandlePointer());
                    }

                    if (!this->is_mitm_session || object_id.value != serviceGetObjectId(this->session->forward_service.get())) {
                        /* Create new session handles. */
                        Handle server_handle;
                        R_ABORT_UNLESS(hipc::CreateSession(&server_handle, out.GetHandlePointer()));

                        /* Register. */
                        R_ABORT_UNLESS(this->manager->RegisterSession(server_handle, std::move(object)));
                    } else {
                        /* Copy from the target domain. */
                        Handle new_forward_target;
                        R_TRY(cmifCopyFromCurrentDomain(this->session->forward_service->session, object_id.value, &new_forward_target));

                        /* Create new session handles. */
                        Handle server_handle;
                        R_ABORT_UNLESS(hipc::CreateSession(&server_handle, out.GetHandlePointer()));

                        /* Register. */
                        std::shared_ptr<::Service> new_forward_service = std::move(ServerSession::CreateForwardService());
                        serviceCreate(new_forward_service.get(), new_forward_target);
                        R_ABORT_UNLESS(this->manager->RegisterMitmSession(server_handle, std::move(object), std::move(new_forward_service)));
                    }

                    return ResultSuccess();
                }

                Result CloneCurrentObject(sf::OutMoveHandle out) {
                    return this->CloneCurrentObjectImpl(out.GetHandlePointer(), this->manager);
                }

                void QueryPointerBufferSize(sf::Out<u16> out) {
                    out.SetValue(this->session->pointer_buffer.GetSize());
                }

                Result CloneCurrentObjectEx(sf::OutMoveHandle out, u32 tag) {
                    return this->CloneCurrentObjectImpl(out.GetHandlePointer(), this->manager->GetSessionManagerByTag(tag));
                }
        };
        static_assert(IsIHipcManager<HipcManager>);

    }

    Result ServerDomainSessionManager::DispatchManagerRequest(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) {
        /* Make a stack object, and pass a shared pointer to it to DispatchRequest. */
        /* Note: This is safe, as no additional references to the hipc manager can ever be stored. */
        /* The shared pointer to stack object is definitely gross, though. */
        impl::HipcManager hipc_manager(this, session);
        return this->DispatchRequest(cmif::ServiceObjectHolder(sf::GetSharedPointerTo<impl::IHipcManager>(hipc_manager)), session, in_message, out_message);
    }

}
