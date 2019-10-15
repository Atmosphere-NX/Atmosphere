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
#include <stratosphere.hpp>

namespace sts::sf::hipc {

    namespace impl {

        class HipcManager : public IServiceObject {
            private:
                enum class CommandId {
                    ConvertCurrentObjectToDomain = 0,
                    CopyFromCurrentDomain        = 1,
                    CloneCurrentObject           = 2,
                    QueryPointerBufferSize       = 3,
                    CloneCurrentObjectEx         = 4,
                };
            private:
                ServerDomainSessionManager *manager;
                ServerSession *session;
                bool is_mitm_session;
            private:
                Result CloneCurrentObjectImpl(Handle *out_client_handle, ServerSessionManager *tagged_manager) {
                    /* Clone the object. */
                    cmif::ServiceObjectHolder &&clone = this->session->srv_obj_holder.Clone();
                    R_UNLESS(clone, ResultHipcDomainObjectNotFound);

                    /* Create new session handles. */
                    Handle server_handle;
                    R_ASSERT(hipc::CreateSession(&server_handle, out_client_handle));

                    /* Register with manager. */
                    if (!is_mitm_session) {
                        R_ASSERT(tagged_manager->RegisterSession(server_handle, std::move(clone)));
                    } else {
                        /* Clone the forward service. */
                        std::shared_ptr<::Service> new_forward_service(new ::Service(), [](::Service *srv) {
                            serviceClose(srv);
                            delete srv;
                        });
                        R_ASSERT(serviceClone(this->session->forward_service.get(), new_forward_service.get()));
                        R_ASSERT(tagged_manager->RegisterMitmSession(server_handle, std::move(clone), std::move(new_forward_service)));
                    }

                    return ResultSuccess;
                }
            public:
                explicit HipcManager(ServerDomainSessionManager *m, ServerSession *s) : manager(m), session(s), is_mitm_session(s->forward_service != nullptr) {
                    /* ... */
                }

                Result ConvertCurrentObjectToDomain(sf::Out<cmif::DomainObjectId> out) {
                    /* TODO */
                    return ResultHipcOutOfDomains;
                }

                Result CopyFromCurrentDomain(sf::OutMoveHandle out, cmif::DomainObjectId object_id) {
                    /* Get domain. */
                    auto domain = this->session->srv_obj_holder.GetServiceObject<cmif::DomainServiceObject>();
                    R_UNLESS(domain != nullptr, ResultHipcTargetNotDomain);

                    /* Get domain object. */
                    auto &&object = domain->GetObject(object_id);
                    if (!object) {
                        R_UNLESS(this->is_mitm_session, ResultHipcDomainObjectNotFound);
                        return cmifCopyFromCurrentDomain(this->session->forward_service->session, object_id.value, out.GetHandlePointer());
                    }

                    if (!this->is_mitm_session || object_id.value != serviceGetObjectId(this->session->forward_service.get())) {
                        /* Create new session handles. */
                        Handle server_handle;
                        R_ASSERT(hipc::CreateSession(&server_handle, out.GetHandlePointer()));

                        /* Register. */
                        R_ASSERT(this->manager->RegisterSession(server_handle, std::move(object)));
                    } else {
                        /* Copy from the target domain. */
                        Handle new_forward_target;
                        R_TRY(cmifCopyFromCurrentDomain(this->session->forward_service->session, object_id.value, &new_forward_target));

                        /* Create new session handles. */
                        Handle server_handle;
                        R_ASSERT(hipc::CreateSession(&server_handle, out.GetHandlePointer()));

                        /* Register. */
                        std::shared_ptr<::Service> new_forward_service(new ::Service(), [](::Service *srv) {
                            serviceClose(srv);
                            delete srv;
                        });
                        serviceCreate(new_forward_service.get(), new_forward_target);
                        R_ASSERT(this->manager->RegisterMitmSession(server_handle, std::move(object), std::move(new_forward_service)));
                    }

                    return ResultSuccess;
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

            public:
                DEFINE_SERVICE_DISPATCH_TABLE {
                    MAKE_SERVICE_COMMAND_META(ConvertCurrentObjectToDomain),
                    MAKE_SERVICE_COMMAND_META(CopyFromCurrentDomain),
                    MAKE_SERVICE_COMMAND_META(CloneCurrentObject),
                    MAKE_SERVICE_COMMAND_META(QueryPointerBufferSize),
                    MAKE_SERVICE_COMMAND_META(CloneCurrentObjectEx),
                };
        };

    }

    Result ServerDomainSessionManager::DispatchManagerRequest(ServerSession *session, const cmif::PointerAndSize &in_message, const cmif::PointerAndSize &out_message) {
        /* Make a stack object, and pass a shared pointer to it to DispatchRequest. */
        /* Note: This is safe, as no additional references to the hipc manager can ever be stored. */
        /* The shared pointer to stack object is definitely gross, though. */
        impl::HipcManager hipc_manager(this, session);
        return this->DispatchRequest(cmif::ServiceObjectHolder(std::shared_ptr<impl::HipcManager>(&hipc_manager, [](impl::HipcManager *) { /* Empty deleter. */ })), session, in_message, out_message);
    }

}
