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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_k_spin_lock.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_k_interrupt_event.hpp>

namespace ams::kern {

    constexpr ALWAYS_INLINE util::BitPack32 GetHandleBitPack(ams::svc::Handle handle) {
        return util::BitPack32{handle};
    }

    class KProcess;
    class KThread;

    class KHandleTable {
        NON_COPYABLE(KHandleTable);
        NON_MOVEABLE(KHandleTable);
        public:
            static constexpr size_t MaxTableSize = 1024;
        private:
            using HandleRawValue = util::BitPack32::Field<0, BITSIZEOF(u32), u32>;
            using HandleEncoded  = util::BitPack32::Field<0, BITSIZEOF(ams::svc::Handle), ams::svc::Handle>;

            using HandleIndex    = util::BitPack32::Field<0,                    15, u16>;
            using HandleLinearId = util::BitPack32::Field<HandleIndex::Next,    15, u16>;
            using HandleReserved = util::BitPack32::Field<HandleLinearId::Next,  2, u32>;

            static constexpr u16 MinLinearId = 1;
            static constexpr u16 MaxLinearId = util::BitPack32{std::numeric_limits<u32>::max()}.Get<HandleLinearId>();

            static constexpr ALWAYS_INLINE ams::svc::Handle EncodeHandle(u16 index, u16 linear_id) {
                util::BitPack32 pack = {0};
                pack.Set<HandleIndex>(index);
                pack.Set<HandleLinearId>(linear_id);
                pack.Set<HandleReserved>(0);
                return pack.Get<HandleEncoded>();
            }

            union EntryInfo {
                u16 linear_id;
                s16 next_free_index;

                constexpr ALWAYS_INLINE u16 GetLinearId() const { return linear_id; }
                constexpr ALWAYS_INLINE s32 GetNextFreeIndex() const { return next_free_index; }
            };
        private:
            EntryInfo m_entry_infos[MaxTableSize];
            KAutoObject *m_objects[MaxTableSize];
            mutable KSpinLock m_lock;
            s32 m_free_head_index;
            u16 m_table_size;
            u16 m_max_count;
            u16 m_next_linear_id;
            u16 m_count;
        public:
            constexpr explicit KHandleTable(util::ConstantInitializeTag) : m_entry_infos(), m_objects(), m_lock(), m_free_head_index(-1), m_table_size(), m_max_count(), m_next_linear_id(MinLinearId), m_count() { /* ... */ }

            explicit KHandleTable() : m_lock(), m_free_head_index(-1), m_count() { MESOSPHERE_ASSERT_THIS(); }

            constexpr NOINLINE Result Initialize(s32 size) {
                MESOSPHERE_ASSERT_THIS();

                R_UNLESS(size <= static_cast<s32>(MaxTableSize), svc::ResultOutOfMemory());

                /* Initialize all fields. */
                m_max_count       = 0;
                m_table_size      = (size <= 0) ? MaxTableSize : size;
                m_next_linear_id  = MinLinearId;
                m_count           = 0;
                m_free_head_index = -1;

                /* Free all entries. */
                for (s32 i = 0; i < static_cast<s32>(m_table_size); ++i) {
                    m_objects[i]                     = nullptr;
                    m_entry_infos[i].next_free_index = i - 1;
                    m_free_head_index                = i;
                }

                return ResultSuccess();
            }

            constexpr ALWAYS_INLINE size_t GetTableSize() const { return m_table_size; }
            constexpr ALWAYS_INLINE size_t GetCount() const { return m_count; }
            constexpr ALWAYS_INLINE size_t GetMaxCount() const { return m_max_count; }

            NOINLINE Result Finalize();
            NOINLINE bool Remove(ams::svc::Handle handle);

