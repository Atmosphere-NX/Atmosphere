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
#include <stratosphere/psc/psc_types.hpp>
#include <stratosphere/psc/psc_pm_module_id.hpp>
#include <stratosphere/psc/sf/psc_sf_i_pm_module.hpp>

namespace ams::psc {

    class PmModule {
        NON_COPYABLE(PmModule);
        NON_MOVEABLE(PmModule);
        private:
            os::SystemEvent m_system_event;
            bool m_initialized;
            PmModuleId m_module_id;
            uintptr_t m_reserved;
            PmState m_state;
            PmFlagSet m_flags;
        public:
            PmModule() : m_initialized(false), m_module_id(PmModuleId_Reserved0), m_reserved(0), m_state(PmState_FullAwake), m_flags() { /* ... */ }
            ~PmModule() {
                if (m_initialized) {
                    R_ABORT_UNLESS(this->Finalize());
                }
            }

            Result Initialize(const PmModuleId mid, const PmModuleId *dependencies, u32 dependency_count, os::EventClearMode clear_mode) {
                /* TODO: Should we do in-process dependency resolution? */
                AMS_UNUSED(dependencies, dependency_count);

                /* Check that we're not already initialized. */
                R_UNLESS(!m_initialized, psc::ResultAlreadyInitialized());

                /* Create our event. */
                R_ABORT_UNLESS(os::CreateSystemEvent(m_system_event.GetBase(), clear_mode, false));

                /* Set our state. */
                m_module_id = mid;
                m_initialized = true;

                R_SUCCEED();
            }

            Result Finalize() {
                /* Check that we're initialized. */
                R_UNLESS(m_initialized, psc::ResultNotInitialized());

                /* Destroy our system event. */
                os::DestroySystemEvent(m_system_event.GetBase());

                /* Mark not initialized. */
                m_initialized = false;

                R_SUCCEED();
            }

            constexpr PmModuleId GetId() const { return m_module_id; }

            Result GetRequest(PmState *out_state, PmFlagSet *out_flags) {
                /* Check that we're initialized. */
                R_UNLESS(m_initialized, psc::ResultNotInitialized());

                /* Set output. */
                *out_state = m_state;
                *out_flags = m_flags;

                R_SUCCEED();
            }

            Result Acknowledge(PmState state, Result res) {
                /* Check that we're initialized. */
                R_UNLESS(m_initialized, psc::ResultNotInitialized());

                /* Check the transition was successful. */
                R_ABORT_UNLESS(res);

                AMS_UNUSED(state);
                R_SUCCEED();
            }

            os::SystemEvent *GetEventPointer() {
                return std::addressof(m_system_event);
            }
    };

}