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

namespace ams::prfile2::cache {

    void SetCache(Volume *vol, pf::CachePage *cache_page, pf::SectorBuffer *cache_buf, u16 num_fat_pages, u16 num_data_pages) {
        /* Set the cache fields. */
        vol->cache.pages          = cache_page;
        vol->cache.buffers        = cache_buf;
        vol->cache.num_fat_pages  = num_fat_pages;
        vol->cache.num_data_pages = num_data_pages;
        std::memset(vol->cache.pages, 0, sizeof(*vol->cache.pages) * (num_fat_pages + num_data_pages));
    }

    void SetFatBufferSize(Volume *vol, u32 size) {
        if (size > 0) {
            vol->cache.fat_buf_size = size;
        }
    }

    void SetDataBufferSize(Volume *vol, u32 size) {
        if (size > 0) {
            vol->cache.data_buf_size = size;
        }
    }

}
