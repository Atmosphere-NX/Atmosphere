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
#include <mesosphere.hpp>

namespace ams::kern {

    Result KCapabilities::Initialize(const u32 *caps, s32 num_caps, KProcessPageTable *page_table) {
        /* We're initializing an initial process. */
        m_svc_access_flags.Reset();
        m_irq_access_flags.Reset();
        m_debug_capabilities      = {0};
        m_handle_table_size       = 0;
        m_intended_kernel_version = {0};
        m_program_type            = 0;

        /* Initial processes may run on all cores. */
        constexpr u64 VirtMask = cpu::VirtualCoreMask;
        constexpr u64 PhysMask = cpu::ConvertVirtualCoreMaskToPhysical(VirtMask);

        m_core_mask      = VirtMask;
        m_phys_core_mask = PhysMask;

        /* Initial processes may use any user priority they like. */
        m_priority_mask = ~0xFul;

        /* Here, Nintendo sets the kernel version to the current kernel version. */
        /* We will follow suit and set the version to the highest supported kernel version. */
        m_intended_kernel_version.Set<KernelVersion::MajorVersion>(ams::svc::SupportedKernelMajorVersion);
        m_intended_kernel_version.Set<KernelVersion::MinorVersion>(ams::svc::SupportedKernelMinorVersion);

        /* Parse the capabilities array. */
        R_RETURN(this->SetCapabilities(caps, num_caps, page_table));
    }

     Result KCapabilities::Initialize(svc::KUserPointer<const u32 *> user_caps, s32 num_caps, KProcessPageTable *page_table) {
        /* We're initializing a user process. */
        m_svc_access_flags.Reset();
        m_irq_access_flags.Reset();
        m_debug_capabilities      = {0};
        m_handle_table_size       = 0;
        m_intended_kernel_version = {0};
        m_program_type            = 0;

        /* User processes must specify what cores/priorities they can use. */
        m_core_mask     = 0;
        m_priority_mask = 0;

        /* Parse the user capabilities array. */
        R_RETURN(this->SetCapabilities(user_caps, num_caps, page_table));
     }

    Result KCapabilities::SetCorePriorityCapability(const util::BitPack32 cap) {
        /* We can't set core/priority if we've already set them. */
        R_UNLESS(m_core_mask     == 0, svc::ResultInvalidArgument());
        R_UNLESS(m_priority_mask == 0, svc::ResultInvalidArgument());

        /* Validate the core/priority. */
        const auto min_core = cap.Get<CorePriority::MinimumCoreId>();
        const auto max_core = cap.Get<CorePriority::MaximumCoreId>();
        const auto max_prio  = cap.Get<CorePriority::LowestThreadPriority>();
        const auto min_prio  = cap.Get<CorePriority::HighestThreadPriority>();

        R_UNLESS(min_core <= max_core,             svc::ResultInvalidCombination());
        R_UNLESS(min_prio <= max_prio,             svc::ResultInvalidCombination());
        R_UNLESS(max_core <  cpu::NumVirtualCores, svc::ResultInvalidCoreId());

        MESOSPHERE_ASSERT(max_prio < BITSIZEOF(u64));

        /* Set core mask. */
        for (auto core_id = min_core; core_id <= max_core; core_id++) {
            m_core_mask |= (1ul << core_id);
        }
        MESOSPHERE_ASSERT((m_core_mask & cpu::VirtualCoreMask) == m_core_mask);

        /* Set physical core mask. */
        m_phys_core_mask = cpu::ConvertVirtualCoreMaskToPhysical(m_core_mask);

        /* Set priority mask. */
        for (auto prio = min_prio; prio <= max_prio; prio++) {
            m_priority_mask |= (1ul << prio);
        }

        /* We must have some core/priority we can use. */
        R_UNLESS(m_core_mask     != 0, svc::ResultInvalidArgument());
        R_UNLESS(m_priority_mask != 0, svc::ResultInvalidArgument());

        /* Processes must not have access to kernel thread priorities. */
        R_UNLESS((m_priority_mask & 0xF) == 0, svc::ResultInvalidArgument());

        R_SUCCEED();
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

        R_SUCCEED();
    }

