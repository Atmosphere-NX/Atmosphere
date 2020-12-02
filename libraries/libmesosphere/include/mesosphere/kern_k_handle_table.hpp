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

            class Entry {
                private:
                    union {
                        struct {
                            u16 linear_id;
                            u16 type;
                        } info;
                        Entry *next_free_entry;
                    } meta;
                    KAutoObject *object;
                public:
                    constexpr Entry() : meta(), object(nullptr) { /* ... */ }

                    constexpr ALWAYS_INLINE void SetFree(Entry *next) {
                        this->object = nullptr;
                        this->meta.next_free_entry = next;
                    }

                    constexpr ALWAYS_INLINE void SetUsed(KAutoObject *obj, u16 linear_id, u16 type) {
                        this->object = obj;
                        this->meta.info = { linear_id, type };
                    }

                    constexpr ALWAYS_INLINE KAutoObject *GetObject() const { return this->object; }
                    constexpr ALWAYS_INLINE Entry *GetNextFreeEntry() const { return this->meta.next_free_entry; }
                    constexpr ALWAYS_INLINE u16 GetLinearId() const { return this->meta.info.linear_id; }
                    constexpr ALWAYS_INLINE u16 GetType() const { return this->meta.info.type; }
            };
        private:
            mutable KSpinLock lock;
            Entry *table;
            Entry *free_head;
            Entry entries[MaxTableSize];
            u16 table_size;
            u16 max_count;
            u16 next_linear_id;
            u16 count;
        public:
            constexpr KHandleTable() :
                lock(), table(nullptr), free_head(nullptr), entries(), table_size(0), max_count(0), next_linear_id(MinLinearId), count(0)
            { MESOSPHERE_ASSERT_THIS(); }

            constexpr NOINLINE Result Initialize(s32 size) {
                MESOSPHERE_ASSERT_THIS();

                R_UNLESS(size <= static_cast<s32>(MaxTableSize), svc::ResultOutOfMemory());

                /* Initialize all fields. */
                this->table = this->entries;
                this->table_size = (size <= 0) ? MaxTableSize : size;
                this->next_linear_id = MinLinearId;
                this->count = 0;
                this->max_count = 0;

                /* Free all entries. */
                for (size_t i = 0; i < static_cast<size_t>(this->table_size - 1); i++) {
                    this->entries[i].SetFree(std::addressof(this->entries[i + 1]));
                }
                this->entries[this->table_size - 1].SetFree(nullptr);

                this->free_head = std::addressof(this->entries[0]);

                return ResultSuccess();
            }

            constexpr ALWAYS_INLINE size_t GetTableSize() const { return this->table_size; }
            constexpr ALWAYS_INLINE size_t GetCount() const { return this->count; }
            constexpr ALWAYS_INLINE size_t GetMaxCount() const { return this->max_count; }

            NOINLINE Result Finalize();
            NOINLINE bool Remove(ams::svc::Handle handle);

            template<typename T = KAutoObject>
            ALWAYS_INLINE KScopedAutoObject<T> GetObjectWithoutPseudoHandle(ams::svc::Handle handle) const {
                /* Lock and look up in table. */
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(this->lock);

                if constexpr (std::is_same<T, KAutoObject>::value) {
                    return this->GetObjectImpl(handle);
                } else {
                    if (auto *obj = this->GetObjectImpl(handle); obj != nullptr) {
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
                        return GetCurrentProcessPointer();
                    }
                } else if constexpr (std::derived_from<KThread, T>) {
                    if (handle == ams::svc::PseudoHandle::CurrentThread) {
                        return GetCurrentThreadPointer();
                    }
                }

                return this->template GetObjectWithoutPseudoHandle<T>(handle);
            }

            KScopedAutoObject<KAutoObject> GetObjectForIpcWithoutPseudoHandle(ams::svc::Handle handle) const {
                /* Lock and look up in table. */
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(this->lock);

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
                if (handle == ams::svc::PseudoHandle::CurrentProcess) {
                    return static_cast<KAutoObject *>(static_cast<void *>(cur_thread->GetOwnerProcess()));
                }
                if (handle == ams::svc::PseudoHandle::CurrentThread) {
                    return static_cast<KAutoObject *>(cur_thread);
                }

                return GetObjectForIpcWithoutPseudoHandle(handle);
            }

            ALWAYS_INLINE KScopedAutoObject<KAutoObject> GetObjectByIndex(ams::svc::Handle *out_handle, size_t index) const {
                MESOSPHERE_ASSERT_THIS();
                KScopedDisableDispatch dd;
                KScopedSpinLock lk(this->lock);

                return this->GetObjectByIndexImpl(out_handle, index);
            }

            NOINLINE Result Reserve(ams::svc::Handle *out_handle);
            NOINLINE void Unreserve(ams::svc::Handle handle);

            template<typename T>
            ALWAYS_INLINE Result Add(ams::svc::Handle *out_handle, T *obj) {
                static_assert(std::is_base_of<KAutoObject, T>::value);
                return this->Add(out_handle, obj, obj->GetTypeObj().GetClassToken());
            }

            template<typename T>
            ALWAYS_INLINE void Register(ams::svc::Handle handle, T *obj) {
                static_assert(std::is_base_of<KAutoObject, T>::value);
                return this->Register(handle, obj, obj->GetTypeObj().GetClassToken());
            }

            template<typename T>
            ALWAYS_INLINE bool GetMultipleObjects(T **out, const ams::svc::Handle *handles, size_t num_handles) const {
                /* Try to convert and open all the handles. */
                size_t num_opened;
                {
                    /* Lock the table. */
                    KScopedDisableDispatch dd;
                    KScopedSpinLock lk(this->lock);
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
            NOINLINE Result Add(ams::svc::Handle *out_handle, KAutoObject *obj, u16 type);
            NOINLINE void Register(ams::svc::Handle handle, KAutoObject *obj, u16 type);

            constexpr ALWAYS_INLINE Entry *AllocateEntry() {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(this->count < this->table_size);

                Entry *entry = this->free_head;
                this->free_head = entry->GetNextFreeEntry();

                this->count++;
                this->max_count = std::max(this->max_count, this->count);

                return entry;
            }

            constexpr ALWAYS_INLINE void FreeEntry(Entry *entry) {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(this->count > 0);

                entry->SetFree(this->free_head);
                this->free_head = entry;

                this->count--;
            }

            constexpr ALWAYS_INLINE u16 AllocateLinearId() {
                const u16 id = this->next_linear_id++;
                if (this->next_linear_id > MaxLinearId) {
                    this->next_linear_id = MinLinearId;
                }
                return id;
            }

            constexpr ALWAYS_INLINE size_t GetEntryIndex(Entry *entry) {
                const size_t index = entry - this->table;
                MESOSPHERE_ASSERT(index < this->table_size);
                return index;
            }

            constexpr ALWAYS_INLINE Entry *FindEntry(ams::svc::Handle handle) const {
                MESOSPHERE_ASSERT_THIS();

                /* Unpack the handle. */
                const auto handle_pack = GetHandleBitPack(handle);
                const auto raw_value   = handle_pack.Get<HandleRawValue>();
                const auto index       = handle_pack.Get<HandleIndex>();
                const auto linear_id   = handle_pack.Get<HandleLinearId>();
                const auto reserved    = handle_pack.Get<HandleReserved>();
                MESOSPHERE_ASSERT(reserved == 0);

                /* Validate our indexing information. */
                if (raw_value == 0) {
                    return nullptr;
                }
                if (linear_id == 0) {
                    return nullptr;
                }
                if (index >= this->table_size) {
                    return nullptr;
                }

                /* Get the entry, and ensure our serial id is correct. */
                Entry *entry = std::addressof(this->table[index]);
                if (entry->GetObject() == nullptr) {
                    return nullptr;
                }
                if (entry->GetLinearId() != linear_id) {
                    return nullptr;
                }

                return entry;
            }

            constexpr ALWAYS_INLINE KAutoObject *GetObjectImpl(ams::svc::Handle handle) const {
                MESOSPHERE_ASSERT_THIS();

                /* Handles must not have reserved bits set. */
                if (GetHandleBitPack(handle).Get<HandleReserved>() != 0) {
                    return nullptr;
                }

                if (Entry *entry = this->FindEntry(handle); entry != nullptr) {
                    return entry->GetObject();
                } else {
                    return nullptr;
                }
            }

            constexpr ALWAYS_INLINE KAutoObject *GetObjectByIndexImpl(ams::svc::Handle *out_handle, size_t index) const {
                MESOSPHERE_ASSERT_THIS();

                /* Index must be in bounds. */
                if (index >= this->table_size || this->table == nullptr) {
                    return nullptr;
                }

                /* Ensure entry has an object. */
                Entry *entry = std::addressof(this->table[index]);
                if (entry->GetObject() == nullptr) {
                    return nullptr;
                }

                *out_handle = EncodeHandle(index, entry->GetLinearId());
                return entry->GetObject();
            }
    };

}
