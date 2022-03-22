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

#include <vapours/util/util_type_traits.hpp>
#include <vapours/util/util_alignment.hpp>
#include <vapours/util/util_size.hpp>
#include <vapours/util/util_int_util.hpp>
#include <vapours/util/util_aligned_buffer.hpp>
#include <vapours/util/util_enum.hpp>
#include <vapours/util/util_endian.hpp>
#include <vapours/util/util_exchange.hpp>
#include <vapours/util/util_scope_guard.hpp>
#include <vapours/util/util_specialization_of.hpp>
#include <vapours/util/util_optional.hpp>
#include <vapours/util/util_bitpack.hpp>
#include <vapours/util/util_bitset.hpp>
#include <vapours/util/util_bitflagset.hpp>
#include <vapours/util/util_bitutil.hpp>
#include <vapours/util/util_typed_storage.hpp>
#include <vapours/util/util_fourcc.hpp>
#include <vapours/util/util_intrusive_list.hpp>
#include <vapours/util/util_intrusive_red_black_tree.hpp>
#include <vapours/util/util_tinymt.hpp>
#include <vapours/util/util_timer.hpp>
#include <vapours/util/util_uuid.hpp>
#include <vapours/util/util_bounded_map.hpp>
#include <vapours/util/util_overlap.hpp>
#include <vapours/util/util_string_util.hpp>
#include <vapours/util/util_string_view.hpp>
#include <vapours/util/util_variadic.hpp>
#include <vapours/util/util_character_encoding.hpp>
#include <vapours/util/util_format_string.hpp>
#include <vapours/util/util_range.hpp>
#include <vapours/util/util_utf8_string_util.hpp>

#include <vapours/util/util_fixed_map.hpp>
#include <vapours/util/util_fixed_set.hpp>

#include <vapours/util/util_atomic.hpp>

#include <vapours/util/util_function_local_static.hpp>

#ifdef ATMOSPHERE_IS_STRATOSPHERE
#include <vapours/util/util_mutex_utils.hpp>
#endif
