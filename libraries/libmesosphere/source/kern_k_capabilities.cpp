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
#include <mesosphere.hpp>

namespace ams::kern {

    Result KCapabilities::Initialize(const u32 *caps, s32 num_caps, KProcessPageTable *page_table) {
        /* We're initializing an initial process. */
        /* Most fields have already been cleared by our constructor. */

        /* Initial processes may run on all cores. */
        this->core_mask = (1ul << cpu::NumCores) - 1;

        /* Initial processes may use any user priority they like. */
        this->priority_mask = ~0xFul;

        /* Here, Nintendo sets the kernel version to the current kernel version. */
        /* We will follow suit and set the version to the highest supported kernel version. */
        this->intended_kernel_version.Set<KernelVersion::MajorVersion>(ams::svc::SupportedKernelMajorVersion);
        this->intended_kernel_version.Set<KernelVersion::MinorVersion>(ams::svc::SupportedKernelMinorVersion);

        /* Parse the capabilities array. */
        return this->SetCapabilities(caps, num_caps, page_table);
    }

     Result KCapabilities::Initialize(svc::KUserPointer<const u32 *> user_caps, s32 num_caps, KProcessPageTable *page_table) {
        /* We're initializing a user process. */
        /* Most fields have already been cleared by our constructor. */

        /* Parse the user capabilities array. */
        return this->SetCapabilities(user_caps, num_caps, page_table);
     }

    Result KCapabilities::SetCorePriorityCapability(const util::BitPack32 cap) {
        /* We can't set core/priority if we've already set them. */
        R_UNLESS(this->core_mask    == 0,  svc::ResultInvalidArgument());
        R_UNLESS(this->priority_mask == 0, svc::ResultInvalidArgument());

        /* Validate the core/priority. */
        const auto min_core = cap.Get<CorePriority::MinimumCoreId>();
        const auto max_core = cap.Get<CorePriority::MaximumCoreId>();
        const auto max_prio  = cap.Get<CorePriority::LowestThreadPriority>();
        const auto min_prio  = cap.Get<CorePriority::HighestThreadPriority>();

        R_UNLESS(min_core <= max_core,       svc::ResultInvalidCombination());
        R_UNLESS(min_prio <= max_prio,       svc::ResultInvalidCombination());
        R_UNLESS(max_core <  cpu::NumCores,  svc::ResultInvalidCoreId());

        MESOSPHERE_ASSERT(max_core < BITSIZEOF(u64));
        MESOSPHERE_ASSERT(max_prio < BITSIZEOF(u64));

        /* Set core mask. */
        for (auto core_id = min_core; core_id <= max_core; core_id++) {
            this->core_mask |= (1ul << core_id);
        }
        MESOSPHERE_ASSERT((this->core_mask & ((1ul << cpu::NumCores) - 1)) == this->core_mask);

        /* Set priority mask. */
        for (auto prio = min_prio; prio <= max_prio; prio++) {
            this->priority_mask |= (1ul << prio);
        }

        /* We must have some core/priority we can use. */
        R_UNLESS(this->core_mask     != 0, svc::ResultInvalidArgument());
        R_UNLESS(this->priority_mask != 0, svc::ResultInvalidArgument());

        return ResultSuccess();
    }

    Result KCapabilities::SetSyscallMaskCapability(const util::BitPack32 cap, u32 &set_svc) {
        /* Validate the index. */
        const auto mask  = cap.Get<SyscallMask::Mask>();
        const auto index = cap.Get<SyscallMask::Index>();

        const u32 index_flag = (1u << index);
        R_UNLESS((set_svc & index_flag) == 0, svc::ResultInvalidCombination());
        set_svc |= index_flag;

        /* Set SVCs. */
        for (size_t i = 0; i < SyscallMask::Mask::Count; i++) {
            const u32 svc_id = SyscallMask::Mask::Count * index + i;
            if (mask & (1u << i)) {
                R_UNLESS(this->SetSvcAllowed(svc_id), svc::ResultOutOfRange());
            }
        }

        return ResultSuccess();
    }

