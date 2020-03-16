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
#include <stratosphere.hpp>
#include "os_rng_manager_impl.hpp"

namespace ams::os::impl {

    u64 RngManager::GenerateRandomU64() {
        std::scoped_lock lk(this->lock);

        if (AMS_UNLIKELY(!this->initialized)) {
            this->Initialize();
        }

        return this->mt.GenerateRandomU64();
    }

}
