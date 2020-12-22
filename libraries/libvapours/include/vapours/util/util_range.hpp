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

namespace ams::util::range {

    template<typename T, typename F>
    constexpr bool any_of(T &&t, F &&f) {
        return std::any_of(std::begin(t), std::end(t), std::forward<F>(f));
    }

    template<typename T, typename F>
    constexpr bool all_of(T &&t, F &&f) {
        return std::all_of(std::begin(t), std::end(t), std::forward<F>(f));
    }

    template<typename T, typename F>
    constexpr bool none_of(T &&t, F &&f) {
        return std::none_of(std::begin(t), std::end(t), std::forward<F>(f));
    }

    template<typename T, typename F>
    constexpr auto find_if(T &&t, F &&f) {
        return std::find_if(std::begin(t), std::end(t), std::forward<F>(f));
    }

    template<typename T, typename F>
    constexpr auto for_each(T &&t, F &&f) {
        return std::for_each(std::begin(t), std::end(t), std::forward<F>(f));
    }

}
