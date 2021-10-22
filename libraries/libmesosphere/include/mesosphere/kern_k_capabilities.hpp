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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_thread.hpp>
#include <mesosphere/kern_select_page_table.hpp>
#include <mesosphere/kern_svc.hpp>

namespace ams::kern {

    class KCapabilities {
        private:
            static constexpr size_t InterruptIdCount = 0x400;

            struct InterruptFlagSetTag{};
            using InterruptFlagSet = util::BitFlagSet<InterruptIdCount, InterruptFlagSetTag>;

            enum class CapabilityType : u32 {
                CorePriority  = (1u <<  3) - 1,
                SyscallMask   = (1u <<  4) - 1,
                MapRange      = (1u <<  6) - 1,
                MapIoPage     = (1u <<  7) - 1,
                MapRegion     = (1u << 10) - 1,
                InterruptPair = (1u << 11) - 1,
                ProgramType   = (1u << 13) - 1,
                KernelVersion = (1u << 14) - 1,
                HandleTable   = (1u << 15) - 1,
                DebugFlags    = (1u << 16) - 1,

                Invalid       = 0u,
                Padding       = ~0u,
            };

            using RawCapabilityValue = util::BitPack32::Field<0, BITSIZEOF(util::BitPack32), u32>;

            static constexpr CapabilityType GetCapabilityType(const util::BitPack32 cap) {
                const u32 value = cap.Get<RawCapabilityValue>();
                return static_cast<CapabilityType>((~value & (value + 1)) - 1);
            }

            static constexpr u32 GetCapabilityFlag(CapabilityType type) {
                return static_cast<u32>(type) + 1;
            }

            template<size_t Index, size_t Count, typename T = u32>
            using Field = util::BitPack32::Field<Index, Count, T>;

            #define DEFINE_FIELD(name, prev, ...) using name = Field<prev::Next, __VA_ARGS__>

            template<CapabilityType Type>
            static constexpr inline u32 CapabilityFlag = static_cast<u32>(Type) + 1;

            template<CapabilityType Type>
            static constexpr inline u32 CapabilityId = util::CountTrailingZeros<u32>(CapabilityFlag<Type>);

            struct CorePriority {
                using IdBits = Field<0, CapabilityId<CapabilityType::CorePriority> + 1>;

                DEFINE_FIELD(LowestThreadPriority,  IdBits,                6);
                DEFINE_FIELD(HighestThreadPriority, LowestThreadPriority,  6);
                DEFINE_FIELD(MinimumCoreId,         HighestThreadPriority, 8);
                DEFINE_FIELD(MaximumCoreId,         MinimumCoreId,         8);
            };

            struct SyscallMask {
                using IdBits = Field<0, CapabilityId<CapabilityType::SyscallMask> + 1>;

                DEFINE_FIELD(Mask,  IdBits, 24);
                DEFINE_FIELD(Index, Mask,    3);
            };

            #if defined(MESOSPHERE_ENABLE_LARGE_PHYSICAL_ADDRESS_CAPABILITIES)
            static constexpr u64 PhysicalMapAllowedMask = (1ul << 40) - 1;
            #else
            static constexpr u64 PhysicalMapAllowedMask = (1ul << 36) - 1;
            #endif

            struct MapRange {
                using IdBits = Field<0, CapabilityId<CapabilityType::MapRange> + 1>;

                DEFINE_FIELD(Address,  IdBits,  24);
                DEFINE_FIELD(ReadOnly, Address,  1, bool);
            };

            struct MapRangeSize {
                using IdBits = Field<0, CapabilityId<CapabilityType::MapRange> + 1>;

                DEFINE_FIELD(Pages, IdBits, 20);

                #if defined(MESOSPHERE_ENABLE_LARGE_PHYSICAL_ADDRESS_CAPABILITIES)
                DEFINE_FIELD(AddressHigh, Pages,        4);
                DEFINE_FIELD(Normal,      AddressHigh,  1, bool);
                #else
                DEFINE_FIELD(Reserved, Pages,     4);
                DEFINE_FIELD(Normal,   Reserved,  1, bool);
                #endif
            };

            struct MapIoPage {
                using IdBits = Field<0, CapabilityId<CapabilityType::MapIoPage> + 1>;

                DEFINE_FIELD(Address, IdBits, 24);
            };

            enum class RegionType : u32 {
                None              = 0,
                KernelTraceBuffer = 1,
                OnMemoryBootImage = 2,
                DTB               = 3,
            };

            struct MapRegion {
                using IdBits = Field<0, CapabilityId<CapabilityType::MapRegion> + 1>;

