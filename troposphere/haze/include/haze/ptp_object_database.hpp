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

#include <haze/common.hpp>
#include <haze/ptp_object_heap.hpp>
#include <vapours/util.hpp>

namespace haze {

    class PtpObjectDatabase {
        public:
            struct ObjectNode {
                util::IntrusiveRedBlackTreeNode m_object_name_to_id_node;
                util::IntrusiveRedBlackTreeNode m_object_id_to_name_node;
                u32 m_parent_id;
                u32 m_object_id;
                char m_name[];

                explicit ObjectNode(char *name, u32 parent_id, u32 object_id) : m_object_name_to_id_node(), m_object_id_to_name_node(), m_parent_id(parent_id), m_object_id(object_id) { /* ... */ }

                const char *GetName() const { return m_name; }
                u32 GetParentId()     const { return m_parent_id; }
                u32 GetObjectId()     const { return m_object_id; }

                struct NameComparator {
                    struct RedBlackKeyType {
                        const char *m_name;

                        constexpr RedBlackKeyType(const char *name) : m_name(name) { /* ... */ }

                        constexpr const char *GetName() const {
                            return m_name;
                        }
                    };

                    template<typename T> requires (std::same_as<T, ObjectNode> || std::same_as<T, RedBlackKeyType>)
                    static constexpr int Compare(const T &lhs, const ObjectNode &rhs) {
                        return std::strcmp(lhs.GetName(), rhs.GetName());
                    }
                };

                struct IdComparator {
                    struct RedBlackKeyType {
                        u32 m_object_id;

                        constexpr RedBlackKeyType(u32 object_id) : m_object_id(object_id) { /* ... */ }

                        constexpr u32 GetObjectId() const {
                            return m_object_id;
                        }
                    };

                    template<typename T> requires (std::same_as<T, ObjectNode> || std::same_as<T, RedBlackKeyType>)
                    static constexpr int Compare(const T &lhs, const ObjectNode &rhs) {
                        return lhs.GetObjectId() - rhs.GetObjectId();
                    }
                };
            };

        private:
            using ObjectNameToIdTreeTraits = util::IntrusiveRedBlackTreeMemberTraitsDeferredAssert<&ObjectNode::m_object_name_to_id_node>;
            using ObjectNameToIdTree       = ObjectNameToIdTreeTraits::TreeType<ObjectNode::NameComparator>;

            using ObjectIdToNameTreeTraits = util::IntrusiveRedBlackTreeMemberTraitsDeferredAssert<&ObjectNode::m_object_id_to_name_node>;
            using ObjectIdToNameTree       = ObjectIdToNameTreeTraits::TreeType<ObjectNode::IdComparator>;

            PtpObjectHeap *m_object_heap;
            ObjectNameToIdTree m_name_to_object_id;
            ObjectIdToNameTree m_object_id_to_name;
            u32 m_next_object_id;

        public:
            constexpr explicit PtpObjectDatabase() : m_object_heap(), m_name_to_object_id(), m_object_id_to_name(), m_next_object_id() { /* ... */ }

            void Initialize(PtpObjectHeap *object_heap);
            void Finalize();

        public:
            /* Object database API. */
            Result AddObjectId(const char *parent_name, const char *name, u32 *out_object_id, u32 parent_id, u32 desired_object_id = 0);
            void RemoveObjectId(u32 object_id);
            ObjectNode *GetObject(u32 object_id);
    };

}
