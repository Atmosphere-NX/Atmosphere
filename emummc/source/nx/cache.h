/**
 * @file cache.h
 * @brief AArch64 cache operations.
 * @author plutoo
 * @copyright libnx Authors
 */
#pragma once
#include "../utils/types.h"

/**
 * @brief Performs a data cache flush on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @remarks Cache flush is defined as Clean + Invalidate.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 */
void armDCacheFlush(void* addr, size_t size);

/**
 * @brief Performs a data cache clean on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 */
void armDCacheClean(void* addr, size_t size);

/**
 * @brief Performs an instruction cache invalidation clean on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 */
void armICacheInvalidate(void* addr, size_t size);

/**
 * @brief Performs a data cache zeroing operation on the specified buffer.
 * @param addr Address of the buffer.
 * @param size Size of the buffer, in bytes.
 * @note The start and end addresses of the buffer are forcibly rounded to cache line boundaries (read from CTR_EL0 system register).
 */
void armDCacheZero(void* addr, size_t size);
