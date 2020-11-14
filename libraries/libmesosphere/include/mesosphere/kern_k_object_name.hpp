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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_auto_object.hpp>
#include <mesosphere/kern_k_light_lock.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    class KObjectName : public KSlabAllocated<KObjectName>, public util::IntrusiveListBaseNode<KObjectName> {
        public:
            static constexpr size_t NameLengthMax = 12;

            using List = util::IntrusiveListBaseTraits<KObjectName>::ListType;
        private:
            char name[NameLengthMax];
            KAutoObject *object;
        public:
            constexpr KObjectName() : name(), object() { /* ... */ }
        public:
            static Result NewFromName(KAutoObject *obj, const char *name);
            static Result Delete(KAutoObject *obj, const char *name);

            static KScopedAutoObject<KAutoObject> Find(const char *name);

            template<typename Derived>
            static Result Delete(const char *name) {
                /* Find the object. */
                KScopedAutoObject obj = Find(name);
                R_UNLESS(obj.IsNotNull(), svc::ResultNotFound());

                /* Cast the object to the desired type. */
                Derived *derived = obj->DynamicCast<Derived *>();
                R_UNLESS(derived != nullptr, svc::ResultNotFound());

                return Delete(obj.GetPointerUnsafe(), name);
            }

            template<typename Derived> requires std::derived_from<Derived, KAutoObject>
            static KScopedAutoObject<Derived> Find(const char *name) {
                return Find(name);
            }
        private:
            static KScopedAutoObject<KAutoObject> FindImpl(const char *name);

            void Initialize(KAutoObject *obj, const char *name);

            bool MatchesName(const char *name) const;
            KAutoObject *GetObject() const { return this->object; }
    };

}
