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
#include <type_traits>
#include <concepts>
#include <algorithm>
#include <iterator>
#include <limits>
#include <random>
#include <atomic>
#include <utility>
#include <optional>
#include <functional>
#include <tuple>
#include <array>
#include <bit>
#include <span>

/* Stratosphere wants additional libstdc++ headers, others do not. */
#ifdef ATMOSPHERE_IS_STRATOSPHERE

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <map>
#include <unordered_map>
#include <set>

/* Libnx. */
#include <switch.h>

#else

/* Non-EL0 code can't include libnx. */
#include "types.hpp"

#endif /* ATMOSPHERE_IS_STRATOSPHERE */

/* Atmosphere meta. */
#include <vapours/ams_version.h>
