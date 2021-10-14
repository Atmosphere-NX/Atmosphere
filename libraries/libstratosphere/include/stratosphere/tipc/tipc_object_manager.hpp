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
#include <vapours.hpp>
#include <stratosphere/tipc/tipc_common.hpp>
#include <stratosphere/tipc/tipc_service_object.hpp>
#include <stratosphere/tipc/tipc_object_holder.hpp>

namespace ams::tipc {

    /* TODO: Put this in a better header. */
    constexpr inline u16 MethodId_Invalid      = 0x0;
    constexpr inline u16 MethodId_CloseSession = 0xF;

    class ObjectManagerBase {
        protected:
            struct Entry {
                util::TypedStorage<ObjectHolder> object;
                os::MultiWaitHolderType multi_wait_holder;
            };
        private:
            os::SdkMutex m_mutex{};
            Entry *m_entries_start{};
            Entry *m_entries_end{};
            os::MultiWaitType *m_multi_wait{};
        private:
            Entry *FindEntry(os::NativeHandle handle) {
                for (Entry *cur = m_entries_start; cur != m_entries_end; ++cur) {
                    if (GetReference(cur->object).GetHandle() == handle) {
                        return cur;
                    }
                }
                return nullptr;
            }

            Entry *FindEntry(os::MultiWaitHolderType *holder) {
                for (Entry *cur = m_entries_start; cur != m_entries_end; ++cur) {
                    if (std::addressof(cur->multi_wait_holder) == holder) {
                        return cur;
                    }
                }
                return nullptr;
            }
        public:
            constexpr ObjectManagerBase() = default;

            void InitializeImpl(os::MultiWaitType *multi_wait, Entry *entries, size_t max_objects) {
                /* Set our multi wait. */
                m_multi_wait = multi_wait;

                /* Setup entry pointers. */
                m_entries_start = entries;
                m_entries_end   = entries + max_objects;

                /* Construct all entries. */
                for (size_t i = 0; i < max_objects; ++i) {
                    util::ConstructAt(m_entries_start[i].object);
                }
            }

            void AddObject(ObjectHolder &object) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Find an empty entry. */
                auto *entry = this->FindEntry(os::InvalidNativeHandle);
                AMS_ABORT_UNLESS(entry != nullptr);

                /* Set the entry's object. */
                GetReference(entry->object) = object;

                /* Setup the entry's holder. */
                os::InitializeMultiWaitHolder(std::addressof(entry->multi_wait_holder), object.GetHandle());
                os::LinkMultiWaitHolder(m_multi_wait, std::addressof(entry->multi_wait_holder));
            }

            void CloseObject(os::NativeHandle handle) {
                /* Lock ourselves. */
                std::scoped_lock lk(m_mutex);

                /* Find the matching entry. */
                auto *entry = this->FindEntry(handle);
                AMS_ABORT_UNLESS(entry != nullptr);

                /* Finalize the entry's holder. */
                os::UnlinkMultiWaitHolder(std::addressof(entry->multi_wait_holder));
                os::FinalizeMultiWaitHolder(std::addressof(entry->multi_wait_holder));

                /* Destroy the object. */
                GetReference(entry->object).Destroy();
            }

            Result ReplyAndReceive(os::MultiWaitHolderType **out_holder, ObjectHolder *out_object, os::NativeHandle reply_target, os::MultiWaitType *multi_wait) {
                /* Declare signaled holder for processing ahead of time. */
                os::MultiWaitHolderType *signaled_holder;

                /* Reply and receive until we get a newly signaled target. */
                Result result = os::SdkReplyAndReceive(out_holder, reply_target, multi_wait);
                for (signaled_holder = *out_holder; signaled_holder == nullptr; signaled_holder = *out_holder) {
                    result = os::SdkReplyAndReceive(out_holder, os::InvalidNativeHandle, multi_wait);
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

            void Reply(os::NativeHandle reply_target) {
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
            }

            Result ProcessRequest(ObjectHolder &object) {
                /* Get the message buffer. */
                const svc::ipc::MessageBuffer message_buffer(svc::ipc::GetMessageBuffer());

                /* Get the method id. */
                const auto method_id = svc::ipc::MessageBuffer::MessageHeader(message_buffer).GetTag();

                /* Ensure that we clean up any handles that get sent our way. */
                auto handle_guard = SCOPE_GUARD {
                    const svc::ipc::MessageBuffer::MessageHeader message_header(message_buffer);
                    const svc::ipc::MessageBuffer::SpecialHeader special_header(message_buffer, message_header);

                    /* Determine the offset to the start of handles. */
                    auto offset = message_buffer.GetSpecialDataIndex(message_header, special_header);
                    if (special_header.GetHasProcessId()) {
                        offset += sizeof(u64) / sizeof(u32);
                    }

                    /* Close all copy handles. */
                    for (auto i = 0; i < special_header.GetCopyHandleCount(); ++i) {
                        svc::CloseHandle(message_buffer.GetHandle(offset));
                        offset += sizeof(ams::svc::Handle) / sizeof(u32);
                    }
                };

                /* Check that the method id is valid. */
                R_UNLESS(method_id != MethodId_Invalid, tipc::ResultInvalidMethod());

                /* If we're closing the object, do so. */
                if (method_id == MethodId_CloseSession) {
                    /* Validate the command format. */
                    {
                        using CloseSessionCommandMeta = impl::CommandMetaInfo<MethodId_CloseSession, std::tuple<>>;
                        using CloseSessionProcessor   = impl::CommandProcessor<CloseSessionCommandMeta>;

                        /* Validate that the command is valid. */
                        R_TRY(CloseSessionProcessor::ValidateCommandFormat(message_buffer));
                    }

                    /* Get the object handle. */
                    const auto handle = object.GetHandle();

                    /* Close the object itself. */
                    this->CloseObject(handle);

                    /* Close the object's handle. */
                    /* NOTE: Nintendo does not check that this succeeds. */
                    R_ABORT_UNLESS(svc::CloseHandle(handle));

                    /* Return result to signify we closed the object (and don't close input handles). */
                    handle_guard.Cancel();
                    return tipc::ResultSessionClosed();
                }

                /* Process the generic method for the object. */
                R_TRY(object.GetObject()->ProcessRequest());

                /* We successfully processed, so we don't need to clean up handles. */
                handle_guard.Cancel();
                return ResultSuccess();
            }
    };

    template<size_t MaxObjects>
    class ObjectManager : public ObjectManagerBase {
        private:
            Entry m_entries_storage[MaxObjects]{};
        public:
            constexpr ObjectManager() = default;

            void Initialize(os::MultiWaitType *multi_wait) {
                this->InitializeImpl(multi_wait, m_entries_storage, MaxObjects);
            }
    };

}

