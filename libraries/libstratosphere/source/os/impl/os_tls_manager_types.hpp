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
#include <stratosphere.hpp>

namespace ams::os::impl {

    static constexpr size_t TotalTlsSlotCountMax = TlsSlotCountMax + SdkTlsSlotCountMax;

    class TlsManager {
        NON_COPYABLE(TlsManager);
        NON_MOVEABLE(TlsManager);
        private:
            TlsDestructor m_destructors[TotalTlsSlotCountMax];
            InternalCriticalSection m_cs;
            int m_used;
        public:
            consteval TlsManager() : m_destructors(), m_cs(), m_used() { /* ... */ }

            void InvokeTlsDestructors();
            bool AllocateTlsSlot(TlsSlot *out, TlsDestructor destructor, bool sdk);
            void FreeTlsSlot(TlsSlot slot);

            int GetUsedTlsSlots() const { return m_used; }

            static consteval int GetMaxTlsSlots() { return TotalTlsSlotCountMax; }
        private:
            int SearchUnusedTlsSlotUnsafe(bool sdk);
    };

}