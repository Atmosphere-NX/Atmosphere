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
#include <vapours/util.hpp>
#include <vapours/crypto/crypto_memory_compare.hpp>
#include <vapours/crypto/crypto_memory_clear.hpp>

namespace ams::crypto::impl {

    template<typename T>
    concept HashFunction = requires(T &t, const void *cv, void *v, size_t sz) {
        { T::HashSize      } -> std::convertible_to<size_t>;
        { T::BlockSize     } -> std::convertible_to<size_t>;
        { t.Initialize()   } -> std::same_as<void>;
        { t.Update(cv, sz) } -> std::same_as<void>;
        { t.GetHash(v, sz) } -> std::same_as<void>;
    };

}