                DEFINE_FIELD(Region0,   IdBits,      6, RegionType);
                DEFINE_FIELD(ReadOnly0, Region0,     1, bool);
                DEFINE_FIELD(Region1,   ReadOnly0,   6, RegionType);
                DEFINE_FIELD(ReadOnly1, Region1,     1, bool);
                DEFINE_FIELD(Region2,   ReadOnly1,   6, RegionType);
                DEFINE_FIELD(ReadOnly2, Region2,     1, bool);
            };

            static const u32 PaddingInterruptId = 0x3FF;
            static_assert(PaddingInterruptId < InterruptIdCount);

            struct InterruptPair {
                using IdBits = Field<0, CapabilityId<CapabilityType::InterruptPair> + 1>;

                DEFINE_FIELD(InterruptId0, IdBits,       10);
                DEFINE_FIELD(InterruptId1, InterruptId0, 10);
            };


            struct ProgramType {
                using IdBits = Field<0, CapabilityId<CapabilityType::ProgramType> + 1>;

                DEFINE_FIELD(Type,     IdBits,  3);
                DEFINE_FIELD(Reserved, Type,   15);
            };

            struct KernelVersion {
                using IdBits = Field<0, CapabilityId<CapabilityType::KernelVersion> + 1>;

                DEFINE_FIELD(MinorVersion, IdBits,        4);
                DEFINE_FIELD(MajorVersion, MinorVersion, 13);
            };

            struct HandleTable {
                using IdBits = Field<0, CapabilityId<CapabilityType::HandleTable> + 1>;

                DEFINE_FIELD(Size,     IdBits, 10);
                DEFINE_FIELD(Reserved, Size,    6);
            };

            struct DebugFlags {
                using IdBits = Field<0, CapabilityId<CapabilityType::DebugFlags> + 1>;

                DEFINE_FIELD(AllowDebug, IdBits,      1, bool);
                DEFINE_FIELD(ForceDebug, AllowDebug,  1, bool);
                DEFINE_FIELD(Reserved,   ForceDebug, 13);
            };

            #undef DEFINE_FIELD

            static constexpr u32 InitializeOnceFlags = CapabilityFlag<CapabilityType::CorePriority>  |
                                                       CapabilityFlag<CapabilityType::ProgramType>   |
                                                       CapabilityFlag<CapabilityType::KernelVersion> |
                                                       CapabilityFlag<CapabilityType::HandleTable>   |
                                                       CapabilityFlag<CapabilityType::DebugFlags>;
        private:
            svc::SvcAccessFlagSet m_svc_access_flags;
            InterruptFlagSet m_irq_access_flags;
            u64 m_core_mask;
            u64 m_priority_mask;
            util::BitPack32 m_debug_capabilities;
            s32 m_handle_table_size;
            util::BitPack32 m_intended_kernel_version;
            u32 m_program_type;
        private:
            constexpr bool SetSvcAllowed(u32 id) {
                if (AMS_LIKELY(id < m_svc_access_flags.GetCount())) {
                    m_svc_access_flags[id] = true;
                    return true;
                } else {
                    return false;
                }
            }

            constexpr bool SetInterruptPermitted(u32 id) {
                if (AMS_LIKELY(id < m_irq_access_flags.GetCount())) {
                    m_irq_access_flags[id] = true;
                    return true;
                } else {
                    return false;
                }
            }

            Result SetCorePriorityCapability(const util::BitPack32 cap);
            Result SetSyscallMaskCapability(const util::BitPack32 cap, u32 &set_svc);
            Result MapRange(const util::BitPack32 cap, const util::BitPack32 size_cap, KProcessPageTable *page_table);
            Result MapIoPage(const util::BitPack32 cap, KProcessPageTable *page_table);
            Result MapRegion(const util::BitPack32 cap, KProcessPageTable *page_table);
            Result SetInterruptPairCapability(const util::BitPack32 cap);
            Result SetProgramTypeCapability(const util::BitPack32 cap);
            Result SetKernelVersionCapability(const util::BitPack32 cap);
            Result SetHandleTableCapability(const util::BitPack32 cap);
            Result SetDebugFlagsCapability(const util::BitPack32 cap);

            Result SetCapability(const util::BitPack32 cap, u32 &set_flags, u32 &set_svc, KProcessPageTable *page_table);
            Result SetCapabilities(const u32 *caps, s32 num_caps, KProcessPageTable *page_table);
            Result SetCapabilities(svc::KUserPointer<const u32 *> user_caps, s32 num_caps, KProcessPageTable *page_table);
        public:
            constexpr explicit KCapabilities(util::ConstantInitializeTag) : m_svc_access_flags{}, m_irq_access_flags{}, m_core_mask{}, m_priority_mask{}, m_debug_capabilities{0}, m_handle_table_size{}, m_intended_kernel_version{}, m_program_type{} { /* ... */ }
            KCapabilities() { /* ... */ }

