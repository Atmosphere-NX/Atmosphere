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
#include <stratosphere/sf/sf_shared_object.hpp>

namespace ams::ncm {

    template<typename T, size_t N>
    class IntegratedList {
        static_assert(N <= std::numeric_limits<u8>::max());
        public:
            struct ListData {
                sf::SharedPointer<T> interface;
                u8 id;
            };
        private:
            size_t m_count;
            sf::SharedPointer<T> m_interfaces[N];
            u8 m_ids[N];
        public:
            IntegratedList() : m_count(0), m_interfaces(), m_ids() { /* ... */ }

            void Add(ListData &data) {
                /* Find place to insert into the list. */
                const size_t pos = std::distance(std::begin(m_ids), std::lower_bound(std::begin(m_ids), std::end(m_ids), data.id));

                /* If we need to, move stuff to make space. */
                if (m_ids[pos] > data.id) {
                    AMS_ABORT_UNLESS(m_count < N);

                    for (size_t i = m_count; i > pos; --i) {
                        m_ids[i]        = std::move(m_ids[i - 1]);
                        m_interfaces[i] = std::move(m_interfaces[i - 1]);
                    }

                    /* If we're inserting somewhere in the middle, increment count. */
                    m_count++;
                } else if (m_ids[pos] < data.id) {
                    /* If we're inserting at the end, increment count. */
                    AMS_ABORT_UNLESS(m_count < N);
                    m_count++;
                }

                /* Set at position. */
                m_interfaces[pos] = data.interface;
                m_ids[pos]        = data.id;
            }

            ListData Get(size_t idx) {
                AMS_ABORT_UNLESS(idx < m_count);

                return { m_interfaces[idx], m_ids[idx] };
            }

            Result TryEach(auto callback) {
                Result result = ResultSuccess();
                for (size_t i = 0; i < m_count; ++i) {
                    result = callback(this->Get(i));
                    if (R_SUCCEEDED(result)) {
                        break;
                    }
                }

                R_RETURN(result);
            }

            Result ForAll(auto callback) {
                for (size_t i = 0; i < m_count; ++i) {
                    R_TRY(callback(this->Get(i)));
                }
                R_SUCCEED();
            }

            size_t GetCount() const { return m_count; }
    };

}
