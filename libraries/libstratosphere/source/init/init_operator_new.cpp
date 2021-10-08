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
#include <stratosphere.hpp>

WEAK_SYMBOL void *operator new(size_t size) {
    return std::malloc(size);
}

WEAK_SYMBOL void *operator new(size_t size, const std::nothrow_t &) {
    return std::malloc(size);
}

WEAK_SYMBOL void operator delete(void *p) {
    return std::free(p);
}

WEAK_SYMBOL void operator delete(void *p, size_t) {
    return std::free(p);
}

WEAK_SYMBOL void *operator new[](size_t size) {
    return std::malloc(size);
}

WEAK_SYMBOL void *operator new[](size_t size, const std::nothrow_t &) {
    return std::malloc(size);
}

WEAK_SYMBOL void operator delete[](void *p) {
    return std::free(p);
}

WEAK_SYMBOL void operator delete[](void *p, size_t) {
    return std::free(p);
}
