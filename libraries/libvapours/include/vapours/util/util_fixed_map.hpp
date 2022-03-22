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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/util_fixed_tree.hpp>

namespace ams::util {

    template<typename Key, typename Value, typename Compare = std::less<Key>, size_t BufferAlignment = 8>
    class FixedMap {
        private:
            using KeyValuePair = std::pair<Key const, Value>;

            struct LessTypeForMap {
                constexpr ALWAYS_INLINE bool operator()(const KeyValuePair &lhs, const KeyValuePair &rhs) const {
                    return Compare{}(lhs.first, rhs.first);
                }
            };

            using TreeType = ::ams::util::FixedTree<KeyValuePair, LessTypeForMap, KeyValuePair, BufferAlignment>;

            using iterator       = typename TreeType::iterator;
            using const_iterator = typename TreeType::const_iterator;
        public:
            static constexpr size_t GetRequiredMemorySize(size_t num_elements) {
                return TreeType::GetRequiredMemorySize(num_elements);
            }
        private:
            TreeType m_tree;
        public:
            FixedMap() : m_tree() { /* ... */ }

            void Initialize(size_t num_elements, void *buffer, size_t buffer_size) {
                return m_tree.Initialize(num_elements, buffer, buffer_size);
            }

            ALWAYS_INLINE iterator begin() { return m_tree.begin(); }
            ALWAYS_INLINE const_iterator begin() const { return m_tree.begin(); }

            ALWAYS_INLINE iterator end() { return m_tree.end(); }
            ALWAYS_INLINE const_iterator end() const { return m_tree.end(); }

            ALWAYS_INLINE bool erase(const Key &key) { const KeyValuePair pair(key, Value{}); return m_tree.erase(pair); }

            ALWAYS_INLINE iterator find(const Key &key) { const KeyValuePair pair(key, Value{}); return m_tree.find(pair); }
            ALWAYS_INLINE const_iterator find(const Key &key) const { const KeyValuePair pair(key, Value{}); return m_tree.find(pair); }

            ALWAYS_INLINE std::pair<iterator, bool> insert(const KeyValuePair &pair) { return m_tree.insert(pair); }

            ALWAYS_INLINE size_t size() const { return m_tree.size(); }

            ALWAYS_INLINE void clear() { return m_tree.clear(); }
    };

}
