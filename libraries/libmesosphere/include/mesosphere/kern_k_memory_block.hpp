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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_typed_address.hpp>
#include <mesosphere/kern_slab_helpers.hpp>

namespace ams::kern {

    enum KMemoryState : u32 {
        KMemoryState_None = 0,
        KMemoryState_Mask = 0xFF,

        KMemoryState_FlagCanReprotect           = (1 <<  8),
        KMemoryState_FlagCanDebug               = (1 <<  9),
        KMemoryState_FlagCanUseIpc              = (1 << 10),
        KMemoryState_FlagCanUseNonDeviceIpc     = (1 << 11),
        KMemoryState_FlagCanUseNonSecureIpc     = (1 << 12),
        KMemoryState_FlagMapped                 = (1 << 13),
        KMemoryState_FlagCode                   = (1 << 14),
        KMemoryState_FlagCanAlias               = (1 << 15),
        KMemoryState_FlagCanCodeAlias           = (1 << 16),
        KMemoryState_FlagCanTransfer            = (1 << 17),
        KMemoryState_FlagCanQueryPhysical       = (1 << 18),
        KMemoryState_FlagCanDeviceMap           = (1 << 19),
        KMemoryState_FlagCanAlignedDeviceMap    = (1 << 20),
        KMemoryState_FlagCanIpcUserBuffer       = (1 << 21),
        KMemoryState_FlagReferenceCounted       = (1 << 22),
        KMemoryState_FlagCanMapProcess          = (1 << 23),
        KMemoryState_FlagCanChangeAttribute     = (1 << 24),
        KMemoryState_FlagCanCodeMemory          = (1 << 25),

        KMemoryState_FlagsData = KMemoryState_FlagCanReprotect          | KMemoryState_FlagCanUseIpc            |
                                 KMemoryState_FlagCanUseNonDeviceIpc    | KMemoryState_FlagCanUseNonSecureIpc   |
                                 KMemoryState_FlagMapped                | KMemoryState_FlagCanAlias             |
                                 KMemoryState_FlagCanTransfer           | KMemoryState_FlagCanQueryPhysical     |
                                 KMemoryState_FlagCanDeviceMap          | KMemoryState_FlagCanAlignedDeviceMap  |
                                 KMemoryState_FlagCanIpcUserBuffer      | KMemoryState_FlagReferenceCounted     |
                                 KMemoryState_FlagCanChangeAttribute,

        KMemoryState_FlagsCode = KMemoryState_FlagCanDebug              | KMemoryState_FlagCanUseIpc            |
                                 KMemoryState_FlagCanUseNonDeviceIpc    | KMemoryState_FlagCanUseNonSecureIpc   |
                                 KMemoryState_FlagMapped                | KMemoryState_FlagCode                 |
                                 KMemoryState_FlagCanQueryPhysical      | KMemoryState_FlagCanDeviceMap         |
                                 KMemoryState_FlagCanAlignedDeviceMap   | KMemoryState_FlagReferenceCounted,

        KMemoryState_FlagsMisc = KMemoryState_FlagMapped                | KMemoryState_FlagReferenceCounted     |
                                 KMemoryState_FlagCanQueryPhysical      | KMemoryState_FlagCanDeviceMap,


        KMemoryState_Free               = ams::svc::MemoryState_Free,
        KMemoryState_Io                 = ams::svc::MemoryState_Io                  | KMemoryState_FlagMapped,
        KMemoryState_Static             = ams::svc::MemoryState_Static              | KMemoryState_FlagMapped | KMemoryState_FlagCanQueryPhysical,
        KMemoryState_Code               = ams::svc::MemoryState_Code                | KMemoryState_FlagsCode  | KMemoryState_FlagCanMapProcess,
        KMemoryState_CodeData           = ams::svc::MemoryState_CodeData            | KMemoryState_FlagsData  | KMemoryState_FlagCanMapProcess       | KMemoryState_FlagCanCodeMemory,
        KMemoryState_Normal             = ams::svc::MemoryState_Normal              | KMemoryState_FlagsData  | KMemoryState_FlagCanCodeMemory,
        KMemoryState_Shared             = ams::svc::MemoryState_Shared              | KMemoryState_FlagMapped | KMemoryState_FlagReferenceCounted,

