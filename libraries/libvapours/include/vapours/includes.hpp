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

/* C headers. */
#include <cstdint>
#include <cstdarg>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <climits>
#include <cctype>


#include <type_traits>
#include <algorithm>
#include <iterator>
#include <limits>
#include <random>

/* Stratosphere wants stdlib headers, others do not.. */
#ifdef ATMOSPHERE_IS_STRATOSPHERE

/* C++ headers. */
#include <atomic>
#include <utility>
#include <optional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <functional>
#include <tuple>
#include <array>
#include <map>
#include <unordered_map>
#include <set>

#endif /* ATMOSPHERE_IS_STRATOSPHERE */

#ifdef ATMOSPHERE_BOARD_NINTENDO_SWITCH

/* Libnx. */
#include <switch.h>

#else

#error "Unsupported board"

#endif /* ATMOSPHERE_BOARD_NINTENDO_SWITCH */

/* Atmosphere meta. */
#include "ams_version.h"
