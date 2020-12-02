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
        KMemoryState_All  = ~KMemoryState_None,

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
        KMemoryPermission_All               = static_cast<u8>(~KMemoryPermission_None),

        KMemoryPermission_KernelShift       = 3,

        KMemoryPermission_KernelRead        = ams::svc::MemoryPermission_Read    << KMemoryPermission_KernelShift,
        KMemoryPermission_KernelWrite       = ams::svc::MemoryPermission_Write   << KMemoryPermission_KernelShift,
        KMemoryPermission_KernelExecute     = ams::svc::MemoryPermission_Execute << KMemoryPermission_KernelShift,

        KMemoryPermission_NotMapped         = (1 << (2 * KMemoryPermission_KernelShift)),

        KMemoryPermission_KernelReadWrite   = KMemoryPermission_KernelRead | KMemoryPermission_KernelWrite,
        KMemoryPermission_KernelReadExecute = KMemoryPermission_KernelRead | KMemoryPermission_KernelExecute,

        KMemoryPermission_UserRead          = ams::svc::MemoryPermission_Read    | KMemoryPermission_KernelRead,
        KMemoryPermission_UserWrite         = ams::svc::MemoryPermission_Write   | KMemoryPermission_KernelWrite,
        KMemoryPermission_UserExecute       = ams::svc::MemoryPermission_Execute,

        KMemoryPermission_UserReadWrite     = KMemoryPermission_UserRead | KMemoryPermission_UserWrite,
        KMemoryPermission_UserReadExecute   = KMemoryPermission_UserRead | KMemoryPermission_UserExecute,

        KMemoryPermission_UserMask          = ams::svc::MemoryPermission_Read | ams::svc::MemoryPermission_Write | ams::svc::MemoryPermission_Execute,

        KMemoryPermission_IpcLockChangeMask = KMemoryPermission_NotMapped | KMemoryPermission_UserReadWrite,
    };

    constexpr KMemoryPermission ConvertToKMemoryPermission(ams::svc::MemoryPermission perm) {
        return static_cast<KMemoryPermission>((perm & KMemoryPermission_UserMask) | KMemoryPermission_KernelRead | ((perm & KMemoryPermission_UserWrite) << KMemoryPermission_KernelShift) | (perm == ams::svc::MemoryPermission_None ? KMemoryPermission_NotMapped : KMemoryPermission_None));
    }

    enum KMemoryAttribute : u8 {
        KMemoryAttribute_None           = 0x00,
        KMemoryAttribute_All            = 0xFF,
        KMemoryAttribute_UserMask       = KMemoryAttribute_All,

        KMemoryAttribute_Locked         = ams::svc::MemoryAttribute_Locked,
        KMemoryAttribute_IpcLocked      = ams::svc::MemoryAttribute_IpcLocked,
        KMemoryAttribute_DeviceShared   = ams::svc::MemoryAttribute_DeviceShared,
        KMemoryAttribute_Uncached       = ams::svc::MemoryAttribute_Uncached,

        KMemoryAttribute_SetMask        = KMemoryAttribute_Uncached,
    };

    enum KMemoryBlockDisableMergeAttribute : u8 {
        KMemoryBlockDisableMergeAttribute_None        = 0,
        KMemoryBlockDisableMergeAttribute_Normal      = (1u << 0),
        KMemoryBlockDisableMergeAttribute_DeviceLeft  = (1u << 1),
        KMemoryBlockDisableMergeAttribute_IpcLeft     = (1u << 2),
        KMemoryBlockDisableMergeAttribute_Locked      = (1u << 3),
        KMemoryBlockDisableMergeAttribute_DeviceRight = (1u << 4),

        KMemoryBlockDisableMergeAttribute_AllLeft  = KMemoryBlockDisableMergeAttribute_Normal | KMemoryBlockDisableMergeAttribute_DeviceLeft | KMemoryBlockDisableMergeAttribute_IpcLeft | KMemoryBlockDisableMergeAttribute_Locked,
        KMemoryBlockDisableMergeAttribute_AllRight = KMemoryBlockDisableMergeAttribute_DeviceRight,
    };

    struct KMemoryInfo {
        uintptr_t address;
        size_t size;
        KMemoryState state;
        u16 device_disable_merge_left_count;
        u16 device_disable_merge_right_count;
        u16 ipc_lock_count;
        u16 device_use_count;
        u16 ipc_disable_merge_count;
        KMemoryPermission perm;
        KMemoryAttribute  attribute;
        KMemoryPermission original_perm;
        KMemoryBlockDisableMergeAttribute disable_merge_attribute;

        constexpr ams::svc::MemoryInfo GetSvcMemoryInfo() const {
            return {
                .addr             = this->address,
                .size             = this->size,
                .state            = static_cast<ams::svc::MemoryState>(this->state & KMemoryState_Mask),
                .attr             = static_cast<ams::svc::MemoryAttribute>(this->attribute & KMemoryAttribute_UserMask),
                .perm             = static_cast<ams::svc::MemoryPermission>(this->perm & KMemoryPermission_UserMask),
                .ipc_refcount     = this->ipc_lock_count,
                .device_refcount  = this->device_use_count,
                .padding          = {},
            };
        }

        constexpr uintptr_t GetAddress() const {
            return this->address;
        }

        constexpr size_t GetSize() const {
            return this->size;
        }

        constexpr size_t GetNumPages() const {
            return this->GetSize() / PageSize;
        }

        constexpr uintptr_t GetEndAddress() const {
            return this->GetAddress() + this->GetSize();
        }

        constexpr uintptr_t GetLastAddress() const {
            return this->GetEndAddress() - 1;
        }

        constexpr u16 GetIpcLockCount() const {
            return this->ipc_lock_count;
        }

        constexpr u16 GetIpcDisableMergeCount() const {
            return this->ipc_disable_merge_count;
        }

        constexpr KMemoryState GetState() const {
            return this->state;
        }

        constexpr KMemoryPermission GetPermission() const {
            return this->perm;
        }

        constexpr KMemoryPermission GetOriginalPermission() const {
            return this->original_perm;
        }

        constexpr KMemoryAttribute GetAttribute() const {
            return this->attribute;
        }

        constexpr KMemoryBlockDisableMergeAttribute GetDisableMergeAttribute() const {
            return this->disable_merge_attribute;
        }
    };

    class KMemoryBlock : public util::IntrusiveRedBlackTreeBaseNode<KMemoryBlock> {
        private:
            u16 device_disable_merge_left_count;
            u16 device_disable_merge_right_count;
            KProcessAddress address;
            size_t num_pages;
            KMemoryState memory_state;
            u16 ipc_lock_count;
            u16 device_use_count;
            u16 ipc_disable_merge_count;
            KMemoryPermission perm;
            KMemoryPermission original_perm;
            KMemoryAttribute attribute;
            KMemoryBlockDisableMergeAttribute disable_merge_attribute;
        public:
            static constexpr ALWAYS_INLINE int Compare(const KMemoryBlock &lhs, const KMemoryBlock &rhs) {
                if (lhs.GetAddress() < rhs.GetAddress()) {
                    return -1;
                } else if (lhs.GetAddress() <= rhs.GetLastAddress()) {
                    return 0;
                } else {
                    return 1;
                }
            }
        public:
            constexpr KProcessAddress GetAddress() const {
                return this->address;
            }

            constexpr size_t GetNumPages() const {
                return this->num_pages;
            }

            constexpr size_t GetSize() const {
                return this->GetNumPages() * PageSize;
            }

            constexpr KProcessAddress GetEndAddress() const {
                return this->GetAddress() + this->GetSize();
            }

            constexpr KProcessAddress GetLastAddress() const {
                return this->GetEndAddress() - 1;
            }

            constexpr u16 GetIpcLockCount() const {
                return this->ipc_lock_count;
            }

            constexpr u16 GetIpcDisableMergeCount() const {
                return this->ipc_disable_merge_count;
            }

            constexpr KMemoryPermission GetPermission() const {
                return this->perm;
            }

            constexpr KMemoryPermission GetOriginalPermission() const {
                return this->original_perm;
            }

            constexpr KMemoryAttribute GetAttribute() const {
                return this->attribute;
            }

            constexpr KMemoryInfo GetMemoryInfo() const {
                return {
                    .address                          = GetInteger(this->GetAddress()),
                    .size                             = this->GetSize(),
                    .state                            = this->memory_state,
                    .device_disable_merge_left_count  = this->device_disable_merge_left_count,
                    .device_disable_merge_right_count = this->device_disable_merge_right_count,
                    .ipc_lock_count                   = this->ipc_lock_count,
                    .device_use_count                 = this->device_use_count,
                    .ipc_disable_merge_count          = this->ipc_disable_merge_count,
                    .perm                             = this->perm,
                    .attribute                        = this->attribute,
                    .original_perm                    = this->original_perm,
                    .disable_merge_attribute          = this->disable_merge_attribute,
                };
            }
        public:
            constexpr KMemoryBlock()
                : device_disable_merge_left_count(), device_disable_merge_right_count(), address(), num_pages(), memory_state(KMemoryState_None), ipc_lock_count(), device_use_count(), ipc_disable_merge_count(), perm(), original_perm(), attribute(), disable_merge_attribute()
            {
                /* ... */
            }

            constexpr KMemoryBlock(KProcessAddress addr, size_t np, KMemoryState ms, KMemoryPermission p, KMemoryAttribute attr)
                : device_disable_merge_left_count(), device_disable_merge_right_count(), address(addr), num_pages(np), memory_state(ms), ipc_lock_count(0), device_use_count(0), ipc_disable_merge_count(), perm(p), original_perm(KMemoryPermission_None), attribute(attr), disable_merge_attribute()
            {
                /* ... */
            }

            constexpr void Initialize(KProcessAddress addr, size_t np, KMemoryState ms, KMemoryPermission p, KMemoryAttribute attr) {
                MESOSPHERE_ASSERT_THIS();
                this->address           = addr;
                this->num_pages         = np;
                this->memory_state      = ms;
                this->ipc_lock_count    = 0;
                this->device_use_count  = 0;
                this->perm              = p;
                this->original_perm     = KMemoryPermission_None;
                this->attribute         = attr;
            }

            constexpr bool HasProperties(KMemoryState s, KMemoryPermission p, KMemoryAttribute a) const {
                MESOSPHERE_ASSERT_THIS();
                constexpr auto AttributeIgnoreMask = KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared;
                return this->memory_state == s && this->perm == p && (this->attribute | AttributeIgnoreMask) == (a | AttributeIgnoreMask);
            }

            constexpr bool HasSameProperties(const KMemoryBlock &rhs) const {
                MESOSPHERE_ASSERT_THIS();

                return this->memory_state     == rhs.memory_state   &&
                       this->perm             == rhs.perm           &&
                       this->original_perm    == rhs.original_perm  &&
                       this->attribute        == rhs.attribute      &&
                       this->ipc_lock_count   == rhs.ipc_lock_count &&
                       this->device_use_count == rhs.device_use_count;
            }

            constexpr bool CanMergeWith(const KMemoryBlock &rhs) const {
                return this->HasSameProperties(rhs) &&
                      (this->disable_merge_attribute & KMemoryBlockDisableMergeAttribute_AllRight) == 0 &&
                      (rhs.disable_merge_attribute & KMemoryBlockDisableMergeAttribute_AllLeft) == 0;
            }

            constexpr bool Contains(KProcessAddress addr) const {
                MESOSPHERE_ASSERT_THIS();

                return this->GetAddress() <= addr && addr <= this->GetEndAddress();
            }

            constexpr void Add(const KMemoryBlock &added_block) {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(added_block.GetNumPages() > 0);
                MESOSPHERE_ASSERT(this->GetAddress() + added_block.GetSize() - 1 < this->GetEndAddress() + added_block.GetSize() - 1);

                this->num_pages += added_block.GetNumPages();
                this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute | added_block.disable_merge_attribute);
                this->device_disable_merge_right_count = added_block.device_disable_merge_right_count;
            }

            constexpr void Update(KMemoryState s, KMemoryPermission p, KMemoryAttribute a, bool set_disable_merge_attr, u8 set_mask, u8 clear_mask) {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(this->original_perm == KMemoryPermission_None);
                MESOSPHERE_ASSERT((this->attribute & KMemoryAttribute_IpcLocked) == 0);

                this->memory_state = s;
                this->perm         = p;
                this->attribute    = static_cast<KMemoryAttribute>(a | (this->attribute & (KMemoryAttribute_IpcLocked | KMemoryAttribute_DeviceShared)));

                if (set_disable_merge_attr && set_mask != 0) {
                    this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute | set_mask);
                }
                if (clear_mask != 0) {
                    this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute & ~clear_mask);
                }
            }

            constexpr void Split(KMemoryBlock *block, KProcessAddress addr) {
                MESOSPHERE_ASSERT_THIS();
                MESOSPHERE_ASSERT(this->GetAddress() < addr);
                MESOSPHERE_ASSERT(this->Contains(addr));
                MESOSPHERE_ASSERT(util::IsAligned(GetInteger(addr), PageSize));

                block->address                          = this->address;
                block->num_pages                        = (addr - this->GetAddress()) / PageSize;
                block->memory_state                     = this->memory_state;
                block->ipc_lock_count                   = this->ipc_lock_count;
                block->device_use_count                 = this->device_use_count;
                block->perm                             = this->perm;
                block->original_perm                    = this->original_perm;
                block->attribute                        = this->attribute;
                block->disable_merge_attribute          = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute & KMemoryBlockDisableMergeAttribute_AllLeft);
                block->ipc_disable_merge_count          = this->ipc_disable_merge_count;
                block->device_disable_merge_left_count  = this->device_disable_merge_left_count;
                block->device_disable_merge_right_count = 0;

                this->address = addr;
                this->num_pages -= block->num_pages;

                this->ipc_disable_merge_count         = 0;
                this->device_disable_merge_left_count = 0;
                this->disable_merge_attribute         = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute & KMemoryBlockDisableMergeAttribute_AllRight);
            }

            constexpr void UpdateDeviceDisableMergeStateForShareLeft(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission/right aren't used. */
                MESOSPHERE_UNUSED(new_perm, right);

                if (left) {
                    this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute | KMemoryBlockDisableMergeAttribute_DeviceLeft);
                    const u16 new_device_disable_merge_left_count = ++this->device_disable_merge_left_count;
                    MESOSPHERE_ABORT_UNLESS(new_device_disable_merge_left_count > 0);
                }
            }

            constexpr void UpdateDeviceDisableMergeStateForShareRight(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission/left aren't used. */
                MESOSPHERE_UNUSED(new_perm, left);

                if (right) {
                    this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute | KMemoryBlockDisableMergeAttribute_DeviceRight);
                    const u16 new_device_disable_merge_right_count = ++this->device_disable_merge_right_count;
                    MESOSPHERE_ABORT_UNLESS(new_device_disable_merge_right_count > 0);
                }
            }

            constexpr void UpdateDeviceDisableMergeStateForShare(KMemoryPermission new_perm, bool left, bool right) {
                this->UpdateDeviceDisableMergeStateForShareLeft(new_perm, left, right);
                this->UpdateDeviceDisableMergeStateForShareRight(new_perm, left, right);
            }

            constexpr void ShareToDevice(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission isn't used. */
                MESOSPHERE_UNUSED(new_perm);

                /* We must either be shared or have a zero lock count. */
                MESOSPHERE_ASSERT((this->attribute & KMemoryAttribute_DeviceShared) == KMemoryAttribute_DeviceShared || this->device_use_count == 0);

                /* Share. */
                const u16 new_count = ++this->device_use_count;
                MESOSPHERE_ABORT_UNLESS(new_count > 0);

                this->attribute = static_cast<KMemoryAttribute>(this->attribute | KMemoryAttribute_DeviceShared);

                this->UpdateDeviceDisableMergeStateForShare(new_perm, left, right);
            }

            constexpr void UpdateDeviceDisableMergeStateForUnshareLeft(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission/right aren't used. */
                MESOSPHERE_UNUSED(new_perm, right);

                if (left) {
                    if (!this->device_disable_merge_left_count) {
                        return;
                    }
                    --this->device_disable_merge_left_count;
                }

                this->device_disable_merge_left_count = std::min(this->device_disable_merge_left_count, this->device_use_count);

                if (this->device_disable_merge_left_count == 0) {
                    this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute & ~KMemoryBlockDisableMergeAttribute_DeviceLeft);
                }
            }

            constexpr void UpdateDeviceDisableMergeStateForUnshareRight(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission/left aren't used. */
                MESOSPHERE_UNUSED(new_perm, left);

                if (right) {
                    const u16 old_device_disable_merge_right_count = this->device_disable_merge_right_count--;
                    MESOSPHERE_ASSERT(old_device_disable_merge_right_count > 0);
                    if (old_device_disable_merge_right_count == 1) {
                        this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute & ~KMemoryBlockDisableMergeAttribute_DeviceRight);
                    }
                }
            }

            constexpr void UpdateDeviceDisableMergeStateForUnshare(KMemoryPermission new_perm, bool left, bool right) {
                this->UpdateDeviceDisableMergeStateForUnshareLeft(new_perm, left, right);
                this->UpdateDeviceDisableMergeStateForUnshareRight(new_perm, left, right);
            }

            constexpr void UnshareToDevice(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission isn't used. */
                MESOSPHERE_UNUSED(new_perm);

                /* We must be shared. */
                MESOSPHERE_ASSERT((this->attribute & KMemoryAttribute_DeviceShared) == KMemoryAttribute_DeviceShared);

                /* Unhare. */
                const u16 old_count = this->device_use_count--;
                MESOSPHERE_ABORT_UNLESS(old_count > 0);

                if (old_count == 1) {
                    this->attribute = static_cast<KMemoryAttribute>(this->attribute & ~KMemoryAttribute_DeviceShared);
                }

                this->UpdateDeviceDisableMergeStateForUnshare(new_perm, left, right);
            }

            constexpr void UnshareToDeviceRight(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission isn't used. */
                MESOSPHERE_UNUSED(new_perm);

                /* We must be shared. */
                MESOSPHERE_ASSERT((this->attribute & KMemoryAttribute_DeviceShared) == KMemoryAttribute_DeviceShared);

                /* Unhare. */
                const u16 old_count = this->device_use_count--;
                MESOSPHERE_ABORT_UNLESS(old_count > 0);

                if (old_count == 1) {
                    this->attribute = static_cast<KMemoryAttribute>(this->attribute & ~KMemoryAttribute_DeviceShared);
                }

                this->UpdateDeviceDisableMergeStateForUnshareRight(new_perm, left, right);
            }

            constexpr void LockForIpc(KMemoryPermission new_perm, bool left, bool right) {
                /* We must either be locked or have a zero lock count. */
                MESOSPHERE_ASSERT((this->attribute & KMemoryAttribute_IpcLocked) == KMemoryAttribute_IpcLocked || this->ipc_lock_count == 0);

                /* Lock. */
                const u16 new_lock_count = ++this->ipc_lock_count;
                MESOSPHERE_ABORT_UNLESS(new_lock_count > 0);

                /* If this is our first lock, update our permissions. */
                if (new_lock_count == 1) {
                    MESOSPHERE_ASSERT(this->original_perm == KMemoryPermission_None);
                    MESOSPHERE_ASSERT((this->perm | new_perm | KMemoryPermission_NotMapped) == (this->perm | KMemoryPermission_NotMapped));
                    MESOSPHERE_ASSERT((this->perm & KMemoryPermission_UserExecute) != KMemoryPermission_UserExecute || (new_perm == KMemoryPermission_UserRead));
                    this->original_perm = this->perm;
                    this->perm          = static_cast<KMemoryPermission>((new_perm & KMemoryPermission_IpcLockChangeMask) | (this->original_perm & ~KMemoryPermission_IpcLockChangeMask));
                }
                this->attribute = static_cast<KMemoryAttribute>(this->attribute | KMemoryAttribute_IpcLocked);

                if (left) {
                    this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute | KMemoryBlockDisableMergeAttribute_IpcLeft);
                    const u16 new_ipc_disable_merge_count = ++this->ipc_disable_merge_count;
                    MESOSPHERE_ABORT_UNLESS(new_ipc_disable_merge_count > 0);
                }
                MESOSPHERE_UNUSED(right);
            }

            constexpr void UnlockForIpc(KMemoryPermission new_perm, bool left, bool right) {
                /* New permission isn't used. */
                MESOSPHERE_UNUSED(new_perm);

                /* We must be locked. */
                MESOSPHERE_ASSERT((this->attribute & KMemoryAttribute_IpcLocked) == KMemoryAttribute_IpcLocked);

                /* Unlock. */
                const u16 old_lock_count = this->ipc_lock_count--;
                MESOSPHERE_ABORT_UNLESS(old_lock_count > 0);

                /* If this is our last unlock, update our permissions. */
                if (old_lock_count == 1) {
                    MESOSPHERE_ASSERT(this->original_perm != KMemoryPermission_None);
                    this->perm          = this->original_perm;
                    this->original_perm = KMemoryPermission_None;
                    this->attribute = static_cast<KMemoryAttribute>(this->attribute & ~KMemoryAttribute_IpcLocked);
                }

                if (left) {
                    const u16 old_ipc_disable_merge_count = this->ipc_disable_merge_count--;
                    MESOSPHERE_ASSERT(old_ipc_disable_merge_count > 0);
                    if (old_ipc_disable_merge_count == 1) {
                        this->disable_merge_attribute = static_cast<KMemoryBlockDisableMergeAttribute>(this->disable_merge_attribute & ~KMemoryBlockDisableMergeAttribute_IpcLeft);
                    }
                }
                MESOSPHERE_UNUSED(right);
            }

            constexpr KMemoryBlockDisableMergeAttribute GetDisableMergeAttribute() const {
                return this->disable_merge_attribute;
            }
    };
    static_assert(std::is_trivially_destructible<KMemoryBlock>::value);

}