        /* KMemoryState_Alias was removed after 1.0.0. */

        KMemoryState_AliasCode          = ams::svc::MemoryState_AliasCode           | KMemoryState_FlagsCode  | KMemoryState_FlagCanMapProcess       | KMemoryState_FlagCanCodeAlias,
        KMemoryState_AliasCodeData      = ams::svc::MemoryState_AliasCodeData       | KMemoryState_FlagsData  | KMemoryState_FlagCanMapProcess       | KMemoryState_FlagCanCodeAlias       | KMemoryState_FlagCanCodeMemory,


        KMemoryState_Ipc                = ams::svc::MemoryState_Ipc                 | KMemoryState_FlagsMisc  | KMemoryState_FlagCanAlignedDeviceMap
                                                                                                              | KMemoryState_FlagCanUseIpc           | KMemoryState_FlagCanUseNonSecureIpc | KMemoryState_FlagCanUseNonDeviceIpc,

        KMemoryState_Stack              = ams::svc::MemoryState_Stack               | KMemoryState_FlagsMisc  | KMemoryState_FlagCanAlignedDeviceMap
                                                                                                              | KMemoryState_FlagCanUseIpc           | KMemoryState_FlagCanUseNonSecureIpc | KMemoryState_FlagCanUseNonDeviceIpc,

        KMemoryState_ThreadLocal        = ams::svc::MemoryState_ThreadLocal         | KMemoryState_FlagMapped | KMemoryState_FlagReferenceCounted,

        KMemoryState_Transfered         = ams::svc::MemoryState_Transfered          | KMemoryState_FlagsMisc  | KMemoryState_FlagCanAlignedDeviceMap | KMemoryState_FlagCanChangeAttribute
                                                                                                              | KMemoryState_FlagCanUseIpc           | KMemoryState_FlagCanUseNonSecureIpc | KMemoryState_FlagCanUseNonDeviceIpc,

        KMemoryState_SharedTransfered   = ams::svc::MemoryState_SharedTransfered    | KMemoryState_FlagsMisc  | KMemoryState_FlagCanAlignedDeviceMap
                                                                                                              | KMemoryState_FlagCanUseNonSecureIpc  | KMemoryState_FlagCanUseNonDeviceIpc,

        KMemoryState_SharedCode         = ams::svc::MemoryState_SharedCode          | KMemoryState_FlagMapped | KMemoryState_FlagReferenceCounted
                                                                                                              | KMemoryState_FlagCanUseNonSecureIpc  | KMemoryState_FlagCanUseNonDeviceIpc,

        KMemoryState_Inaccessible       = ams::svc::MemoryState_Inaccessible,

        KMemoryState_NonSecureIpc       = ams::svc::MemoryState_NonSecureIpc        | KMemoryState_FlagsMisc  | KMemoryState_FlagCanAlignedDeviceMap
                                                                                                              | KMemoryState_FlagCanUseNonSecureIpc  | KMemoryState_FlagCanUseNonDeviceIpc,

        KMemoryState_NonDeviceIpc       = ams::svc::MemoryState_NonDeviceIpc        | KMemoryState_FlagsMisc  | KMemoryState_FlagCanUseNonDeviceIpc,


        KMemoryState_Kernel             = ams::svc::MemoryState_Kernel              | KMemoryState_FlagMapped,

        KMemoryState_GeneratedCode      = ams::svc::MemoryState_GeneratedCode       | KMemoryState_FlagMapped | KMemoryState_FlagReferenceCounted    | KMemoryState_FlagCanDebug,
        KMemoryState_CodeOut            = ams::svc::MemoryState_CodeOut             | KMemoryState_FlagMapped | KMemoryState_FlagReferenceCounted,
    };

#if 1
    static_assert(KMemoryState_Free             == 0x00000000);
    static_assert(KMemoryState_Io               == 0x00002001);
    static_assert(KMemoryState_Static           == 0x00042002);
    static_assert(KMemoryState_Code             == 0x00DC7E03);
    static_assert(KMemoryState_CodeData         == 0x03FEBD04);
    static_assert(KMemoryState_Normal           == 0x037EBD05);
    static_assert(KMemoryState_Shared           == 0x00402006);

