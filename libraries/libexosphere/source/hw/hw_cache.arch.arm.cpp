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
#include <exosphere.hpp>

namespace ams::hw::arch::arm {

#ifdef __BPMP__

    namespace {

        constexpr inline uintptr_t AVP_CACHE = AVP_CACHE_ADDR(0);

        ALWAYS_INLINE bool IsLargeBuffer(size_t size) {
            /* From TRM: For very large physical buffers or when the full cache needs to be cleared, */
            /*           software should simply loop over all lines in all ways and run the *_LINE command on each of them. */
            return size >= DataCacheSize / 4;
        }

        ALWAYS_INLINE bool IsCacheEnabled() {
            return reg::HasValue(AVP_CACHE + AVP_CACHE_CONFIG, AVP_CACHE_REG_BITS_ENUM(CONFIG_ENABLE_CACHE, TRUE));
        }

        void DoPhyCacheOperation(AVP_CACHE_MAINT_OPCODE op, uintptr_t addr) {
            /* Clear maintenance done. */
            reg::Write(AVP_CACHE + AVP_CACHE_INT_CLEAR, AVP_CACHE_REG_BITS_ENUM(INT_CLEAR_MAINTENANCE_DONE, TRUE));

            /* Write maintenance address. */
            reg::Write(AVP_CACHE + AVP_CACHE_MAINT_0, addr);

            /* Write maintenance request. */
            reg::Write(AVP_CACHE + AVP_CACHE_MAINT_2, AVP_CACHE_REG_BITS_VALUE(MAINT_2_WAY_BITMAP, 0x0),
                                                      AVP_CACHE_REG_BITS_VALUE(MAINT_2_OPCODE,      op));

            /* Wait for maintenance to be done. */
            while (!reg::HasValue(AVP_CACHE + AVP_CACHE_INT_RAW_EVENT, AVP_CACHE_REG_BITS_ENUM(INT_RAW_EVENT_MAINTENANCE_DONE, TRUE))) {
                /* ... */
            }

            /* Clear raw event. */
            reg::Write(AVP_CACHE + AVP_CACHE_INT_CLEAR, reg::Read(AVP_CACHE + AVP_CACHE_INT_RAW_EVENT));
        }

        void DoEntireCacheOperation(AVP_CACHE_MAINT_OPCODE op) {
            /* Clear maintenance done. */
            reg::Write(AVP_CACHE + AVP_CACHE_INT_CLEAR, AVP_CACHE_REG_BITS_ENUM(INT_CLEAR_MAINTENANCE_DONE, TRUE));

            /* Write maintenance request. */
            reg::Write(AVP_CACHE + AVP_CACHE_MAINT_2, AVP_CACHE_REG_BITS_VALUE(MAINT_2_WAY_BITMAP, 0xF),
                                                      AVP_CACHE_REG_BITS_VALUE(MAINT_2_OPCODE,      op));

            /* Wait for maintenance to be done. */
            while (!reg::HasValue(AVP_CACHE + AVP_CACHE_INT_RAW_EVENT, AVP_CACHE_REG_BITS_ENUM(INT_RAW_EVENT_MAINTENANCE_DONE, TRUE))) {
                /* ... */
            }

            /* Clear raw event. */
            reg::Write(AVP_CACHE + AVP_CACHE_INT_CLEAR, reg::Read(AVP_CACHE + AVP_CACHE_INT_RAW_EVENT));
        }

    }

    #define REQUIRE_CACHE_ENABLED()                \
        do {                                       \
            if (AMS_UNLIKELY(!IsCacheEnabled())) { \
                return;                            \
            }                                      \
        } while (false)                            \


    #define REQUIRE_CACHE_DISABLED()              \
        do {                                      \
            if (AMS_UNLIKELY(IsCacheEnabled())) { \
                return;                           \
            }                                     \
        } while (false)                           \