            template<typename T = KAutoObject>
            ALWAYS_INLINE KScopedAutoObject<T> GetObjectWithoutPseudoHandle(ams::svc::Handle handle) const {
                /* Lock and look up in table. */
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(m_lock);

                if constexpr (std::is_same<T, KAutoObject>::value) {
                    return this->GetObjectImpl(handle);
                } else {
                    if (auto *obj = this->GetObjectImpl(handle); AMS_LIKELY(obj != nullptr)) {
                        return obj->DynamicCast<T*>();
                    } else {
                        return nullptr;
                    }
                }
            }

            template<typename T = KAutoObject>
            ALWAYS_INLINE KScopedAutoObject<T> GetObject(ams::svc::Handle handle) const {
                MESOSPHERE_ASSERT_THIS();

                /* Handle pseudo-handles. */
                if constexpr (std::derived_from<KProcess, T>) {
                    if (handle == ams::svc::PseudoHandle::CurrentProcess) {
                        auto * const cur_process = GetCurrentProcessPointer();
                        AMS_ASSUME(cur_process != nullptr);
                        return cur_process;
                    }
                } else if constexpr (std::derived_from<KThread, T>) {
                    if (handle == ams::svc::PseudoHandle::CurrentThread) {
                        auto * const cur_thread = GetCurrentThreadPointer();
                        AMS_ASSUME(cur_thread != nullptr);
                        return cur_thread;
                    }
                }

                return this->template GetObjectWithoutPseudoHandle<T>(handle);
            }

            KScopedAutoObject<KAutoObject> GetObjectForIpcWithoutPseudoHandle(ams::svc::Handle handle) const {
                /* Lock and look up in table. */
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(m_lock);

                KAutoObject *obj = this->GetObjectImpl(handle);
                if (AMS_LIKELY(obj != nullptr)) {
                    if (AMS_UNLIKELY(obj->DynamicCast<KInterruptEvent *>() != nullptr)) {
                        return nullptr;
                    }
                }

                return obj;
            }

            ALWAYS_INLINE KScopedAutoObject<KAutoObject> GetObjectForIpc(ams::svc::Handle handle, KThread *cur_thread) const {
                /* Handle pseudo-handles. */
                AMS_ASSUME(cur_thread != nullptr);
                if (handle == ams::svc::PseudoHandle::CurrentProcess) {
                    auto * const cur_process = static_cast<KAutoObject *>(static_cast<void *>(cur_thread->GetOwnerProcess()));
                    AMS_ASSUME(cur_process != nullptr);
                    return cur_process;
                }
                if (handle == ams::svc::PseudoHandle::CurrentThread) {
                    return static_cast<KAutoObject *>(cur_thread);
                }

                return GetObjectForIpcWithoutPseudoHandle(handle);
            }

            ALWAYS_INLINE KScopedAutoObject<KAutoObject> GetObjectByIndex(ams::svc::Handle *out_handle, size_t index) const {
                MESOSPHERE_ASSERT_THIS();
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(m_lock);

                return this->GetObjectByIndexImpl(out_handle, index);
            }

            NOINLINE Result Reserve(ams::svc::Handle *out_handle);
            NOINLINE void Unreserve(ams::svc::Handle handle);

            NOINLINE Result Add(ams::svc::Handle *out_handle, KAutoObject *obj);
            NOINLINE void Register(ams::svc::Handle handle, KAutoObject *obj);

            template<typename T>
            ALWAYS_INLINE bool GetMultipleObjects(T **out, const ams::svc::Handle *handles, size_t num_handles) const {
                /* Try to convert and open all the handles. */
                size_t num_opened;
                {
                    /* Lock the table. */
                    KScopedDisableDispatch dd;
                    KScopedSpinLock lk(m_lock);
                    for (num_opened = 0; num_opened < num_handles; num_opened++) {
                        /* Get the current handle. */
                        const auto cur_handle = handles[num_opened];

                        /* Get the object for the current handle. */
                        KAutoObject *cur_object = this->GetObjectImpl(cur_handle);
                        if (AMS_UNLIKELY(cur_object == nullptr)) {
                            break;
                        }

                        /* Cast the current object to the desired type. */
                        T *cur_t = cur_object->DynamicCast<T*>();
                        if (AMS_UNLIKELY(cur_t == nullptr)) {
                            break;
                        }

                        /* Open a reference to the current object. */
                        cur_t->Open();
                        out[num_opened] = cur_t;
                    }
                }

                /* If we converted every object, succeed. */
                if (AMS_LIKELY(num_opened == num_handles)) {
                    return true;
                }

                /* If we didn't convert entry object, close the ones we opened. */
                for (size_t i = 0; i < num_opened; i++) {
                    out[i]->Close();
                }

                return false;
            }
        private:

