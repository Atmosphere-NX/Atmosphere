/*
 * Copyright (c) 2018 Atmosphère-NX
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

#include <functional>

namespace detail {

template<typename T>
struct class_of;

template<typename Ret, typename C>
struct class_of<Ret C::*> {
    using type = C;
};

template<typename T>
using class_of_t = typename class_of<T>::type;

template<typename Mem, typename T, typename C = class_of_t<Mem>>
struct member_equals_fn_helper {
    T ref;
    Mem mem_fn;

    bool operator()(const C& val) const {
        return (std::mem_fn(mem_fn)(val) == ref);
    }

    bool operator()(C&& val) const {
        return (std::mem_fn(mem_fn)(std::move(val)) == ref);
    }
};

} // namespace detail

template<typename Mem, typename T>
auto member_equals_fn(Mem mem, T ref) {
    return detail::member_equals_fn_helper<Mem, T>{std::move(ref), std::move(mem)};
}