    Result KCapabilities::MapRange(const util::BitPack32 cap, const util::BitPack32 size_cap, KProcessPageTable *page_table) {
        /* Get/validate address/size */
        #if defined(MESOSPHERE_ENABLE_LARGE_PHYSICAL_ADDRESS_CAPABILITIES)
        const u64 phys_addr    = static_cast<u64>(cap.Get<MapRange::Address>() | (size_cap.Get<MapRangeSize::AddressHigh>() << MapRange::Address::Count)) * PageSize;
        #else
        const u64 phys_addr    = static_cast<u64>(cap.Get<MapRange::Address>()) * PageSize;

        /* Validate reserved bits are unused. */
        R_UNLESS(size_cap.Get<MapRangeSize::Reserved>() == 0, svc::ResultOutOfRange());
        #endif
        const size_t num_pages = size_cap.Get<MapRangeSize::Pages>();
        const size_t size      = num_pages * PageSize;
        R_UNLESS(phys_addr == GetInteger(KPhysicalAddress(phys_addr)),    svc::ResultInvalidAddress());
        R_UNLESS(num_pages != 0,                                          svc::ResultInvalidSize());
        R_UNLESS(phys_addr < phys_addr + size,                            svc::ResultInvalidAddress());
        R_UNLESS(((phys_addr + size - 1) & ~PhysicalMapAllowedMask) == 0, svc::ResultInvalidAddress());

        /* Do the mapping. */
        const KMemoryPermission perm = cap.Get<MapRange::ReadOnly>() ? KMemoryPermission_UserRead : KMemoryPermission_UserReadWrite;
        if (size_cap.Get<MapRangeSize::Normal>()) {
            R_RETURN(page_table->MapStatic(phys_addr, size, perm));
        } else {
            R_RETURN(page_table->MapIo(phys_addr, size, perm));
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
        R_RETURN(page_table->MapIo(phys_addr, size, KMemoryPermission_UserReadWrite));
    }

    template<typename F>
    ALWAYS_INLINE Result KCapabilities::ProcessMapRegionCapability(const util::BitPack32 cap, F f) {
        /* Define the allowed memory regions. */
        constexpr const KMemoryRegionType MemoryRegions[] = {
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
                case RegionType::NoMapping:
                    break;
                case RegionType::KernelTraceBuffer:
                    /* NOTE: This does not match official, but is used to make pre-processing hbl capabilities in userland unnecessary. */
                    /* If ktrace isn't enabled, allow ktrace to succeed without mapping anything. */
                    if constexpr (!ams::kern::IsKTraceEnabled) {
                        break;
                    }
                case RegionType::OnMemoryBootImage:
                case RegionType::DTB:
                    R_TRY(f(MemoryRegions[static_cast<u32>(type)], perm));
                    break;
                default:
                    R_THROW(svc::ResultNotFound());
            }
        }

        R_SUCCEED();
    }

    Result KCapabilities::MapRegion(const util::BitPack32 cap, KProcessPageTable *page_table) {
        /* Map each region into the process's page table. */
        R_RETURN(ProcessMapRegionCapability(cap, [page_table] ALWAYS_INLINE_LAMBDA (KMemoryRegionType region_type, KMemoryPermission perm) -> Result {
            R_RETURN(page_table->MapRegion(region_type, perm));
        }));
    }

    Result KCapabilities::CheckMapRegion(const util::BitPack32 cap) {
        /* Check that each region has a physical backing store. */
        R_RETURN(ProcessMapRegionCapability(cap, [] ALWAYS_INLINE_LAMBDA (KMemoryRegionType region_type, KMemoryPermission perm) -> Result {
            MESOSPHERE_UNUSED(perm);
            R_UNLESS(KMemoryLayout::GetPhysicalMemoryRegionTree().FindFirstDerived(region_type) != nullptr, svc::ResultOutOfRange());
            R_SUCCEED();
        }));
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

        R_SUCCEED();
    }

