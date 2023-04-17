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

    struct PtpObject {
        public:
            util::IntrusiveRedBlackTreeNode m_name_node;
            util::IntrusiveRedBlackTreeNode m_object_id_node;
            u32 m_parent_id;
            u32 m_object_id;
            char m_name[];
        public:
            const char *GetName()  const { return m_name; }
            u32 GetParentId()      const { return m_parent_id; }
            u32 GetObjectId()      const { return m_object_id; }
        public:
            bool GetIsRegistered() const { return m_object_id != 0; }
            void Register(u32 object_id) { m_object_id = object_id; }
            void Unregister()            { m_object_id = 0; }
        public:
            struct NameComparator {
                struct RedBlackKeyType {
                    const char *m_name;

                    constexpr RedBlackKeyType(const char *name) : m_name(name) { /* ... */ }

                    constexpr const char *GetName() const {
                        return m_name;
                    }
                };

                template<typename T> requires (std::same_as<T, PtpObject> || std::same_as<T, RedBlackKeyType>)
                static constexpr int Compare(const T &lhs, const PtpObject &rhs) {
                    /* All SD card filesystems supported by fs are case-insensitive and case-preserving. */
                    /* Account for that in collation here. */
                    return strcasecmp(lhs.GetName(), rhs.GetName());
                }
            };

            struct ObjectIdComparator {
                struct RedBlackKeyType {
                    u32 m_object_id;

                    constexpr RedBlackKeyType(u32 object_id) : m_object_id(object_id) { /* ... */ }

                    constexpr u32 GetObjectId() const {
                        return m_object_id;
                    }
                };

                template<typename T> requires (std::same_as<T, PtpObject> || std::same_as<T, RedBlackKeyType>)
                static constexpr int Compare(const T &lhs, const PtpObject &rhs) {
                    return lhs.GetObjectId() - rhs.GetObjectId();
                }
            };
    };

    class PtpObjectDatabase {
        private:
            using ObjectNameTreeTraits = util::IntrusiveRedBlackTreeMemberTraitsDeferredAssert<&PtpObject::m_name_node>;
            using ObjectNameTree       = ObjectNameTreeTraits::TreeType<PtpObject::NameComparator>;

            using ObjectIdTreeTraits   = util::IntrusiveRedBlackTreeMemberTraitsDeferredAssert<&PtpObject::m_object_id_node>;
            using ObjectIdTree         = ObjectIdTreeTraits::TreeType<PtpObject::ObjectIdComparator>;

            PtpObjectHeap *m_object_heap;
            ObjectNameTree m_name_tree;
            ObjectIdTree m_object_id_tree;
            u32 m_next_object_id;
        public:
            constexpr explicit PtpObjectDatabase() : m_object_heap(), m_name_tree(), m_object_id_tree(), m_next_object_id() { /* ... */ }

            void Initialize(PtpObjectHeap *object_heap);
            void Finalize();
        public:
            /* Object database API. */
            Result CreateOrFindObject(const char *parent_name, const char *name, u32 parent_id, PtpObject **out_object);
            void RegisterObject(PtpObject *object, u32 desired_id = 0);
            void UnregisterObject(PtpObject *object);
            void DeleteObject(PtpObject *obj);

            Result CreateAndRegisterObjectId(const char *parent_name, const char *name, u32 parent_id, u32 *out_object_id);
        public:
            PtpObject *GetObjectById(u32 object_id);
            PtpObject *GetObjectByName(const char *name);
    };

}
