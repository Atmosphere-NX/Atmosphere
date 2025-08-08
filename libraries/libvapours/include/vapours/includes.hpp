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

/* Unconditionally include type-traits as first header. */
#include <type_traits>

/* C headers. */
#include <cstdint>
#include <cstdarg>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cctype>
#include <cinttypes>

/* C++ headers. */
#include <concepts>
#include <algorithm>
#include <iterator>
#include <limits>
#include <random>
#include <atomic>
#include <utility>
#include <functional>
#include <tuple>
#include <array>
#include <bit>
#include <span>

/* Stratosphere/Troposphere want additional libstdc++ headers and libnx,
 * others do not. */
#if defined(ATMOSPHERE_IS_STRATOSPHERE) || defined(ATMOSPHERE_IS_TROPOSPHERE)

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <map>
#include <unordered_map>
#include <set>

#if defined(ATMOSPHERE_OS_HORIZON) && defined(ATMOSPHERE_BOARD_NINTENDO_NX)

/* Libnx. */
#include <switch.h>

#else

/* Non-switch code can't include libnx. */
#include "types.hpp"

#endif

#else

/* Non-EL0 code can't include libnx. */
#include "types.hpp"

#endif /* defined(ATMOSPHERE_IS_STRATOSPHERE) || defined(ATMOSPHERE_IS_TROPOSPHERE) */

/* Atmosphere meta. */
#include <vapours/ams_version.h>
