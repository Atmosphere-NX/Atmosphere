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
#include <vapours/includes.hpp>
#include <vapours/defines.hpp>

#define AMS_PRAGMA(X) \
    _Pragma(#X)

#define AMS_PRAGMA_BEGIN_OPTIMIZE(X) \
    AMS_PRAGMA(GCC push_options)     \
    AMS_PRAGMA(GCC optimize(X))

#define AMS_PRAGMA_END_OPTIMIZE() \
    AMS_PRAGMA(GCC pop_options)

#define AMS_PRAGMA_BEGIN_PACK(n) \
    AMS_PRAGMA(pack(push, n))

#define AMS_PRAGMA_END_PACK() \
    AMS_PRAGMA(pack(pop))

#define AMS_CONCEPTS_REQUIRES_IF_SUPPORTED(__EXPR__) requires (__EXPR__)
