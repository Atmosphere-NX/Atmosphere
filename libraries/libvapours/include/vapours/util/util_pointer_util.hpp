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

#if defined(ATMOSPHERE_IS_STRATOSPHERE)
namespace ams::util {

    namespace impl {

        template<typename T>
        struct IsSharedPointerImpl : public std::false_type{};

        template<typename T>
        struct IsSharedPointerImpl<std::shared_ptr<T>> : public std::true_type{};

        template<typename T>
        struct IsUniquePointerImpl : public std::false_type{};

        template<typename T>
        struct IsUniquePointerImpl<std::unique_ptr<T>> : public std::true_type{};

        template<typename T, typename U>
        concept PointerToImpl = std::same_as<typename std::pointer_traits<T>::element_type, U>;

    }

    template<typename T>
    concept IsRawPointer   = std::is_pointer<T>::value;

    template<typename T>
    concept IsSharedPointer = impl::IsSharedPointerImpl<T>::value;

    template<typename T>
    concept IsUniquePointer = impl::IsUniquePointerImpl<T>::value;

    template<typename T>
    concept IsSmartPointer = IsSharedPointer<T> || IsUniquePointer<T>;

    template<typename T>
    concept IsRawOrSmartPointer = IsRawPointer<T> || IsSmartPointer<T>;

    template<typename T, typename U>
    concept SmartPointerTo = IsSmartPointer<T> && impl::PointerToImpl<T, U>;

    template<typename T, typename U>
    concept RawOrSmartPointerTo = IsRawOrSmartPointer<T> && impl::PointerToImpl<T, U>;

}
#endif
