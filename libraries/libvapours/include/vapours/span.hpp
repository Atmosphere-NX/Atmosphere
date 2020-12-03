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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

namespace ams {

    template<typename T>
    using Span = std::span<T>;

    template<typename T>
    constexpr Span<T> MakeSpan(T *ptr, size_t size) { return { ptr, size }; }

    template <typename T>
    constexpr Span<T> MakeSpan(T *begin, T *end) { return { begin, end }; }

    template<typename T, size_t Size>
    constexpr Span<T> MakeSpan(T (&arr)[Size]) { return Span<T>(arr); }

    template<typename T, size_t Size>
    constexpr Span<T> MakeSpan(std::array<T, Size> &arr) { return Span<T>(arr); }

    template<typename T, size_t Size>
    constexpr Span<T> MakeSpan(const std::array<T, Size> &arr) { return Span<const T>(arr); }

}