            constexpr ALWAYS_INLINE s32 AllocateEntry() {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(m_count < m_table_size);

                const auto index  = m_free_head_index;

                m_free_head_index = m_entry_infos[index].GetNextFreeIndex();

                m_max_count = std::max(m_max_count, ++m_count);

                return index;
            }

            constexpr ALWAYS_INLINE void FreeEntry(s32 index) {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(m_count > 0);

                m_objects[index]                     = nullptr;
                m_entry_infos[index].next_free_index = m_free_head_index;

                m_free_head_index = index;

                --m_count;
            }

            constexpr ALWAYS_INLINE u16 AllocateLinearId() {
                const u16 id = m_next_linear_id++;
                if (m_next_linear_id > MaxLinearId) {
                    m_next_linear_id = MinLinearId;
                }
                return id;
            }

            constexpr ALWAYS_INLINE bool IsValidHandle(ams::svc::Handle handle) const {
                MESOSPHERE_ASSERT_THIS();

                /* Unpack the handle. */
                const auto handle_pack = GetHandleBitPack(handle);
                const auto raw_value   = handle_pack.Get<HandleRawValue>();
                const auto index       = handle_pack.Get<HandleIndex>();
                const auto linear_id   = handle_pack.Get<HandleLinearId>();
                const auto reserved    = handle_pack.Get<HandleReserved>();
                MESOSPHERE_ASSERT(reserved == 0);
                MESOSPHERE_UNUSED(reserved);

                /* Validate our indexing information. */
                if (AMS_UNLIKELY(raw_value == 0)) {
                    return false;
                }
                if (AMS_UNLIKELY(linear_id == 0)) {
                    return false;
                }
                if (AMS_UNLIKELY(index >= m_table_size)) {
                    return false;
                }

                /* Check that there's an object, and our serial id is correct. */
                if (AMS_UNLIKELY(m_objects[index] == nullptr)) {
                    return false;
                }
                if (AMS_UNLIKELY(m_entry_infos[index].GetLinearId() != linear_id)) {
                    return false;
                }

                return true;
            }

            constexpr NOINLINE KAutoObject *GetObjectImpl(ams::svc::Handle handle) const {
                MESOSPHERE_ASSERT_THIS();

                /* Handles must not have reserved bits set. */
                const auto handle_pack = GetHandleBitPack(handle);
                if (AMS_UNLIKELY(handle_pack.Get<HandleReserved>() != 0)) {
                    return nullptr;
                }

                if (AMS_LIKELY(this->IsValidHandle(handle))) {
                    return m_objects[handle_pack.Get<HandleIndex>()];
                } else {
                    return nullptr;
                }
            }

            constexpr ALWAYS_INLINE KAutoObject *GetObjectByIndexImpl(ams::svc::Handle *out_handle, size_t index) const {
                MESOSPHERE_ASSERT_THIS();

                /* Index must be in bounds. */
                if (AMS_UNLIKELY(index >= m_table_size)) {
                    return nullptr;
                }

                /* Ensure entry has an object. */
                if (KAutoObject *obj = m_objects[index]; obj != nullptr) {
                    *out_handle = EncodeHandle(index, m_entry_infos[index].GetLinearId());
                    return obj;
                } else {
                    return nullptr;
                }
            }
    };

}
