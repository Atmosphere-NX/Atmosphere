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
#pragma once
#include <stratosphere.hpp>

namespace ams::fssystem {

    template<typename Key, typename Value>
    class LruListCache {
        NON_COPYABLE(LruListCache);
        NON_MOVEABLE(LruListCache);
        public:
            class Node : public ::ams::fs::impl::Newable {
                NON_COPYABLE(Node);
                NON_MOVEABLE(Node);
                public:
                    Key key;
                    Value value;
                    util::IntrusiveListNode mru_list_node;
                public:
                    explicit Node(const Value &value) : value(value) { /* ... */ }
            };
        private:
            using MruList = typename util::IntrusiveListMemberTraits<&Node::mru_list_node>::ListType;
        private:
            MruList mru_list;
        public:
            constexpr LruListCache() : mru_list() { /* ... */ }

            bool FindValueAndUpdateMru(Value *out, const Key &key) {
                for (auto it = this->mru_list.begin(); it != this->mru_list.end(); ++it) {
                    if (it->key == key) {
                        *out = it->value;

                        this->mru_list.erase(it);
                        this->mru_list.push_front(*it);

                        return true;
                    }
                }

                return false;
            }

            std::unique_ptr<Node> PopLruNode() {
                AMS_ABORT_UNLESS(!this->mru_list.empty());
                Node *lru = std::addressof(*this->mru_list.rbegin());
                this->mru_list.pop_back();

                return std::unique_ptr<Node>(lru);
            }

            void PushMruNode(std::unique_ptr<Node> &&node, const Key &key) {
                node->key = key;
                this->mru_list.push_front(*node);
                node.release();
            }

            void DeleteAllNodes() {
                while (!this->mru_list.empty()) {
                    Node *lru = std::addressof(*this->mru_list.rbegin());
                    this->mru_list.erase(this->mru_list.iterator_to(*lru));
                    delete lru;
                }
            }

            size_t GetSize() const {
                return this->mru_list.size();
            }

            bool IsEmpty() const {
                return this->mru_list.empty();
            }
    };

}
