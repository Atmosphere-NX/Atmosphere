/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <switch.h>
#include <type_traits>

/* Declare false allowed struct. */
template <typename>
struct AllowedOut : std::false_type {};

struct OutDataTag{};
struct OutHandleTag{};
struct OutSessionTag{};

/* Define out struct, so that we can get errors on enable_if */
template <typename T, typename Allowed = void>
class Out {
    static_assert(std::is_pod<T>::value && !std::is_pod<T>::value, "Invalid IPC Out Type!");
};

template <typename T>
class Out<T, typename std::enable_if<std::is_trivial<T>::value || AllowedOut<T>::value>::type> : public OutDataTag {
private:
    T *obj;
public:
    Out(T *o) : obj(o) { }

    void SetValue(const T& t) {
        *obj = t;
    }

    const T& GetValue() {
        return *obj;
    }

    T *GetPointer() {
        return obj;
    }

    /* Convenience operators. */
    T& operator*() {
        return *obj;
    }

    T* operator->() {
        return obj;
    }
};

template <typename T>
class Out<T*> {
    static_assert(std::is_pod<T>::value && !std::is_pod<T>::value, "Invalid IPC Out Type (Raw Pointer)!");
};

template <typename T>
struct OutHelper;

template <typename T>
struct OutHelper<Out<T>> {
    using type = T;
};