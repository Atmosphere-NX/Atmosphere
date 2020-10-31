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
#include <vapours.hpp>
#include <stratosphere/ddsf/ddsf_types.hpp>

namespace ams::ddsf::impl {

    #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)

        #define AMS_DDSF_CASTABLE_ROOT_TRAITS(__CLASS__)                                                                                    \
            public:                                                                                                                         \
                static constexpr inline ::ams::ddsf::impl::TypeTag s_ams_ddsf_castable_type_tag{#__CLASS__};                                \
                constexpr virtual const ::ams::ddsf::impl::TypeTag &GetTypeTag() const override { return s_ams_ddsf_castable_type_tag; }

    #else

        #define AMS_DDSF_CASTABLE_ROOT_TRAITS(__CLASS__)                                                                                    \
            public:                                                                                                                         \
                static constexpr inline ::ams::ddsf::impl::TypeTag s_ams_ddsf_castable_type_tag{};                                          \
                constexpr virtual const ::ams::ddsf::impl::TypeTag &GetTypeTag() const override { return s_ams_ddsf_castable_type_tag; }

    #endif

    class TypeTag {
        private:
            const char * const class_name;
            const TypeTag * const base;
        public:
            #if !(defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING))
                constexpr TypeTag() : class_name(nullptr), base(nullptr) { /* ... */}
                constexpr TypeTag(const TypeTag &b) : class_name(nullptr), base(std::addressof(b)) { AMS_ASSERT(this != this->base); }

                constexpr TypeTag(const char *c) : class_name(nullptr), base(nullptr) { AMS_UNUSED(c); }
                constexpr TypeTag(const char *c, const TypeTag &b) : class_name(nullptr), base(std::addressof(b)) { AMS_UNUSED(c); AMS_ASSERT(this != this->base); }
            #else
                constexpr TypeTag(const char *c) : class_name(c), base(nullptr) { /* ... */ }
                constexpr TypeTag(const char *c, const TypeTag &b) : class_name(c), base(std::addressof(b)) { AMS_ASSERT(this != this->base); }
            #endif

            constexpr const char * GetClassName() const { return this->class_name; }

            constexpr bool Is(const TypeTag &rhs) const { return this == std::addressof(rhs); }

            constexpr bool DerivesFrom(const TypeTag &rhs) const {
                const TypeTag * cur = this;
                while (cur != nullptr) {
                    if (cur == std::addressof(rhs)) {
                        return true;
                    }
                    cur = cur->base;
                }
                return false;
            }
    };

}
