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
#include <stratosphere.hpp>
#include "sm_wait_list.hpp"

namespace ams::sm::impl {

    namespace {

        static constexpr size_t DeferredSessionCountMax = 0x40;

        struct WaitListEntry {
            sm::ServiceName service;
            os::WaitableHolderType *session;
        };

        constinit WaitListEntry g_entries[DeferredSessionCountMax];
        constinit WaitListEntry *g_processing_entry = nullptr;
        constinit ServiceName g_triggered_service = InvalidServiceName;

        WaitListEntry *Find(ServiceName service) {
            for (auto &entry : g_entries) {
                if (entry.service == service) {
                    return std::addressof(entry);
                }
            }

            return nullptr;
        }

    }

    Result StartRegisterRetry(ServiceName service) {
        /* Check that we're not already processing a retry. */
        AMS_ABORT_UNLESS(g_processing_entry == nullptr);

        /* Find a free entry. */
        auto *entry = Find(InvalidServiceName);
        R_UNLESS(entry != nullptr, sm::ResultOutOfProcesses());

        /* Initialize the entry. */
        entry->service = service;
        entry->session = nullptr;

        /* Set the processing entry. */
        g_processing_entry = entry;

        return sf::ResultRequestDeferredByUser();
    }

    void ProcessRegisterRetry(os::WaitableHolderType *session_holder) {
        /* Verify that we have a processing entry. */
        AMS_ABORT_UNLESS(g_processing_entry != nullptr);

        /* Process the session. */
        g_processing_entry->session = session_holder;
        g_processing_entry = nullptr;
    }

    void TriggerResume(ServiceName service) {
        /* Check that we haven't already triggered a resume. */
        AMS_ABORT_UNLESS(g_triggered_service == InvalidServiceName);

        /* Set the triggered resume. */
        g_triggered_service = service;
    }

    void TestAndResume(ResumeFunction resume_function) {
        /* If we don't have a triggered service, there's nothing to do. */
        if (g_triggered_service == InvalidServiceName) {
            return;
        }

        /* Get and clear the triggered service. */
        const auto resumed_service = g_triggered_service;
        g_triggered_service = InvalidServiceName;

        /* Process all entries. */
        for (size_t i = 0; i < util::size(g_entries); /* ... */) {
            auto &entry = g_entries[i];
            if (entry.service == resumed_service) {
                /* Get the entry's session. */
                auto * const session = entry.session;

                /* Clear the entry. */
                entry.service = InvalidServiceName;
                entry.session = nullptr;

                /* Resume the request. */
                R_TRY_CATCH(resume_function(session)) {
                    R_CATCH(sf::ResultRequestDeferred) {
                        ProcessRegisterRetry(session);
                    }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;

                /* Handle nested resumes. */
                if (g_triggered_service != InvalidServiceName) {
                    AMS_ABORT_UNLESS(g_triggered_service == resumed_service);

                    g_triggered_service = InvalidServiceName;
                    i = 0;
                    continue;
                }
            }

            /* Advance. */
            ++i;
        }
    }

}
