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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif

namespace ams::prfile2 {

    #if defined(AMS_PRFILE2_THREAD_SAFE)

        struct CriticalSection::Resource {
            os::MutexType mutex;
            bool in_use;
        };

        namespace {

            constexpr inline const auto NumCriticalSectionResources = MaxVolumes + 1;

            constinit os::SdkMutex g_crit_resource_mutex;
            constinit CriticalSection::Resource g_crit_resources[NumCriticalSectionResources];

            CriticalSection::Resource *AllocateCriticalSectionResource() {
                std::scoped_lock lk(g_crit_resource_mutex);

                for (auto &resource : g_crit_resources) {
                    if (!resource.in_use) {
                        resource.in_use = true;
                        return std::addressof(resource);
                    }
                }

                AMS_ABORT("Failed to allocate critical section resource");
            }

            void AcquireCriticalSectionResource(CriticalSection::Resource *resource) {
                return os::LockMutex(std::addressof(resource->mutex));
            }

            void ReleaseCriticalSectionResource(CriticalSection::Resource *resource) {
                return os::UnlockMutex(std::addressof(resource->mutex));
            }

        }

        void InitializeCriticalSection(CriticalSection *cs) {
            /* Check pre-condition. */
            AMS_ASSERT(cs->state == CriticalSection::State_NotInitialized);

            /* Perform initialization. */
            if (cs->state == CriticalSection::State_NotInitialized) {
                cs->resource = AllocateCriticalSectionResource();
                cs->owner    = 0;
                cs->state    = CriticalSection::State_Initialized;
            }
        }

        void FinalizeCriticalSection(CriticalSection *cs) {
            /* Check pre-condition. */
            AMS_ASSERT(cs->state == CriticalSection::State_Initialized);

            /* TODO */
            AMS_UNUSED(cs);
            AMS_ABORT("prfile2::FinalizeCriticalSection");
        }

        void EnterCriticalSection(CriticalSection *cs) {
            /* Check pre-condition. */
            AMS_ASSERT(cs->state == CriticalSection::State_Initialized);

            if (AMS_LIKELY(cs->lock_count == 0)) {
                /* Acquire the lock .*/
                AcquireCriticalSectionResource(cs->resource);
                system::GetCurrentContextId(std::addressof(cs->owner));
            } else {
                /* Get the current context id. */
                u64 current_context_id;
                system::GetCurrentContextId(std::addressof(current_context_id));

                /* If the lock isn't already held, acquire it. */
                if (cs->owner != current_context_id) {
                    AcquireCriticalSectionResource(cs->resource);
                    cs->owner = current_context_id;
                }
            }

            /* Increment the lock count. */
            ++cs->lock_count;
        }

        void ExitCriticalSection(CriticalSection *cs) {
            /* Check pre-condition. */
            AMS_ASSERT(cs->state == CriticalSection::State_Initialized);

            /* Unlock. */
            if ((--cs->lock_count) == 0) {
                cs->owner = 0;
                ReleaseCriticalSectionResource(cs->resource);
            }
        }

    #else

        void InitializeCriticalSection(CriticalSection *cs) {
            AMS_UNUSED(cs);
        }

        void FinalizeCriticalSection(CriticalSection *cs) {
            AMS_UNUSED(cs);
        }

        void EnterCriticalSection(CriticalSection *cs) {
            AMS_UNUSED(cs);
        }

        void ExitCriticalSection(CriticalSection *cs) {
            AMS_UNUSED(cs);
        }

    #endif

}