    void InitializeDataCache() {
        REQUIRE_CACHE_DISABLED();

        /* Issue init mmu command. */
        reg::Write(AVP_CACHE + AVP_CACHE_MMU_CMD, AVP_CACHE_REG_BITS_ENUM(MMU_CMD_CMD, INIT));

        /* Set mmu fallback entry as RWX, uncached. */
        reg::Write(AVP_CACHE + AVP_CACHE_MMU_FALLBACK_ENTRY, AVP_CACHE_REG_BITS_ENUM(MMU_FALLBACK_ENTRY_WR_ENA,   ENABLE),
                                                             AVP_CACHE_REG_BITS_ENUM(MMU_FALLBACK_ENTRY_RD_ENA,   ENABLE),
                                                             AVP_CACHE_REG_BITS_ENUM(MMU_FALLBACK_ENTRY_EXE_ENA,  ENABLE),
                                                             AVP_CACHE_REG_BITS_ENUM(MMU_FALLBACK_ENTRY_CACHED,  DISABLE));

        /* Set mmu cfg. */
        reg::Write(AVP_CACHE + AVP_CACHE_MMU_CFG, AVP_CACHE_REG_BITS_ENUM(MMU_CFG_CLR_ABORT,                    NOP),
                                                  AVP_CACHE_REG_BITS_ENUM(MMU_CFG_ABORT_MODE,            STORE_LAST),
                                                  AVP_CACHE_REG_BITS_ENUM(MMU_CFG_SEQ_CHECK_ALL_ENTRIES,    DISABLE),
                                                  AVP_CACHE_REG_BITS_ENUM(MMU_CFG_TLB_ENA,                   ENABLE),
                                                  AVP_CACHE_REG_BITS_ENUM(MMU_CFG_SEQ_ENA,                   ENABLE),
                                                  AVP_CACHE_REG_BITS_ENUM(MMU_CFG_BLOCK_MAIN_ENTRY_WR,      DISABLE));

        /* Initialize mmu entries. */
        {
            /* Clear shadow copy mask. */
            reg::Write(AVP_CACHE + AVP_CACHE_MMU_SHADOW_COPY_MASK_0, 0);

            /* Add DRAM as index 0, RWX/Cached. */
            {
                reg::Write(AVP_CACHE + AVP_CACHE_MMU_SHADOW_ENTRY_0_MIN_ADDR, 0x80000000);
                reg::Write(AVP_CACHE + AVP_CACHE_MMU_SHADOW_ENTRY_0_MAX_ADDR, util::AlignDown(0xFFFFFFFF, DataCacheLineSize));

                reg::Write(AVP_CACHE + AVP_CACHE_MMU_SHADOW_ENTRY_0_CFG, AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_WR_ENA,  ENABLE),
                                                                         AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_RD_ENA,  ENABLE),
                                                                         AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_EXE_ENA, ENABLE),
                                                                         AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_CACHED,  ENABLE));

                reg::SetBits(AVP_CACHE + AVP_CACHE_MMU_SHADOW_COPY_MASK_0, (1 << 0));
            }

            /* Add IRAM as index 1, RWX/Cached. */
            {
                reg::Write(AVP_CACHE + AVP_CACHE_MMU_SHADOW_ENTRY_1_MIN_ADDR, 0x40000000);
                reg::Write(AVP_CACHE + AVP_CACHE_MMU_SHADOW_ENTRY_1_MAX_ADDR, util::AlignDown(0x4003FFFF, DataCacheLineSize));

                reg::Write(AVP_CACHE + AVP_CACHE_MMU_SHADOW_ENTRY_1_CFG, AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_WR_ENA,  ENABLE),
                                                                         AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_RD_ENA,  ENABLE),
                                                                         AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_EXE_ENA, ENABLE),
                                                                         AVP_CACHE_REG_BITS_ENUM(SHADOW_ENTRY_CFG_CACHED,  ENABLE));

                reg::SetBits(AVP_CACHE + AVP_CACHE_MMU_SHADOW_COPY_MASK_0, (1 << 1));
            }

            /* Issue copy shadow mmu command. */
            reg::Write(AVP_CACHE + AVP_CACHE_MMU_CMD, AVP_CACHE_REG_BITS_ENUM(MMU_CMD_CMD, COPY_SHADOW));
        }

        /* Invalidate entire cache. */
        DoEntireCacheOperation(AVP_CACHE_MAINT_OPCODE_INVALID_WAY);

        /* Enable the cache. */
        reg::Write(AVP_CACHE + AVP_CACHE_CONFIG, AVP_CACHE_REG_BITS_ENUM(CONFIG_ENABLE_CACHE,                 TRUE),
                                                 AVP_CACHE_REG_BITS_ENUM(CONFIG_FORCE_WRITE_THROUGH,          TRUE),
                                                 AVP_CACHE_REG_BITS_ENUM(CONFIG_MMU_TAG_MODE,             PARALLEL),
                                                 AVP_CACHE_REG_BITS_ENUM(CONFIG_TAG_CHECK_ABORT_ON_ERROR,     TRUE));

        /* Invalidate entire cache again (WAR for hardware bug). */
        DoEntireCacheOperation(AVP_CACHE_MAINT_OPCODE_INVALID_WAY);
    }

    void FinalizeDataCache() {
        REQUIRE_CACHE_ENABLED();

        /* Flush entire data cache. */
        FlushEntireDataCache();

        /* Disable cache. */
        reg::Write(AVP_CACHE + AVP_CACHE_CONFIG, AVP_CACHE_REG_BITS_ENUM(CONFIG_ENABLE_CACHE, FALSE));
    }

    void InvalidateEntireDataCache() {
        REQUIRE_CACHE_ENABLED();

        DoEntireCacheOperation(AVP_CACHE_MAINT_OPCODE_INVALID_WAY);
    }

    void StoreEntireDataCache() {
        REQUIRE_CACHE_ENABLED();

        DoEntireCacheOperation(AVP_CACHE_MAINT_OPCODE_CLEAN_WAY);
    }

    void FlushEntireDataCache() {
        REQUIRE_CACHE_ENABLED();

        DoEntireCacheOperation(AVP_CACHE_MAINT_OPCODE_CLEAN_INVALID_WAY);
    }

    void InvalidateDataCacheLine(void *ptr) {
        /* NOTE: Don't check cache enabled as an optimization, as only direct caller will be InvalidateDataCache(). */
        /* REQUIRE_CACHE_ENABLED(); */

        DoPhyCacheOperation(AVP_CACHE_MAINT_OPCODE_INVALID_PHY, reinterpret_cast<uintptr_t>(ptr));
    }

    void StoreDataCacheLine(void *ptr) {
        /* NOTE: Don't check cache enabled as an optimization, as only direct caller will be FlushDataCache(). */
        /* REQUIRE_CACHE_ENABLED(); */

        DoPhyCacheOperation(AVP_CACHE_MAINT_OPCODE_CLEAN_PHY, reinterpret_cast<uintptr_t>(ptr));
    }

    void FlushDataCacheLine(void *ptr) {
        /* NOTE: Don't check cache enabled as an optimization, as only direct caller will be FlushDataCache(). */
        /* REQUIRE_CACHE_ENABLED(); */

        DoPhyCacheOperation(AVP_CACHE_MAINT_OPCODE_CLEAN_INVALID_PHY, reinterpret_cast<uintptr_t>(ptr));
    }

    void InvalidateDataCache(void *ptr, size_t size) {
        REQUIRE_CACHE_ENABLED();

        if (IsLargeBuffer(size)) {
            InvalidateEntireDataCache();
        } else {
            const uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
            const uintptr_t end   = util::AlignUp(start + size, hw::DataCacheLineSize);

            for (uintptr_t cur = start; cur < end; cur += hw::DataCacheLineSize) {
                InvalidateDataCacheLine(reinterpret_cast<void *>(cur));
            }
        }
    }

    void StoreDataCache(const void *ptr, size_t size) {
        REQUIRE_CACHE_ENABLED();

        if (IsLargeBuffer(size)) {
            StoreEntireDataCache();
        } else {
            const uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
            const uintptr_t end   = util::AlignUp(start + size, hw::DataCacheLineSize);

            for (uintptr_t cur = start; cur < end; cur += hw::DataCacheLineSize) {
                StoreDataCacheLine(reinterpret_cast<void *>(cur));
            }
        }
    }

    void FlushDataCache(const void *ptr, size_t size) {
        REQUIRE_CACHE_ENABLED();

        if (IsLargeBuffer(size)) {
            FlushEntireDataCache();
        } else {
            const uintptr_t start = reinterpret_cast<uintptr_t>(ptr);
            const uintptr_t end   = util::AlignUp(start + size, hw::DataCacheLineSize);

            for (uintptr_t cur = start; cur < end; cur += hw::DataCacheLineSize) {
                FlushDataCacheLine(reinterpret_cast<void *>(cur));
            }
        }
    }
#endif

}