    Result KCapabilities::MapRange(const util::BitPack32 cap, const util::BitPack32 size_cap, KProcessPageTable *page_table) {
        /* Validate reserved bits are unused. */
        R_UNLESS(size_cap.Get<MapRangeSize::Reserved>() == 0, svc::ResultOutOfRange());

        /* Get/validate address/size */
        const u64 phys_addr    = cap.Get<MapRange::Address>() * PageSize;
        const size_t num_pages = size_cap.Get<MapRangeSize::Pages>();
        const size_t size      = num_pages * PageSize;
        R_UNLESS(phys_addr == GetInteger(KPhysicalAddress(phys_addr)),    svc::ResultInvalidAddress());
        R_UNLESS(num_pages != 0,                                          svc::ResultInvalidSize());
        R_UNLESS(phys_addr < phys_addr + size,                            svc::ResultInvalidAddress());
        R_UNLESS(((phys_addr + size - 1) & ~PhysicalMapAllowedMask) == 0, svc::ResultInvalidAddress());

        /* Do the mapping. */
        const KMemoryPermission perm = cap.Get<MapRange::ReadOnly>() ? KMemoryPermission_UserRead : KMemoryPermission_UserReadWrite;
        if (size_cap.Get<MapRangeSize::Normal>()) {
            return page_table->MapStatic(phys_addr, size, perm);
        } else {
            return page_table->MapIo(phys_addr, size, perm);
        }
    }

    Result KCapabilities::MapIoPage(const util::BitPack32 cap, KProcessPageTable *page_table) {
        /* Get/validate address/size */
        const u64 phys_addr    = cap.Get<MapIoPage::Address>() * PageSize;
        const size_t num_pages = 1;
        const size_t size      = num_pages * PageSize;
        R_UNLESS(phys_addr == GetInteger(KPhysicalAddress(phys_addr)),    svc::ResultInvalidAddress());
        R_UNLESS(num_pages != 0,                                          svc::ResultInvalidSize());
        R_UNLESS(phys_addr < phys_addr + size,                            svc::ResultInvalidAddress());
        R_UNLESS(((phys_addr + size - 1) & ~PhysicalMapAllowedMask) == 0, svc::ResultInvalidAddress());

        /* Do the mapping. */
        return page_table->MapIo(phys_addr, size, KMemoryPermission_UserReadWrite);
    }

    Result KCapabilities::MapRegion(const util::BitPack32 cap, KProcessPageTable *page_table) {
        /* Define the allowed memory regions. */
        constexpr KMemoryRegionType MemoryRegions[] = {
            KMemoryRegionType_None,
            KMemoryRegionType_KernelTraceBuffer,
            KMemoryRegionType_OnMemoryBootImage,
            KMemoryRegionType_DTB,
        };

        /* Extract regions/read only. */
        const RegionType types[3] = { cap.Get<MapRegion::Region0>(),   cap.Get<MapRegion::Region1>(),   cap.Get<MapRegion::Region2>(), };
        const bool          ro[3] = { cap.Get<MapRegion::ReadOnly0>(), cap.Get<MapRegion::ReadOnly1>(), cap.Get<MapRegion::ReadOnly2>(), };

        for (size_t i = 0; i < util::size(types); i++) {
            const auto type = types[i];
            const auto perm = ro[i] ? KMemoryPermission_UserRead : KMemoryPermission_UserReadWrite;
            switch (type) {
                case RegionType::None:
                    break;
                case RegionType::KernelTraceBuffer:
                case RegionType::OnMemoryBootImage:
                case RegionType::DTB:
                    R_TRY(page_table->MapRegion(MemoryRegions[static_cast<u32>(type)], perm));
                    break;
                default:
                    return svc::ResultNotFound();
            }
        }

        return ResultSuccess();
    }

    Result KCapabilities::SetInterruptPairCapability(const util::BitPack32 cap) {
        /* Extract interrupts. */
        const u32 ids[2] = { cap.Get<InterruptPair::InterruptId0>(), cap.Get<InterruptPair::InterruptId1>(), };

        for (size_t i = 0; i < util::size(ids); i++) {
            if (ids[i] != PaddingInterruptId) {
                R_UNLESS(Kernel::GetInterruptManager().IsInterruptDefined(ids[i]), svc::ResultOutOfRange());
                R_UNLESS(this->SetInterruptPermitted(ids[i]),                      svc::ResultOutOfRange());
            }
        }

        return ResultSuccess();
    }

    Result KCapabilities::SetProgramTypeCapability(const util::BitPack32 cap) {
        /* Validate. */
        R_UNLESS(cap.Get<ProgramType::Reserved>() == 0, svc::ResultReservedUsed());

        this->program_type = cap.Get<ProgramType::Type>();
        return ResultSuccess();
    }

    Result KCapabilities::SetKernelVersionCapability(const util::BitPack32 cap) {
        /* Ensure we haven't set our version before. */
        R_UNLESS(this->intended_kernel_version.Get<KernelVersion::MajorVersion>() == 0, svc::ResultInvalidArgument());

        /* Set, ensure that we set a valid version. */
        this->intended_kernel_version = cap;
        R_UNLESS(this->intended_kernel_version.Get<KernelVersion::MajorVersion>() != 0, svc::ResultInvalidArgument());

        return ResultSuccess();
    }

