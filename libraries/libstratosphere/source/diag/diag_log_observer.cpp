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
#include <stratosphere/diag/diag_log_observer.hpp>

namespace ams::diag {

    namespace impl {

        extern LogObserverHolder g_default_log_observer;
        extern LogObserverHolder *g_log_observer_list_head;

        void DefaultLogObserver(const LogMetaData &log_metadata, const LogBody &log_body, void *user_data);

    }
    
    void InitializeLogObserverHolder(LogObserverHolder *observer_holder, LogFunction log_function, void *user_data) {
        observer_holder->log_function = log_function;
        observer_holder->next = nullptr;
        observer_holder->is_registered = false;
        observer_holder->user_data = user_data;
    }

    void RegisterLogObserver(LogObserverHolder *observer_holder) {
        AMS_ASSERT(!observer_holder->is_registered);

        if (impl::g_log_observer_list_head) {
            /* Find the last registered observer and append this one. */
            auto observer = impl::g_log_observer_list_head;
            LogObserverHolder *observer_parent = nullptr;
            do {
                observer_parent = observer;
                observer = observer->next;
            } while(observer);
            observer_parent->next = observer_holder;
        }
        else {
            /* There are no observers yet, this will be the first one. */
            impl::g_log_observer_list_head = observer_holder;
        }
        observer_holder->is_registered = true;
        observer_holder->next = nullptr;
    }

    void UnregisterLogObserver(LogObserverHolder *observer_holder) {
        AMS_ASSERT(observer_holder->is_registered);

        if (observer_holder == impl::g_log_observer_list_head) {
            /* Move the next item to be the list head. */
            impl::g_log_observer_list_head = observer_holder->next;
        }
        else {
            /* Find this observer's parent observer and move the following observer, removing the observer. */
            auto observer = impl::g_log_observer_list_head;
            while (observer) {
                auto observer_parent = observer;
                observer = observer->next;
                if (observer_holder == observer) {
                    observer_parent->next = observer_holder->next;
                    break;
                }
            }
        }
        observer_holder->is_registered = false;
    }

    void ResetDefaultLogObserver() {
        ReplaceDefaultLogObserver(impl::DefaultLogObserver);
    }

    void ReplaceDefaultLogObserver(LogFunction log_function) {
        UnregisterLogObserver(std::addressof(impl::g_default_log_observer));
        InitializeLogObserverHolder(std::addressof(impl::g_default_log_observer), log_function, nullptr);
        RegisterLogObserver(std::addressof(impl::g_default_log_observer));
    }

}