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
#include <mesosphere.hpp>

namespace ams::kern {

    Result KHandleTable::Finalize() {
        MESOSPHERE_ASSERT_THIS();

        /* Get the table and clear our record of it. */
        u16 saved_table_size = 0;
        {
            KScopedDisableDispatch dd;
            KScopedSpinLock lk(m_lock);

            std::swap(m_table_size, saved_table_size);
        }

        /* Close and free all entries. */
        for (size_t i = 0; i < saved_table_size; i++) {
            if (KAutoObject *obj = m_objects[i]; obj != nullptr) {
                obj->Close();
            }
        }

        return ResultSuccess();
    }

    bool KHandleTable::Remove(ams::svc::Handle handle) {
        MESOSPHERE_ASSERT_THIS();

        /* Don't allow removal of a pseudo-handle. */
        if (AMS_UNLIKELY(ams::svc::IsPseudoHandle(handle))) {
            return false;
        }

        /* Handles must not have reserved bits set. */
        const auto handle_pack = GetHandleBitPack(handle);
        if (AMS_UNLIKELY(handle_pack.Get<HandleReserved>() != 0)) {
            return false;
        }

        /* Find the object and free the entry. */
        KAutoObject *obj = nullptr;
        {
            KScopedDisableDispatch dd;
            KScopedSpinLock lk(m_lock);

            if (AMS_LIKELY(this->IsValidHandle(handle))) {
                const auto index = handle_pack.Get<HandleIndex>();

                obj = m_objects[index];
                this->FreeEntry(index);
            } else {
                return false;
            }
        }

        /* Close the object. */
        obj->Close();
        return true;
    }

    Result KHandleTable::Add(ams::svc::Handle *out_handle, KAutoObject *obj) {
        MESOSPHERE_ASSERT_THIS();
        KScopedDisableDispatch dd;
        KScopedSpinLock lk(m_lock);

        /* Never exceed our capacity. */
        R_UNLESS(m_count < m_table_size, svc::ResultOutOfHandles());

        /* Allocate entry, set output handle. */
        {
            const auto linear_id = this->AllocateLinearId();
            const auto index     = this->AllocateEntry();

            m_entry_infos[index].linear_id = linear_id;
            m_objects[index]               = obj;

            obj->Open();

            *out_handle = EncodeHandle(index, linear_id);
        }

        return ResultSuccess();
    }

    Result KHandleTable::Reserve(ams::svc::Handle *out_handle) {
        MESOSPHERE_ASSERT_THIS();
        KScopedDisableDispatch dd;
        KScopedSpinLock lk(m_lock);

        /* Never exceed our capacity. */
        R_UNLESS(m_count < m_table_size, svc::ResultOutOfHandles());

        *out_handle = EncodeHandle(this->AllocateEntry(), this->AllocateLinearId());
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
        MESOSPHERE_UNUSED(linear_id, reserved);

        if (AMS_LIKELY(index < m_table_size)) {
            /* NOTE: This code does not check the linear id. */
            MESOSPHERE_ASSERT(m_objects[index] == nullptr);
            this->FreeEntry(index);
        }
    }

    void KHandleTable::Register(ams::svc::Handle handle, KAutoObject *obj) {
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
        MESOSPHERE_UNUSED(reserved);

        if (AMS_LIKELY(index < m_table_size)) {
            /* Set the entry. */
            MESOSPHERE_ASSERT(m_objects[index] == nullptr);

            m_entry_infos[index].linear_id = linear_id;
            m_objects[index]               = obj;

            obj->Open();
        }
    }

}