    Result KCapabilities::SetProgramTypeCapability(const util::BitPack32 cap) {
        /* Validate. */
        R_UNLESS(cap.Get<ProgramType::Reserved>() == 0, svc::ResultReservedUsed());

        m_program_type = cap.Get<ProgramType::Type>();
        R_SUCCEED();
    }

    Result KCapabilities::SetKernelVersionCapability(const util::BitPack32 cap) {
        /* Ensure we haven't set our version before. */
        R_UNLESS(m_intended_kernel_version.Get<KernelVersion::MajorVersion>() == 0, svc::ResultInvalidArgument());

        /* Set, ensure that we set a valid version. */
        m_intended_kernel_version = cap;
        R_UNLESS(m_intended_kernel_version.Get<KernelVersion::MajorVersion>() != 0, svc::ResultInvalidArgument());

        R_SUCCEED();
    }

    Result KCapabilities::SetHandleTableCapability(const util::BitPack32 cap) {
        /* Validate. */
        R_UNLESS(cap.Get<HandleTable::Reserved>() == 0, svc::ResultReservedUsed());

        m_handle_table_size = cap.Get<HandleTable::Size>();
        R_SUCCEED();
    }

    Result KCapabilities::SetDebugFlagsCapability(const util::BitPack32 cap) {
        /* Validate. */
        R_UNLESS(cap.Get<DebugFlags::Reserved>() == 0, svc::ResultReservedUsed());

        u32 total = 0;
        if (cap.Get<DebugFlags::AllowDebug>()) { ++total; }
        if (cap.Get<DebugFlags::ForceDebugProd>()) { ++total; }
        if (cap.Get<DebugFlags::ForceDebug>()) { ++total; }
        R_UNLESS(total <= 1, svc::ResultInvalidCombination());

        m_debug_capabilities.Set<DebugFlags::AllowDebug>(cap.Get<DebugFlags::AllowDebug>());
        m_debug_capabilities.Set<DebugFlags::ForceDebugProd>(cap.Get<DebugFlags::ForceDebugProd>());
        m_debug_capabilities.Set<DebugFlags::ForceDebug>(cap.Get<DebugFlags::ForceDebug>());
        R_SUCCEED();
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
            case CapabilityType::CorePriority:  R_RETURN(this->SetCorePriorityCapability(cap));
            case CapabilityType::SyscallMask:   R_RETURN(this->SetSyscallMaskCapability(cap, set_svc));
            case CapabilityType::MapIoPage:     R_RETURN(this->MapIoPage(cap, page_table));
            case CapabilityType::MapRegion:     R_RETURN(this->MapRegion(cap, page_table));
            case CapabilityType::InterruptPair: R_RETURN(this->SetInterruptPairCapability(cap));
            case CapabilityType::ProgramType:   R_RETURN(this->SetProgramTypeCapability(cap));
            case CapabilityType::KernelVersion: R_RETURN(this->SetKernelVersionCapability(cap));
            case CapabilityType::HandleTable:   R_RETURN(this->SetHandleTableCapability(cap));
            case CapabilityType::DebugFlags:    R_RETURN(this->SetDebugFlagsCapability(cap));
            default:                            R_THROW(svc::ResultInvalidArgument());
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

        R_SUCCEED();
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

        R_SUCCEED();
    }

    Result KCapabilities::CheckCapabilities(svc::KUserPointer<const u32 *> user_caps, s32 num_caps) {
        for (s32 i = 0; i < num_caps; ++i) {
            /* Read the cap from userspace. */
            u32 cap0;
            R_TRY(user_caps.CopyArrayElementTo(std::addressof(cap0), i));

            /* Check the capability refers to a valid region. */

            const util::BitPack32 cap = { cap0 };
            if (GetCapabilityType(cap) == CapabilityType::MapRegion) {
                R_TRY(CheckMapRegion(cap));
            }
        }

        R_SUCCEED();
    }

}
