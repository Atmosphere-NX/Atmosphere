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

namespace ams::diag::impl {

    template<typename Holder, typename Context>
    class ObserverManager {
        NON_COPYABLE(ObserverManager);
        NON_MOVEABLE(ObserverManager);
        private:
            Holder *m_observer_list_head;
            Holder **m_observer_list_tail;
            os::ReaderWriterLock m_lock;
        public:
            constexpr ObserverManager() : m_observer_list_head(nullptr), m_observer_list_tail(std::addressof(m_observer_list_head)), m_lock() {
                /* ... */
            }

            constexpr ~ObserverManager() {
                if (std::is_constant_evaluated()) {
                    this->UnregisterAllObserverLocked();
                } else {
                    this->UnregisterAllObserver();
                }
            }

            void RegisterObserver(Holder *holder) {
                /* Acquire a write hold on our lock. */
                std::scoped_lock lk(m_lock);

                this->RegisterObserverLocked(holder);
            }

            void UnregisterObserver(Holder *holder) {
                /* Acquire a write hold on our lock. */
                std::scoped_lock lk(m_lock);

                /* Check that we can unregister. */
                AMS_ASSERT(holder->is_registered);

                /* Remove the holder. */
                if (m_observer_list_head == holder) {
                    m_observer_list_head = holder->next;
                    if (m_observer_list_tail == std::addressof(holder->next)) {
                        m_observer_list_tail = std::addressof(m_observer_list_head);
                    }
                } else {
                    for (auto *cur = m_observer_list_head; cur != nullptr; cur = cur->next) {
                        if (cur->next == holder) {
                            cur->next = holder->next;

                            if (m_observer_list_tail == std::addressof(holder->next)) {
                                m_observer_list_tail = std::addressof(cur->next);
                            }

                            break;
                        }
                    }
                }

                /* Set unregistered. */
                holder->next = nullptr;
            }

            void UnregisterAllObserver() {
                /* Acquire a write hold on our lock. */
                std::scoped_lock lk(m_lock);

                this->UnregisterAllObserverLocked();
            }

            void InvokeAllObserver(const Context &context) {
                /* Use the holder's observer. */
                InvokeAllObserver(context, [] (const Holder &holder, const Context &context) {
                    holder.observer(context);
                });
            }

            template<typename Observer>
            void InvokeAllObserver(const Context &context, Observer observer) {
                /* Acquire a read hold on our lock. */
                std::shared_lock lk(m_lock);

                /* Invoke all observers. */
                for (const auto *holder = m_observer_list_head; holder != nullptr; holder = holder->next) {
                    observer(*holder, context);
                }
            }
        protected:
            constexpr void RegisterObserverLocked(Holder *holder) {
                /* Check that we can register. */
                AMS_ASSERT(!holder->is_registered);

                /* Insert the holder. */
                *m_observer_list_tail = holder;
                m_observer_list_tail  = std::addressof(holder->next);

                /* Set registered. */
                holder->next = nullptr;
                holder->is_registered = true;
            }

            constexpr void UnregisterAllObserverLocked() {
                /* Unregister all observers. */
                for (auto *holder = m_observer_list_head; holder != nullptr; holder = holder->next) {
                    holder->is_registered = false;
                }

                /* Reset head/fail. */
                m_observer_list_head = nullptr;
                m_observer_list_tail = std::addressof(m_observer_list_head);
            }
    };

    template<typename Holder, typename Context>
    class ObserverManagerWithDefaultHolder : public ObserverManager<Holder, Context> {
        private:
            Holder m_default_holder;
        public:
            template<typename Initializer, typename... Args>
            constexpr ObserverManagerWithDefaultHolder(Initializer initializer, Args &&... args) : ObserverManager<Holder, Context>(), m_default_holder{} {
                /* Initialize the default observer. */
                initializer(std::addressof(m_default_holder), std::forward<Args>(args)...);

                /* Register the default observer. */
                if (std::is_constant_evaluated()) {
                    this->RegisterObserverLocked(std::addressof(m_default_holder));
                } else {
                    this->RegisterObserver(std::addressof(m_default_holder));
                }
            }

            Holder &GetDefaultObserverHolder() {
                return m_default_holder;
            }

            const Holder &GetDefaulObservertHolder() const {
                return m_default_holder;
            }
    };

}