    Result KCapabilities::SetHandleTableCapability(const util::BitPack32 cap) {
        /* Validate. */
        R_UNLESS(cap.Get<HandleTable::Reserved>() == 0, svc::ResultReservedUsed());

        this->handle_table_size = cap.Get<HandleTable::Size>();
        return ResultSuccess();
    }

    Result KCapabilities::SetDebugFlagsCapability(const util::BitPack32 cap) {
        /* Validate. */
        R_UNLESS(cap.Get<DebugFlags::Reserved>() == 0, svc::ResultReservedUsed());

        this->debug_capabilities.Set<DebugFlags::AllowDebug>(cap.Get<DebugFlags::AllowDebug>());
        this->debug_capabilities.Set<DebugFlags::ForceDebug>(cap.Get<DebugFlags::ForceDebug>());
        return ResultSuccess();
    }

    Result KCapabilities::SetCapability(const util::BitPack32 cap, u32 &set_flags, u32 &set_svc, KProcessPageTable *page_table) {
        /* Validate this is a capability we can act on. */
        const auto type = GetCapabilityType(cap);
        R_UNLESS(type != CapabilityType::Invalid, svc::ResultInvalidArgument());

        /* If the type is padding, we have no work to do. */
        R_SUCCEED_IF(type == CapabilityType::Padding);

        /* Check that we haven't already processed this capability. */
        const auto flag = GetCapabilityFlag(type);
        R_UNLESS(((set_flags & InitializeOnceFlags) & flag) == 0, svc::ResultInvalidCombination());
        set_flags |= flag;

        /* Process the capability. */
        switch (type) {
            case CapabilityType::CorePriority:  return this->SetCorePriorityCapability(cap);
            case CapabilityType::SyscallMask:   return this->SetSyscallMaskCapability(cap, set_svc);
            case CapabilityType::MapIoPage:     return this->MapIoPage(cap, page_table);
            case CapabilityType::MapRegion:     return this->MapRegion(cap, page_table);
            case CapabilityType::InterruptPair: return this->SetInterruptPairCapability(cap);
            case CapabilityType::ProgramType:   return this->SetProgramTypeCapability(cap);
            case CapabilityType::KernelVersion: return this->SetKernelVersionCapability(cap);
            case CapabilityType::HandleTable:   return this->SetHandleTableCapability(cap);
            case CapabilityType::DebugFlags:    return this->SetDebugFlagsCapability(cap);
            default:                            return svc::ResultInvalidArgument();
        }
    }

    Result KCapabilities::SetCapabilities(const u32 *caps, s32 num_caps, KProcessPageTable *page_table) {
        u32 set_flags = 0, set_svc = 0;

        for (s32 i = 0; i < num_caps; i++) {
            const util::BitPack32 cap = { caps[i] };
            if (GetCapabilityType(cap) == CapabilityType::MapRange) {
                /* Check that the pair cap exists. */
                R_UNLESS((++i) < num_caps, svc::ResultInvalidCombination());

                /* Check the pair cap is a map range cap. */
                const util::BitPack32 size_cap = { caps[i] };
                R_UNLESS(GetCapabilityType(size_cap) == CapabilityType::MapRange, svc::ResultInvalidCombination());

                /* Map the range. */
                R_TRY(this->MapRange(cap, size_cap, page_table));
            } else {
                R_TRY(this->SetCapability(cap, set_flags, set_svc, page_table));
            }
        }

        return ResultSuccess();
    }

    Result KCapabilities::SetCapabilities(svc::KUserPointer<const u32 *> user_caps, s32 num_caps, KProcessPageTable *page_table) {
        u32 set_flags = 0, set_svc = 0;

        for (s32 i = 0; i < num_caps; i++) {
            /* Read the cap from userspace. */
            u32 cap0;
            R_TRY(user_caps.CopyArrayElementTo(std::addressof(cap0), i));

            const util::BitPack32 cap = { cap0 };
            if (GetCapabilityType(cap) == CapabilityType::MapRange) {
                /* Check that the pair cap exists. */
                R_UNLESS((++i) < num_caps, svc::ResultInvalidCombination());

                /* Read the second cap from userspace. */
                u32 cap1;
                R_TRY(user_caps.CopyArrayElementTo(std::addressof(cap1), i));

                /* Check the pair cap is a map range cap. */
                const util::BitPack32 size_cap = { cap1 };
                R_UNLESS(GetCapabilityType(size_cap) == CapabilityType::MapRange, svc::ResultInvalidCombination());

                /* Map the range. */
                R_TRY(this->MapRange(cap, size_cap, page_table));
            } else {
                R_TRY(this->SetCapability(cap, set_flags, set_svc, page_table));
            }
        }

        return ResultSuccess();
    }

}
