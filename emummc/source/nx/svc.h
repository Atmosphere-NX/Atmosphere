/**
 * @file svc.h
 * @brief Wrappers for kernel syscalls.
 * @copyright libnx Authors
 */
#pragma once
#include "../utils/types.h"

/// Memory information structure.
typedef struct {
    u64 addr;            ///< Base address.
    u64 size;            ///< Size.
    u32 type;            ///< Memory type (see lower 8 bits of \ref MemoryState).
    u32 attr;            ///< Memory attributes (see \ref MemoryAttribute).
    u32 perm;            ///< Memory permissions (see \ref Permission).
    u32 device_refcount; ///< Device reference count.
    u32 ipc_refcount;    ///< IPC reference count.
    u32 padding;         ///< Padding.
} MemoryInfo;

/// Memory permission bitmasks.
typedef enum {
    Perm_None     = 0,               ///< No permissions.
    Perm_R        = BIT(0),          ///< Read permission.
    Perm_W        = BIT(1),          ///< Write permission.
    Perm_X        = BIT(2),          ///< Execute permission.
    Perm_Rw       = Perm_R | Perm_W, ///< Read/write permissions.
    Perm_Rx       = Perm_R | Perm_X, ///< Read/execute permissions.
    Perm_DontCare = BIT(28),         ///< Don't care
} Permission;

/// Secure monitor arguments.
typedef struct {
    u64 X[8]; ///< Values of X0 through X7.
} SecmonArgs;

_Static_assert(sizeof(SecmonArgs) == 0x40, "SecmonArgs definition");

#define DeviceName_SDMMC1A 19
#define DeviceName_SDMMC2A 20
#define DeviceName_SDMMC3A 21
#define DeviceName_SDMMC4A 22

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns a virtual address mapped to a given IO range.
 * @return Result code.
 * @note Syscall number 0x55.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 * @warning Only exists on [10.0.0+]. For older versions use \ref svcLegacyQueryIoMapping.
 */
Result svcQueryIoMapping(u64* virtaddr, u64* out_size, u64 physaddr, u64 size);

/**
 * @brief Returns a virtual address mapped to a given IO range.
 * @return Result code.
 * @note Syscall number 0x55.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 * @warning Only exists on [1.0.0-9.2.0]. For newer versions use \ref svcQueryIoMapping.
 */
Result svcLegacyQueryIoMapping(u64* virtaddr, u64 physaddr, u64 size);

/**
 * @brief Attaches a device address space to a device.
 * @return Result code.
 * @note Syscall number 0x57.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcAttachDeviceAddressSpace(u64 device, Handle handle);

/**
 * @brief Query information about an address. Will always fetch the lowest page-aligned mapping that contains the provided address.
 * @param[out] meminfo_ptr \ref MemoryInfo structure which will be filled in.
 * @param[out] pageinfo Page information which will be filled in.
 * @param[in] addr Address to query.
 * @return Result code.
 * @note Syscall number 0x06.
 */
Result svcQueryMemory(MemoryInfo* meminfo_ptr, u32 *pageinfo, u64 addr);

/**
 * @brief Sets the memory permissions for the specified memory with the supplied process handle.
 * @param[in] proc Process handle.
 * @param[in] addr Address of the memory.
 * @param[in] size Size of the memory.
 * @param[in] perm Permissions (see \ref Permission).
 * @return Result code.
 * @remark This returns an error (0xD801) when \p perm is >0x5, hence -WX and RWX are not allowed.
 * @note Syscall number 0x73.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
Result svcSetProcessMemoryPermission(Handle proc, u64 addr, u64 size, u32 perm);

/**
 * @brief Set the memory permissions of a (page-aligned) range of memory.
 * @param[in] addr Start address of the range.
 * @param[in] size Size of the range, in bytes.
 * @param[in] perm Permissions (see \ref Permission).
 * @return Result code.
 * @remark Perm_X is not allowed. Setting write-only is not allowed either (Perm_W).
 *         This can be used to move back and forth between Perm_None, Perm_R and Perm_Rw.
 * @note Syscall number 0x01.
 */
Result svcSetMemoryPermission(void* addr, u64 size, u32 perm);

/**
 * @brief Calls a secure monitor function (TrustZone, EL3).
 * @param regs Arguments to pass to the secure monitor.
 * @return Return value from the secure monitor.
 * @note Syscall number 0x7F.
 * @warning This is a privileged syscall. Use \ref envIsSyscallHinted to check if it is available.
 */
u64 svcCallSecureMonitor(SecmonArgs* regs);

#ifdef __cplusplus
}
#endif

///@}
