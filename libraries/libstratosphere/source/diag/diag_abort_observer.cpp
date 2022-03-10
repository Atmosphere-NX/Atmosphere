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
#include "impl/diag_abort_observer_manager.hpp"

namespace ams::diag {

    namespace impl {

        constinit bool g_enable_default_abort_observer = true;

    }

    namespace {

        template<typename Holder, typename Observer>
        void InitializeAbortObserverHolderImpl(Holder *holder, Observer observer) {
            holder->observer      = observer;
            holder->next          = nullptr;
            holder->is_registered = false;
        }

    }

    void InitializeAbortObserverHolder(AbortObserverHolder *holder, AbortObserver observer) {
        InitializeAbortObserverHolderImpl(holder, observer);
    }

    void RegisterAbortObserver(AbortObserverHolder *holder) {
        impl::GetAbortObserverManager()->RegisterObserver(holder);
    }

    void UnregisterAbortObserver(AbortObserverHolder *holder) {
        impl::GetAbortObserverManager()->UnregisterObserver(holder);
    }

    void EnableDefaultAbortObserver(bool en) {
        ::ams::diag::impl::g_enable_default_abort_observer = en;
    }

    void InitializeSdkAbortObserverHolder(SdkAbortObserverHolder *holder, SdkAbortObserver observer) {
        InitializeAbortObserverHolderImpl(holder, observer);
    }

    void RegisterSdkAbortObserver(SdkAbortObserverHolder *holder) {
        impl::GetSdkAbortObserverManager()->RegisterObserver(holder);
    }

    void UnregisterSdkAbortObserver(SdkAbortObserverHolder *holder) {
        impl::GetSdkAbortObserverManager()->UnregisterObserver(holder);
    }

}
