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
#include "ro_random.hpp"

namespace ams::ro::impl {

    u64 GenerateSecureRandom(u64 max) {
        /* Generate a cryptographically random number. */
        u64 rand;
        crypto::GenerateCryptographicallyRandomBytes(std::addressof(rand), sizeof(rand));

        /* Coerce into range. */
        return rand % (max + 1);
    }

}