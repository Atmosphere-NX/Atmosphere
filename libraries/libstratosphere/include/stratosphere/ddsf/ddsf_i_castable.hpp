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
#include <stratosphere/ddsf/impl/ddsf_type_tag.hpp>

namespace ams::ddsf {

    #if defined(AMS_BUILD_FOR_DEBUGGING) || defined(AMS_BUILD_FOR_AUDITING)

        #define AMS_DDSF_CASTABLE_TRAITS(__CLASS__, __BASE__)                                                                                           \
            static_assert(std::convertible_to<__CLASS__ *, __BASE__ *>);                                                                                \
            public:                                                                                                                                     \
                static constexpr inline ::ams::ddsf::impl::TypeTag s_ams_ddsf_castable_type_tag{#__CLASS__, __BASE__::s_ams_ddsf_castable_type_tag};    \
                constexpr virtual const ::ams::ddsf::impl::TypeTag &GetTypeTag() const override { return s_ams_ddsf_castable_type_tag; }

    #else

        #define AMS_DDSF_CASTABLE_TRAITS(__CLASS__, __BASE__)                                                                                           \
            static_assert(std::convertible_to<__CLASS__ *, __BASE__ *>);                                                                                \
            public:                                                                                                                                     \
                static constexpr inline ::ams::ddsf::impl::TypeTag s_ams_ddsf_castable_type_tag{__BASE__::s_ams_ddsf_castable_type_tag};                \
                constexpr virtual const ::ams::ddsf::impl::TypeTag &GetTypeTag() const override { return s_ams_ddsf_castable_type_tag; }

    #endif

    class ICastable {
        private:
            constexpr virtual const impl::TypeTag &GetTypeTag() const = 0;

            template<typename T>
            constexpr ALWAYS_INLINE void AssertCastableTo() const {
                AMS_ASSERT(this->IsCastableTo<T>());
            }
        public:
            template<typename T>
            constexpr bool IsCastableTo() const {
                return this->GetTypeTag().DerivesFrom(T::s_ams_ddsf_castable_type_tag);
            }

            template<typename T>
            constexpr T &SafeCastTo() {
                this->AssertCastableTo<T>();
                return static_cast<T &>(*this);
            }

            template<typename T>
            constexpr const T &SafeCastTo() const {
                this->AssertCastableTo<T>();
                return static_cast<const T &>(*this);
            }

            template<typename T>
            constexpr T *SafeCastToPointer() {
                this->AssertCastableTo<T>();
                return static_cast<T *>(this);
            }

            template<typename T>
            constexpr const T *SafeCastToPointer() const {
                this->AssertCastableTo<T>();
                return static_cast<const T *>(this);
            }

            #if defined(AMS_BUILD_FOR_AUDITING) || defined(AMS_BUILD_FOR_DEBUGGING)

                constexpr const char *GetClassName() const {
                    return this->GetTypeTag().GetClassName();
                }

            #endif
    };

}
