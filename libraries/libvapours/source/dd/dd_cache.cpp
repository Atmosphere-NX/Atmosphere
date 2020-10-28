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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "impl/dd_select_cache_impl.hpp"

namespace ams::dd {

    void InvalidateDataCache(void *addr, size_t size) {
        return impl::InvalidateDataCacheImpl(addr, size);
    }

    void StoreDataCache(void *addr, size_t size) {
        return impl::StoreDataCacheImpl(addr, size);
    }

    void FlushDataCache(void *addr, size_t size) {
        return impl::FlushDataCacheImpl(addr, size);
    }

}
