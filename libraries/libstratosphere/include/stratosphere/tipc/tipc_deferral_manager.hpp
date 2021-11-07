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
#include <stratosphere/tipc/tipc_object_holder.hpp>
#include <stratosphere/tipc/tipc_service_object_base.hpp>

namespace ams::tipc {

    template<typename T>
    concept IsResumeKey = util::is_pod<T>::value && (0 < sizeof(T) && sizeof(T) <= sizeof(uintptr_t));

    template<IsResumeKey ResumeKey>
    static constexpr ALWAYS_INLINE uintptr_t ConvertToInternalResumeKey(ResumeKey key) {
        if constexpr (std::same_as<ResumeKey, uintptr_t>) {
            return key;
        } else if constexpr (sizeof(key) == sizeof(uintptr_t)) {
            return std::bit_cast<uintptr_t>(key);
        } else {
            uintptr_t converted = 0;
            std::memcpy(std::addressof(converted), std::addressof(key), sizeof(key));
            return converted;
        }
    }

    class DeferralManagerBase;

    namespace impl {

        class DeferrableBaseTag{};

    }

    class DeferrableBaseImpl : public impl::DeferrableBaseTag {
        private:
            DeferralManagerBase *m_deferral_manager;
            ObjectHolder m_object_holder;
            uintptr_t m_resume_key;
            const u32 m_message_buffer_size;
            u8 m_message_buffer_base[0];
        public:
            ALWAYS_INLINE DeferrableBaseImpl(u32 mb_size) : m_deferral_manager(nullptr), m_object_holder(), m_resume_key(), m_message_buffer_size(mb_size) { /* ... */ }

            ~DeferrableBaseImpl();

            ALWAYS_INLINE void SetDeferralManager(DeferralManagerBase *manager, os::NativeHandle reply_target, ServiceObjectBase *object) {
                m_deferral_manager = manager;
                m_object_holder.InitializeForDeferralManager(reply_target, object);
            }

            ALWAYS_INLINE bool TestResume(uintptr_t key) const {
                return m_resume_key == key;
            }

            template<IsResumeKey ResumeKey>
            ALWAYS_INLINE void RegisterRetry(ResumeKey key) {
                m_resume_key = ConvertToInternalResumeKey(key);
                std::memcpy(m_message_buffer_base, svc::ipc::GetMessageBuffer(), m_message_buffer_size);
            }

            template<IsResumeKey ResumeKey, typename F>
            ALWAYS_INLINE Result RegisterRetryIfDeferred(ResumeKey key, F f) {
                const Result result = f();
                if (tipc::ResultRequestDeferred::Includes(result)) {
                    this->RegisterRetry(key);
                }
                return result;
            }

            template<typename PortManager>
            ALWAYS_INLINE void TriggerResume(PortManager *port_manager) {
                /* Clear resume key. */
                m_resume_key = 0;

                /* Restore message buffer. */
                std::memcpy(svc::ipc::GetMessageBuffer(), m_message_buffer_base, m_message_buffer_size);

                /* Process the request. */
                return port_manager->ProcessDeferredRequest(m_object_holder);
            }
        protected:
            static consteval size_t GetMessageBufferOffsetBase();
    };
    static_assert(std::is_standard_layout<DeferrableBaseImpl>::value);

    template<size_t _MessageBufferRequiredSize>
    class DeferrableBaseImplWithBuffer : public DeferrableBaseImpl {
        private:
            static constexpr size_t MessageBufferRequiredSize = _MessageBufferRequiredSize;
        private:
            u8 m_message_buffer[MessageBufferRequiredSize];
        public:
            DeferrableBaseImplWithBuffer();
        private:
            static consteval size_t GetMessageBufferOffset();
    };

    consteval size_t DeferrableBaseImpl::GetMessageBufferOffsetBase() {
        return AMS_OFFSETOF(DeferrableBaseImpl, m_message_buffer_base);
    }

    template<size_t _MessageBufferRequiredSize>
    consteval size_t DeferrableBaseImplWithBuffer<_MessageBufferRequiredSize>::GetMessageBufferOffset() {
        return AMS_OFFSETOF(DeferrableBaseImplWithBuffer<_MessageBufferRequiredSize>, m_message_buffer);
    }