    static_assert(KMemoryState_AliasCode        == 0x00DD7E08);
    static_assert(KMemoryState_AliasCodeData    == 0x03FFBD09);
    static_assert(KMemoryState_Ipc              == 0x005C3C0A);
    static_assert(KMemoryState_Stack            == 0x005C3C0B);
    static_assert(KMemoryState_ThreadLocal      == 0x0040200C);
    static_assert(KMemoryState_Transfered       == 0x015C3C0D);
    static_assert(KMemoryState_SharedTransfered == 0x005C380E);
    static_assert(KMemoryState_SharedCode       == 0x0040380F);
    static_assert(KMemoryState_Inaccessible     == 0x00000010);
    static_assert(KMemoryState_NonSecureIpc     == 0x005C3811);
    static_assert(KMemoryState_NonDeviceIpc     == 0x004C2812);
    static_assert(KMemoryState_Kernel           == 0x00002013);
    static_assert(KMemoryState_GeneratedCode    == 0x00402214);
    static_assert(KMemoryState_CodeOut          == 0x00402015);
#endif

    enum KMemoryPermission : u8 {
        KMemoryPermission_None              = 0,

        KMemoryPermission_UserRead          = ams::svc::MemoryPermission_Read,
        KMemoryPermission_UserWrite         = ams::svc::MemoryPermission_Write,
        KMemoryPermission_UserExecute       = ams::svc::MemoryPermission_Execute,

        KMemoryPermission_UserReadWrite     = ams::svc::MemoryPermission_ReadWrite,
        KMemoryPermission_UserReadExecute   = ams::svc::MemoryPermission_ReadExecute,

        KMemoryPermission_UserMask          = KMemoryPermission_UserRead | KMemoryPermission_UserWrite | KMemoryPermission_UserExecute,

        KMemoryPermission_KernelShift       = 3,

        KMemoryPermission_KernelRead        = KMemoryPermission_UserRead    << KMemoryPermission_KernelShift,
        KMemoryPermission_KernelWrite       = KMemoryPermission_UserWrite   << KMemoryPermission_KernelShift,
        KMemoryPermission_KernelExecute     = KMemoryPermission_UserExecute << KMemoryPermission_KernelShift,

        KMemoryPermission_KernelReadWrite   = KMemoryPermission_KernelRead | KMemoryPermission_KernelWrite,
        KMemoryPermission_KernelReadExecute = KMemoryPermission_KernelRead | KMemoryPermission_KernelExecute,
    };

    constexpr KMemoryPermission ConvertToKMemoryPermission(ams::svc::MemoryPermission perm) {
        return static_cast<KMemoryPermission>((perm & KMemoryPermission_UserMask) | KMemoryPermission_KernelRead | ((perm & KMemoryPermission_UserWrite) << KMemoryPermission_KernelShift));
    }

    enum KMemoryAttribute : u8 {
        KMemoryAttribute_None           = 0x00,

        KMemoryAttribute_Locked         = ams::svc::MemoryAttribute_Locked,
        KMemoryAttribute_IpcLocked      = ams::svc::MemoryAttribute_IpcLocked,
        KMemoryAttribute_DeviceShared   = ams::svc::MemoryAttribute_DeviceShared,
        KMemoryAttribute_Uncached       = ams::svc::MemoryAttribute_Uncached,
    };

    class KMemoryBlock : public util::IntrusiveRedBlackTreeBaseNode<KMemoryBlock> {
        private:
            KProcessAddress address;
            size_t num_pages;
            KMemoryState memory_state;
            u16 ipc_lock_count;
            u16 device_use_count;
            KMemoryPermission perm;
            KMemoryPermission original_perm;
            KMemoryAttribute attribute;
        public:
            constexpr KMemoryBlock()
                : address(), num_pages(), memory_state(KMemoryState_None), ipc_lock_count(), device_use_count(), perm(), original_perm(), attribute()
            {
                /* ... */
            }
    };

}