            Result Initialize(const u32 *caps, s32 num_caps, KProcessPageTable *page_table);
            Result Initialize(svc::KUserPointer<const u32 *> user_caps, s32 num_caps, KProcessPageTable *page_table);

            constexpr u64 GetCoreMask() const { return m_core_mask; }
            constexpr u64 GetPriorityMask() const { return m_priority_mask; }
            constexpr s32 GetHandleTableSize() const { return m_handle_table_size; }

            ALWAYS_INLINE void CopySvcPermissionsTo(KThread::StackParameters &sp) const {
                /* Copy permissions. */
                sp.svc_access_flags = m_svc_access_flags;

                /* Clear specific SVCs based on our state. */
                sp.svc_access_flags[svc::SvcId_ReturnFromException]        = false;
                sp.svc_access_flags[svc::SvcId_SynchronizePreemptionState] = false;
                if (sp.is_pinned) {
                    sp.svc_access_flags[svc::SvcId_GetInfo] = false;
                }
            }

            ALWAYS_INLINE void CopyPinnedSvcPermissionsTo(KThread::StackParameters &sp) const {
                /* Get whether we have access to return from exception. */
                const bool return_from_exception = sp.svc_access_flags[svc::SvcId_ReturnFromException];

                /* Clear all permissions. */
                sp.svc_access_flags.Reset();

                /* Set SynchronizePreemptionState if allowed. */
                if (m_svc_access_flags[svc::SvcId_SynchronizePreemptionState]) {
                    sp.svc_access_flags[svc::SvcId_SynchronizePreemptionState] = true;
                }

                /* If we previously had ReturnFromException, potentially grant it and GetInfo. */
                if (return_from_exception) {
                    /* Set ReturnFromException (guaranteed allowed, if we're here). */
                    sp.svc_access_flags[svc::SvcId_ReturnFromException] = true;

                    /* Set GetInfo if allowed. */
                    if (m_svc_access_flags[svc::SvcId_GetInfo]) {
                        sp.svc_access_flags[svc::SvcId_GetInfo] = true;
                    }
                }
            }

            ALWAYS_INLINE void CopyUnpinnedSvcPermissionsTo(KThread::StackParameters &sp) const {
                /* Get whether we have access to return from exception. */
                const bool return_from_exception = sp.svc_access_flags[svc::SvcId_ReturnFromException];

                /* Copy permissions. */
                sp.svc_access_flags = m_svc_access_flags;

                /* Clear specific SVCs based on our state. */
                sp.svc_access_flags[svc::SvcId_SynchronizePreemptionState] = false;

                if (!return_from_exception) {
                    sp.svc_access_flags[svc::SvcId_ReturnFromException] = false;
                }
            }

            ALWAYS_INLINE void CopyEnterExceptionSvcPermissionsTo(KThread::StackParameters &sp) const {
                /* Set ReturnFromException if allowed. */
                if (m_svc_access_flags[svc::SvcId_ReturnFromException]) {
                    sp.svc_access_flags[svc::SvcId_ReturnFromException] = true;
                }

                /* Set GetInfo if allowed. */
                if (m_svc_access_flags[svc::SvcId_GetInfo]) {
                    sp.svc_access_flags[svc::SvcId_GetInfo] = true;
                }
            }

            ALWAYS_INLINE void CopyLeaveExceptionSvcPermissionsTo(KThread::StackParameters &sp) const {
                /* Clear ReturnFromException. */
                sp.svc_access_flags[svc::SvcId_ReturnFromException] = false;

                /* If pinned, clear GetInfo. */
                if (sp.is_pinned) {
                    sp.svc_access_flags[svc::SvcId_GetInfo] = false;
                }
            }

            constexpr bool IsPermittedInterrupt(u32 id) const {
                return (id < m_irq_access_flags.GetCount()) && m_irq_access_flags[id];
            }

            constexpr bool IsPermittedDebug() const {
                return m_debug_capabilities.Get<DebugFlags::AllowDebug>();
            }

            constexpr bool CanForceDebug() const {
                return m_debug_capabilities.Get<DebugFlags::ForceDebug>();
            }

            constexpr u32 GetIntendedKernelMajorVersion() const { return m_intended_kernel_version.Get<KernelVersion::MajorVersion>(); }
            constexpr u32 GetIntendedKernelMinorVersion() const { return m_intended_kernel_version.Get<KernelVersion::MinorVersion>(); }
            constexpr u32 GetIntendedKernelVersion() const { return ams::svc::EncodeKernelVersion(this->GetIntendedKernelMajorVersion(), this->GetIntendedKernelMinorVersion()); }
    };

}