    template<size_t _MessageBufferRequiredSize>
    ALWAYS_INLINE DeferrableBaseImplWithBuffer<_MessageBufferRequiredSize>::DeferrableBaseImplWithBuffer() : DeferrableBaseImpl(MessageBufferRequiredSize) {
        static_assert(GetMessageBufferOffsetBase() == GetMessageBufferOffset());
        static_assert(sizeof(DeferrableBaseImplWithBuffer<_MessageBufferRequiredSize>) >= sizeof(DeferrableBaseImpl) + MessageBufferRequiredSize);
    }

    template<typename Interface, size_t MaximumDefaultRequestSize = 0>
    class DeferrableBase : public DeferrableBaseImplWithBuffer<std::max(Interface::MaximumRequestSize, MaximumDefaultRequestSize)> {
        private:
            using BaseImpl = DeferrableBaseImplWithBuffer<std::max(Interface::MaximumRequestSize, MaximumDefaultRequestSize)>;
        public:
            using BaseImpl::BaseImpl;
    };

    template<class T>
    concept IsDeferrable = std::derived_from<T, impl::DeferrableBaseTag>;

    class DeferralManagerBase {
        NON_COPYABLE(DeferralManagerBase);
        NON_MOVEABLE(DeferralManagerBase);
        private:
            size_t m_object_count;
            DeferrableBaseImpl *m_objects_base[0];
        public:
            ALWAYS_INLINE DeferralManagerBase() : m_object_count(0) { /* ... */  }

            void AddObject(DeferrableBaseImpl &object, os::NativeHandle reply_target, ServiceObjectBase *service_object) {
                /* Set ourselves as the manager for the object. */
                object.SetDeferralManager(this, reply_target, service_object);

                /* Add the object to our entries. */
                AMS_ASSERT(m_object_count < N);
                m_objects_base[m_object_count++] = std::addressof(object);
            }

            void RemoveObject(DeferrableBaseImpl *object) {
                /* If the object is present, remove it. */
                for (size_t i = 0; i < m_object_count; ++i) {
                    if (m_objects_base[i] == object) {
                        std::swap(m_objects_base[i], m_objects_base[--m_object_count]);
                        break;
                    }
                }
            }

            ALWAYS_INLINE bool TestResume(uintptr_t resume_key) const {
                /* Try to resume all entries. */
                for (size_t i = 0; i < m_object_count; ++i) {
                    if (m_objects_base[i]->TestResume(resume_key)) {
                        return true;
                    }
                }

                return false;
            }

            template<typename PortManager>
            ALWAYS_INLINE void TriggerResume(PortManager *port_manager, uintptr_t resume_key) const {
                /* Try to resume all entries. */
                for (size_t i = 0; i < m_object_count; ++i) {
                    if (m_objects_base[i]->TestResume(resume_key)) {
                        m_objects_base[i]->TriggerResume(port_manager);
                    }
                }
            }
        protected:
            static consteval size_t GetObjectPointersOffsetBase();
    };
    static_assert(std::is_standard_layout<DeferralManagerBase>::value);

    inline DeferrableBaseImpl::~DeferrableBaseImpl() {
        AMS_ASSUME(m_deferral_manager != nullptr);
        m_deferral_manager->RemoveObject(this);
    }

    template<size_t N> requires (N > 0)
    class DeferralManager final : public DeferralManagerBase {
        private:
            DeferrableBaseImpl *m_objects[N];
        public:
            DeferralManager();
        private:
            static consteval size_t GetObjectPointersOffset();
    };

    consteval size_t DeferralManagerBase::GetObjectPointersOffsetBase() {
        return AMS_OFFSETOF(DeferralManagerBase, m_objects_base);
    }

    template<size_t N> requires (N > 0)
    consteval size_t DeferralManager<N>::GetObjectPointersOffset() {
        return AMS_OFFSETOF(DeferralManager<N>, m_objects);
    }

    template<size_t N> requires (N > 0)
    inline DeferralManager<N>::DeferralManager() : DeferralManagerBase() {
        static_assert(GetObjectPointersOffset() == GetObjectPointersOffsetBase());
        static_assert(sizeof(DeferralManager<N>) == sizeof(DeferralManagerBase) + N * sizeof(DeferrableBaseImpl *));
    }

}

