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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_k_class_token.hpp>

namespace ams::kern {

    class KProcess;

    #define MESOSPHERE_AUTOOBJECT_TRAITS(CLASS, BASE_CLASS)                                                     \
        NON_COPYABLE(CLASS);                                                                                    \
        NON_MOVEABLE(CLASS);                                                                                    \
        private:                                                                                                \
            friend class ::ams::kern::KClassTokenGenerator;                                                     \
            static constexpr inline auto ObjectType = ::ams::kern::KClassTokenGenerator::ObjectType::CLASS;     \
            static constexpr inline const char * const TypeName = #CLASS;                                       \
            static constexpr inline ClassTokenType ClassToken() { return ::ams::kern::ClassToken<CLASS>; }      \
        public:                                                                                                 \
            using BaseClass = BASE_CLASS;                                                                       \
            static constexpr ALWAYS_INLINE TypeObj GetStaticTypeObj() {                                         \
                constexpr ClassTokenType Token = ClassToken();                                                  \
                return TypeObj(TypeName, Token);                                                                \
            }                                                                                                   \
            static constexpr ALWAYS_INLINE const char *GetStaticTypeName() { return TypeName; }                 \
            virtual TypeObj GetTypeObj() const { return GetStaticTypeObj(); }                                   \
            virtual const char *GetTypeName() { return GetStaticTypeName(); }                                   \
        private:


    class KAutoObject {
        protected:
            class TypeObj {
                private:
                    const char *m_name;
                    ClassTokenType m_class_token;
                public:
                    constexpr explicit TypeObj(const char *n, ClassTokenType tok) : m_name(n), m_class_token(tok) { /* ... */ }

                    constexpr ALWAYS_INLINE const char *GetName() const { return m_name; }
                    constexpr ALWAYS_INLINE ClassTokenType GetClassToken() const { return m_class_token; }

                    constexpr ALWAYS_INLINE bool operator==(const TypeObj &rhs) {
                        return this->GetClassToken() == rhs.GetClassToken();
                    }

                    constexpr ALWAYS_INLINE bool operator!=(const TypeObj &rhs) {
                        return this->GetClassToken() != rhs.GetClassToken();
                    }

                    constexpr ALWAYS_INLINE bool IsDerivedFrom(const TypeObj &rhs) {
                        return (this->GetClassToken() | rhs.GetClassToken()) == this->GetClassToken();
                    }
            };
        private:
            MESOSPHERE_AUTOOBJECT_TRAITS(KAutoObject, KAutoObject);
        private:
            std::atomic<u32> m_ref_count;
        public:
            static KAutoObject *Create(KAutoObject *ptr);
        public:
            constexpr ALWAYS_INLINE explicit KAutoObject() : m_ref_count(0) { MESOSPHERE_ASSERT_THIS(); }
            virtual ~KAutoObject() { MESOSPHERE_ASSERT_THIS(); }

            /* Destroy is responsible for destroying the auto object's resources when ref_count hits zero. */
            virtual void Destroy() { MESOSPHERE_ASSERT_THIS(); }

            /* Finalize is responsible for cleaning up resource, but does not destroy the object. */
            virtual void Finalize() { MESOSPHERE_ASSERT_THIS(); }

            virtual KProcess *GetOwner() const { return nullptr; }

            u32 GetReferenceCount() const {
                return m_ref_count.load();
            }

            ALWAYS_INLINE bool IsDerivedFrom(const TypeObj &rhs) const {
                return this->GetTypeObj().IsDerivedFrom(rhs);
            }

            ALWAYS_INLINE bool IsDerivedFrom(const KAutoObject &rhs) const {
                return this->IsDerivedFrom(rhs.GetTypeObj());
            }

            template<typename Derived>
            ALWAYS_INLINE Derived DynamicCast() {
                static_assert(std::is_pointer<Derived>::value);
                using DerivedType = typename std::remove_pointer<Derived>::type;

                if (AMS_LIKELY(this->IsDerivedFrom(DerivedType::GetStaticTypeObj()))) {
                    return static_cast<Derived>(this);
                } else {
                    return nullptr;
                }
            }

            template<typename Derived>
            ALWAYS_INLINE const Derived DynamicCast() const {
                static_assert(std::is_pointer<Derived>::value);
                using DerivedType = typename std::remove_pointer<Derived>::type;

                if (AMS_LIKELY(this->IsDerivedFrom(DerivedType::GetStaticTypeObj()))) {
                    return static_cast<Derived>(this);
                } else {
                    return nullptr;
                }
            }

