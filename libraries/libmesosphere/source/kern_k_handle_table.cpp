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
#include <mesosphere.hpp>

namespace ams::kern {

    Result KHandleTable::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Get the table and clear our record of it. */
        Entry *saved_table = nullptr;
        u16 saved_table_size = 0;
        {
            KScopedDisableDispatch dd;
            KScopedSpinLock lk(m_lock);

            std::swap(m_table, saved_table);
            std::swap(m_table_size, saved_table_size);
        }

        /* Close and free all entries. */
        for (size_t i = 0; i < saved_table_size; i++) {
            Entry *entry = std::addressof(saved_table[i]);

            if (KAutoObject *obj = entry->GetObject(); obj != nullptr) {
                obj->Close();
                this->FreeEntry(entry);
            }
        }

        return ResultSuccess();
    }

    bool KHandleTable::Remove(ams::svc::Handle handle) {
        MESOSPHERE_ASSERT_THIS();

        /* Don't allow removal of a pseudo-handle. */
        if (ams::svc::IsPseudoHandle(handle)) {
            return false;
        }

        /* Handles must not have reserved bits set. */
        if (GetHandleBitPack(handle).Get<HandleReserved>() != 0) {
            return false;
        }

        /* Find the object and free the entry. */
        KAutoObject *obj = nullptr;
        {
            KScopedDisableDispatch dd;
            KScopedSpinLock lk(m_lock);

            if (Entry *entry = this->FindEntry(handle); entry != nullptr) {
                obj = entry->GetObject();
                this->FreeEntry(entry);
            } else {
                return false;
            }
        }

        /* Close the object. */
        obj->Close();
        return true;
    }

    Result KHandleTable::Add(ams::svc::Handle *out_handle, KAutoObject *obj, u16 type) {
        MESOSPHERE_ASSERT_THIS();
        KScopedDisableDispatch dd;
        KScopedSpinLock lk(m_lock);

        /* Never exceed our capacity. */
        R_UNLESS(m_count < m_table_size, svc::ResultOutOfHandles());

        /* Allocate entry, set output handle. */
        {
            const auto linear_id = this->AllocateLinearId();
            Entry *entry = this->AllocateEntry();
            entry->SetUsed(obj, linear_id, type);
            obj->Open();
            *out_handle = EncodeHandle(this->GetEntryIndex(entry), linear_id);
        }

        return ResultSuccess();
    }

    Result KHandleTable::Reserve(ams::svc::Handle *out_handle) {
        MESOSPHERE_ASSERT_THIS();
        KScopedDisableDispatch dd;
        KScopedSpinLock lk(m_lock);

        /* Never exceed our capacity. */
        R_UNLESS(m_count < m_table_size, svc::ResultOutOfHandles());

        *out_handle = EncodeHandle(this->GetEntryIndex(this->AllocateEntry()), this->AllocateLinearId());
        return ResultSuccess();
    }

    void KHandleTable::Unreserve(ams::svc::Handle handle) {
        MESOSPHERE_ASSERT_THIS();
        KScopedDisableDispatch dd;
        KScopedSpinLock lk(m_lock);

        /* Unpack the handle. */
        const auto handle_pack = GetHandleBitPack(handle);
        const auto index       = handle_pack.Get<HandleIndex>();
        const auto linear_id   = handle_pack.Get<HandleLinearId>();
        const auto reserved    = handle_pack.Get<HandleReserved>();
        MESOSPHERE_ASSERT(reserved == 0);
        MESOSPHERE_ASSERT(linear_id != 0);
        MESOSPHERE_ASSERT(index < m_table_size);
        MESOSPHERE_UNUSED(linear_id, reserved);

        /* Free the entry. */
        /* NOTE: This code does not check the linear id. */
        Entry *entry = std::addressof(m_table[index]);
        MESOSPHERE_ASSERT(entry->GetObject() == nullptr);

        this->FreeEntry(entry);
    }

    void KHandleTable::Register(ams::svc::Handle handle, KAutoObject *obj, u16 type) {
        MESOSPHERE_ASSERT_THIS();
        KScopedDisableDispatch dd;
        KScopedSpinLock lk(m_lock);

        /* Unpack the handle. */
        const auto handle_pack = GetHandleBitPack(handle);
        const auto index       = handle_pack.Get<HandleIndex>();
        const auto linear_id   = handle_pack.Get<HandleLinearId>();
        const auto reserved    = handle_pack.Get<HandleReserved>();
        MESOSPHERE_ASSERT(reserved == 0);
        MESOSPHERE_ASSERT(linear_id != 0);
        MESOSPHERE_ASSERT(index < m_table_size);
        MESOSPHERE_UNUSED(reserved);

        /* Set the entry. */
        Entry *entry = std::addressof(m_table[index]);
        MESOSPHERE_ASSERT(entry->GetObject() == nullptr);

        entry->SetUsed(obj, linear_id, type);
        obj->Open();
    }

}
