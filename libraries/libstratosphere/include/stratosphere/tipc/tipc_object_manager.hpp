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
#pragma once
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_service_object.hpp>
#include <stratosphere/tipc/tipc_waitable_object.hpp>

namespace ams::tipc {

    /* TODO: Put this in a better header. */
    constexpr inline u16 MethodId_Invalid      = 0x0;
    constexpr inline u16 MethodId_CloseSession = 0xF;

    class ObjectManagerBase {
        private:
            struct Entry {
                util::TypedStorage<WaitableObject> object;
                os::WaitableHolderType waitable_holder;
            };
        private:
            os::SdkMutex m_mutex{};
            Entry *m_entries_start{};
            Entry *m_entries_end{};
            os::WaitableManagerType *m_waitable_manager{};
        private:
            Entry *FindEntry(svc::Handle handle) {
                for (Entry *cur = m_entries_start; cur != m_entries_end; ++cur) {
                    if (GetReference(cur->object).GetHandle() == handle) {
                        return cur;
                    }
                }
                return nullptr;
            }

            Entry *FindEntry(os::WaitableHolderType *holder) {
                for (Entry *cur = m_entries_start; cur != m_entries_end; ++cur) {
                    if (std::addressof(cur->waitable_holder) == holder) {
                        return cur;
                    }
                }
                return nullptr;
            }
        public:
            constexpr ObjectManagerBase() = default;

            void InitializeImpl(os::WaitableManagerType *manager, Entry *entries, size_t max_objects) {
                /* Set our waitable manager. */
                m_waitable_manager = manager;

                /* Setup entry pointers. */
                m_entries_start = entries;
                m_entries_end   = entries + max_objects;

                /* Construct all entries. */
                for (size_t i = 0; i < max_objects; ++i) {
                    util::ConstructAt(m_entries_start[i].object);
                }
            }

            void AddObject(WaitableObject &object) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Find an empty entry. */
                auto *entry = this->FindEntry(svc::InvalidHandle);
                AMS_ABORT_UNLESS(entry != nullptr);

                /* Set the entry's object. */
                GetReference(entry->object) = object;

                /* Setup the entry's holder. */
                os::InitializeWaitableHolder(std::addressof(entry->waitable_holder), object.GetHandle());
                os::LinkWaitableHolder(m_waitable_manager, std::addressof(entry->waitable_holder));
            }

            void CloseObject(svc::Handle handle) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Find the matching entry. */
                auto *entry = this->FindEntry(handle);
                AMS_ABORT_UNLESS(entry != nullptr);

                /* Finalize the entry's holder. */
                os::UnlinkWaitableHolder(std::addressof(entry->waitable_holder));
                os::FinalizeWaitableHolder(std::addressof(entry->waitable_holder));

                /* Destroy the object. */
                GetReference(entry->object).Destroy();
            }

            Result ReplyAndReceive(os::WaitableHolderType **out_holder, WaitableObject *out_object, svc::Handle reply_target, os::WaitableManagerType *manager) {
                /* Declare signaled holder for processing ahead of time. */
                os::WaitableHolderType *signaled_holder;

                /* Reply and receive until we get a newly signaled target. */
                Result result = os::SdkReplyAndReceive(out_holder, reply_target, manager);
                for (signaled_holder = *out_holder; signaled_holder == nullptr; signaled_holder = *out_holder) {
                    result = os::SdkReplyAndReceive(out_holder, svc::InvalidHandle, manager);
                }

                /* Find the entry matching the signaled holder. */
                if (auto *entry = this->FindEntry(signaled_holder); entry != nullptr) {
                    /* Get the output object. */
                    *out_object = GetReference(entry->object);
                    *out_holder = nullptr;

                    return result;
                } else {
                    return ResultSuccess();
                }
            }

            Result Reply(svc::Handle reply_target) {
                /* Perform the reply. */
                s32 dummy;
                R_TRY_CATCH(svc::ReplyAndReceive(std::addressof(dummy), nullptr, 0, reply_target, 0)) {
                    R_CATCH(svc::ResultTimedOut) {
                        /* Timing out is acceptable. */
                    }
                    R_CATCH(svc::ResultSessionClosed) {
                        /* It's okay if we couldn't reply to a closed session. */
                    }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                return ResultSuccess();
            }

            Result ProcessRequest(WaitableObject &object) {
                /* Get the method id. */
                const auto method_id = svc::ipc::MessageBuffer::MessageHeader(svc::ipc::MessageBuffer(svc::ipc::GetMessageBuffer())).GetTag();

                /* Check that the method id is valid. */
                R_UNLESS(method_id != MethodId_Invalid, tipc::ResultInvalidMethod());

                /* If we're closing the object, do so. */
                if (method_id == MethodId_CloseSession) {
                    const auto handle = object.GetHandle();

                    /* Close the object itself. */
                    this->CloseObject(handle);

                    /* Close the object's handle. */
                    /* NOTE: Nintendo does not check that this succeeds. */
                    R_ABORT_UNLESS(svc::CloseHandle(handle));

                    /* Return result to signify we closed the object. */
                    return tipc::ResultSessionClosed();
                }

                /* Process the generic method for the object. */
                R_TRY(object.GetObject()->ProcessRequest());

                return ResultSuccess();
            }
    };

    template<size_t MaxObjects>
    class ObjectManager : public ObjectManagerBase {
        private:
            Entry m_entries_storage[MaxObjects]{};
        public:
            constexpr ObjectManager() = default;

            void Initialize(os::WaitableManagerType *manager) {
                this->InitializeImpl(manager, m_entries_storage, MaxObjects);
            }
    };

}