            ALWAYS_INLINE bool Open() {
                MESOSPHERE_ASSERT_THIS();

                /* Atomically increment the reference count, only if it's positive. */
                u32 cur_ref_count = m_ref_count.load(std::memory_order_acquire);
                do {
                    if (AMS_UNLIKELY(cur_ref_count == 0)) {
                        MESOSPHERE_AUDIT(cur_ref_count != 0);
                        return false;
                    }
                    MESOSPHERE_ABORT_UNLESS(cur_ref_count < cur_ref_count + 1);
                } while (!m_ref_count.compare_exchange_weak(cur_ref_count, cur_ref_count + 1, std::memory_order_relaxed));

                return true;
            }

            ALWAYS_INLINE void Close() {
                MESOSPHERE_ASSERT_THIS();

                /* Atomically decrement the reference count, not allowing it to become negative. */
                u32 cur_ref_count = m_ref_count.load(std::memory_order_acquire);
                do {
                    MESOSPHERE_ABORT_UNLESS(cur_ref_count > 0);
                } while (!m_ref_count.compare_exchange_weak(cur_ref_count, cur_ref_count - 1, std::memory_order_relaxed));

                /* If ref count hits zero, destroy the object. */
                if (cur_ref_count - 1 == 0) {
                    this->Destroy();
                }
            }
    };

    class KAutoObjectWithListContainer;

    class KAutoObjectWithList : public KAutoObject {
        private:
            friend class KAutoObjectWithListContainer;
        private:
            util::IntrusiveRedBlackTreeNode list_node;
        public:
            static ALWAYS_INLINE int Compare(const KAutoObjectWithList &lhs, const KAutoObjectWithList &rhs) {
                const u64 lid = lhs.GetId();
                const u64 rid = rhs.GetId();

                if (lid < rid) {
                    return -1;
                } else if (lid > rid) {
                    return 1;
                } else {
                    return 0;
                }
            }
        public:
            virtual u64 GetId() const {
                return reinterpret_cast<u64>(this);
            }
    };

    template<typename T> requires std::derived_from<T, KAutoObject>
    class KScopedAutoObject {
        NON_COPYABLE(KScopedAutoObject);
        private:
            template<typename U>
            friend class KScopedAutoObject;
        private:
            T *m_obj;
        private:
            constexpr ALWAYS_INLINE void Swap(KScopedAutoObject &rhs) {
                std::swap(m_obj, rhs.m_obj);
            }
        public:
            constexpr ALWAYS_INLINE KScopedAutoObject() : m_obj(nullptr) { /* ... */ }
            constexpr ALWAYS_INLINE KScopedAutoObject(T *o) : m_obj(o) {
                if (m_obj != nullptr) {
                    m_obj->Open();
                }
            }

            ~KScopedAutoObject() {
                if (m_obj != nullptr) {
                    m_obj->Close();
                }
                m_obj = nullptr;
            }

            template<typename U> requires (std::derived_from<T, U> || std::derived_from<U, T>)
            constexpr KScopedAutoObject(KScopedAutoObject<U> &&rhs) {
                if constexpr (std::derived_from<U, T>) {
                    /* Upcast. */
                    m_obj = rhs.m_obj;
                    rhs.m_obj = nullptr;
                } else {
                    /* Downcast. */
                    T *derived = nullptr;
                    if (rhs.m_obj != nullptr) {
                        derived = rhs.m_obj->template DynamicCast<T *>();
                        if (derived == nullptr) {
                            rhs.m_obj->Close();
                        }
                    }

                    m_obj = derived;
                    rhs.m_obj = nullptr;
                }
            }

            constexpr ALWAYS_INLINE KScopedAutoObject<T> &operator=(KScopedAutoObject<T> &&rhs) {
                rhs.Swap(*this);
                return *this;
            }

            constexpr ALWAYS_INLINE T *operator->() { return m_obj; }
            constexpr ALWAYS_INLINE T &operator*() { return *m_obj; }

            constexpr ALWAYS_INLINE void Reset(T *o) {
                KScopedAutoObject(o).Swap(*this);
            }

            constexpr ALWAYS_INLINE T *GetPointerUnsafe() { return m_obj; }

            constexpr ALWAYS_INLINE T *ReleasePointerUnsafe() { T *ret = m_obj; m_obj = nullptr; return ret; }

            constexpr ALWAYS_INLINE bool IsNull() const { return m_obj == nullptr; }
            constexpr ALWAYS_INLINE bool IsNotNull() const { return m_obj != nullptr; }
    };


}
