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
#include <stratosphere.hpp>
#include "os_tls_manager.hpp"
#include "os_thread_manager.hpp"

namespace ams::os::impl {

    /* TODO: Should we migrate away from libnx to this on NX at some point? */
    #if !defined(ATMOSPHERE_OS_HORIZON)
    namespace {

        constexpr auto TryCallDestructorCount = 4;

        void DefaultTlsDestructor(uintptr_t) {
            /* ... */
        }

    }

    void TlsManager::InvokeTlsDestructors() {
        /* Get the curent thread. */
        auto * const thread = impl::GetCurrentThread();

        /* Call all destructors. */
        for (int slot = 0; slot < static_cast<int>(TotalTlsSlotCountMax); ++slot) {
            if (const auto destructor = m_destructors[slot]; destructor != nullptr) {
                destructor(thread->tls_value_array[slot]);
            }

            thread->tls_value_array[slot] = 0;
        }

        /* Verify all tls values are wiped, trying up to four times. */
        for (auto i = 0; i < TryCallDestructorCount; ++i) {
            bool all_cleared = true;
            for (auto slot = 0; slot < static_cast<int>(TotalTlsSlotCountMax); ++slot) {
                if (thread->tls_value_array[slot] != 0) {
                    all_cleared = false;

                    if (const auto destructor = m_destructors[slot]; destructor != nullptr) {
                        destructor(thread->tls_value_array[slot]);
                    }

                    thread->tls_value_array[slot] = 0;
                }
            }

            if (all_cleared) {
                break;
            }
        }
    }

    bool TlsManager::AllocateTlsSlot(TlsSlot *out, TlsDestructor destructor, bool sdk) {
        /* Decide on a slot. */
        int slot;
        {
            /* Lock appropriately. */
            std::scoped_lock lk(util::GetReference(impl::GetCurrentThread()->cs_thread));
            std::scoped_lock lk2(m_cs);

            /* If we're out of tls slots, fail. */
            if (!sdk && m_used >= static_cast<int>(TlsSlotCountMax)) {
                return false;
            }

            /* Get a slot. */
            slot = SearchUnusedTlsSlotUnsafe(sdk);

            /* Set the destructor. */
            m_destructors[slot] = (destructor != nullptr) ? destructor : DefaultTlsDestructor;

            /* Increment our used count, if the slot was a user slot. */
            if (!sdk) {
                ++m_used;
            }
        }

        /* Zero the slot in all threads. */
        GetThreadManager().SetZeroToAllThreadsTlsSafe(slot);

        /* Set the output slot's value. */
        out->_value = slot;
        return true;
    }

    void TlsManager::FreeTlsSlot(TlsSlot slot) {
        /* Get the slot's index. */
        const auto slot_idx = slot._value;

        /* Lock appropriately. */
        std::scoped_lock lk(util::GetReference(impl::GetCurrentThread()->cs_thread));
        std::scoped_lock lk2(m_cs);

        /* Verify the slot is allocated. */
        AMS_ABORT_UNLESS(m_destructors[slot_idx] != nullptr);

        /* Free the slot. */
        m_destructors[slot_idx] = nullptr;

        /* Decrement our used count, if the slot was a user slot. */
        if (slot_idx < TlsSlotCountMax) {
            --m_used;
        }
    }

    int TlsManager::SearchUnusedTlsSlotUnsafe(bool sdk) {
        if (sdk) {
            /* Search backwards for an unused slot. */
            for (int slot = static_cast<int>(TotalTlsSlotCountMax) - 1; slot >= static_cast<int>(TlsSlotCountMax + SdkInternalTlsCount); --slot) {
                if (m_destructors[slot] == nullptr) {
                    return slot;
                }
            }
            AMS_ABORT("Failed to allocate SdkTlsSlot");
        } else {
            /* Search forwards for an unused slot. */
            for (int slot = 0; slot < static_cast<int>(TlsSlotCountMax); ++slot) {
                if (m_destructors[slot] == nullptr) {
                    return slot;
                }
            }
            AMS_ABORT("Failed to allocate TlsSlot");
        }
    }
    #endif

}
