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

namespace ams::kern {

    enum KMemoryRegionType : u32 {};

    enum KMemoryRegionAttr : typename std::underlying_type<KMemoryRegionType>::type {
        KMemoryRegionAttr_CarveoutProtected = 0x04000000,
        KMemoryRegionAttr_DidKernelMap      = 0x08000000,
        KMemoryRegionAttr_ShouldKernelMap   = 0x10000000,
        KMemoryRegionAttr_UserReadOnly      = 0x20000000,
        KMemoryRegionAttr_NoUserMap         = 0x40000000,
        KMemoryRegionAttr_LinearMapped      = 0x80000000,
    };

    namespace impl {

        consteval size_t BitsForDeriveSparse(size_t n) {
            return n + 1;
        }

        consteval size_t BitsForDeriveDense(size_t n) {
            AMS_ASSUME(n > 0);

            size_t low = 0, high = 1;
            for (size_t i = 0; i < n - 1; ++i) {
                if ((++low) == high) {
                    ++high;
                    low = 0;
                }
            }
            return high + 1;
        }

        class KMemoryRegionTypeValue {
            private:
                using ValueType = typename std::underlying_type<KMemoryRegionType>::type;
            private:
                ValueType m_value;
                size_t m_next_bit;
                bool m_finalized;
                bool m_sparse_only;
                bool m_dense_only;
            private:
                consteval KMemoryRegionTypeValue(ValueType v) : m_value(v), m_next_bit(0), m_finalized(false), m_sparse_only(false), m_dense_only(false) { /* ... */ }
            public:
                consteval KMemoryRegionTypeValue() : KMemoryRegionTypeValue(0) { /* ... */ }

                consteval operator KMemoryRegionType() const { return static_cast<KMemoryRegionType>(m_value); }
                consteval ValueType GetValue() const { return m_value; }

                consteval const KMemoryRegionTypeValue &Finalize()      { m_finalized = true;   return *this; }
                consteval const KMemoryRegionTypeValue &SetSparseOnly() { m_sparse_only = true; return *this; }
                consteval const KMemoryRegionTypeValue &SetDenseOnly()  { m_dense_only = true; return *this; }

                consteval KMemoryRegionTypeValue &SetAttribute(KMemoryRegionAttr attr) { AMS_ASSUME(!m_finalized); m_value |= attr; return *this; }

                consteval KMemoryRegionTypeValue DeriveInitial(size_t i, size_t next = BITSIZEOF(ValueType)) const {
                    AMS_ASSUME(!m_finalized);
                    AMS_ASSUME(!m_value);
                    AMS_ASSUME(!m_next_bit);
                    AMS_ASSUME(next > i);

                    KMemoryRegionTypeValue new_type = *this;
                    new_type.m_value    = (ValueType{1} << i);
                    new_type.m_next_bit = next;
                    return new_type;
                }

                consteval KMemoryRegionTypeValue DeriveAttribute(KMemoryRegionAttr attr) const {
                    AMS_ASSUME(!m_finalized);

                    KMemoryRegionTypeValue new_type = *this;
                    new_type.m_value |= attr;
                    return new_type;
                }

                consteval KMemoryRegionTypeValue DeriveTransition(size_t ofs = 0, size_t adv = 1) const {
                    AMS_ASSUME(!m_finalized);
                    AMS_ASSUME(ofs < adv);
                    AMS_ASSUME(m_next_bit + adv <= BITSIZEOF(ValueType));

                    KMemoryRegionTypeValue new_type = *this;
                    new_type.m_value |= (ValueType{1} << (m_next_bit + ofs));
                    new_type.m_next_bit += adv;
                    return new_type;
                }

                consteval KMemoryRegionTypeValue DeriveSparse(size_t ofs, size_t n, size_t i) const {
                    AMS_ASSUME(!m_finalized);
                    AMS_ASSUME(!m_dense_only);
                    AMS_ASSUME(m_next_bit + ofs + n + 1 <= BITSIZEOF(ValueType));
                    AMS_ASSUME(i < n);

                    KMemoryRegionTypeValue new_type = *this;
                    new_type.m_value |= (ValueType{1} << (m_next_bit + ofs));
                    new_type.m_value |= (ValueType{1} << (m_next_bit + ofs + 1 + i));
                    new_type.m_next_bit += ofs + n + 1;
                    return new_type;
                }

                consteval KMemoryRegionTypeValue Derive(size_t n, size_t i) const {
                    AMS_ASSUME(!m_finalized);
                    AMS_ASSUME(!m_sparse_only);
                    AMS_ASSUME(m_next_bit + BitsForDeriveDense(n) <= BITSIZEOF(ValueType));
                    AMS_ASSUME(i < n);

                    size_t low = 0, high = 1;
                    for (size_t j = 0; j < i; ++j) {
                        if ((++low) == high) {
                            ++high;
                            low = 0;
                        }
                    }
                    AMS_ASSUME(high < BitsForDeriveDense(n));


                    KMemoryRegionTypeValue new_type = *this;
                    new_type.m_value |= (ValueType{1} << (m_next_bit + low));
                    new_type.m_value |= (ValueType{1} << (m_next_bit + high));
                    new_type.m_next_bit += BitsForDeriveDense(n);
                    return new_type;
                }

                consteval KMemoryRegionTypeValue Advance(size_t n) const {
                    AMS_ASSUME(!m_finalized);
                    AMS_ASSUME(m_next_bit + n <= BITSIZEOF(ValueType));

                    KMemoryRegionTypeValue new_type = *this;
                    new_type.m_next_bit += n;
                    return new_type;
                }

                constexpr ALWAYS_INLINE bool IsAncestorOf(ValueType v) const {
                    return (m_value | v) == v;
                }
        };

    }

    constexpr inline const auto KMemoryRegionType_None = impl::KMemoryRegionTypeValue();

    constexpr inline const auto KMemoryRegionType_Kernel = KMemoryRegionType_None.DeriveInitial(0, 2);
    constexpr inline const auto KMemoryRegionType_Dram   = KMemoryRegionType_None.DeriveInitial(1, 2);
    static_assert(KMemoryRegionType_Kernel.GetValue() == 0x1);
    static_assert(KMemoryRegionType_Dram  .GetValue() == 0x2);

    /* constexpr inline const auto KMemoryRegionType_CoreLocalRegion = KMemoryRegionType_None.DeriveInitial(2).Finalize(); */
    /* static_assert(KMemoryRegionType_CoreLocalRegion.GetValue() == 0x4); */

    constexpr inline const auto KMemoryRegionType_DramKernelBase   = KMemoryRegionType_Dram.DeriveSparse(0, 3, 0).SetAttribute(KMemoryRegionAttr_NoUserMap).SetAttribute(KMemoryRegionAttr_CarveoutProtected);
    constexpr inline const auto KMemoryRegionType_DramReservedBase = KMemoryRegionType_Dram.DeriveSparse(0, 3, 1);
    constexpr inline const auto KMemoryRegionType_DramHeapBase     = KMemoryRegionType_Dram.DeriveSparse(0, 3, 2).SetAttribute(KMemoryRegionAttr_LinearMapped);
    static_assert(KMemoryRegionType_DramKernelBase  .GetValue() == (0xE  | KMemoryRegionAttr_CarveoutProtected | KMemoryRegionAttr_NoUserMap));
    static_assert(KMemoryRegionType_DramReservedBase.GetValue() == (0x16));
    static_assert(KMemoryRegionType_DramHeapBase    .GetValue() == (0x26 | KMemoryRegionAttr_LinearMapped));

    constexpr inline const auto KMemoryRegionType_DramKernelCode   = KMemoryRegionType_DramKernelBase.DeriveSparse(0, 4, 0);
    constexpr inline const auto KMemoryRegionType_DramKernelSlab   = KMemoryRegionType_DramKernelBase.DeriveSparse(0, 4, 1);
    constexpr inline const auto KMemoryRegionType_DramKernelPtHeap = KMemoryRegionType_DramKernelBase.DeriveSparse(0, 4, 2).SetAttribute(KMemoryRegionAttr_LinearMapped);
    constexpr inline const auto KMemoryRegionType_DramKernelInitPt = KMemoryRegionType_DramKernelBase.DeriveSparse(0, 4, 3).SetAttribute(KMemoryRegionAttr_LinearMapped);
    static_assert(KMemoryRegionType_DramKernelCode  .GetValue() == (0xCE  | KMemoryRegionAttr_CarveoutProtected | KMemoryRegionAttr_NoUserMap));
    static_assert(KMemoryRegionType_DramKernelSlab  .GetValue() == (0x14E | KMemoryRegionAttr_CarveoutProtected | KMemoryRegionAttr_NoUserMap));
    static_assert(KMemoryRegionType_DramKernelPtHeap.GetValue() == (0x24E | KMemoryRegionAttr_CarveoutProtected | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped));
    static_assert(KMemoryRegionType_DramKernelInitPt.GetValue() == (0x44E | KMemoryRegionAttr_CarveoutProtected | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_LinearMapped));


    constexpr inline const auto KMemoryRegionType_DramReservedEarly = KMemoryRegionType_DramReservedBase.DeriveAttribute(KMemoryRegionAttr_NoUserMap);
    static_assert(KMemoryRegionType_DramReservedEarly.GetValue() == (0x16 | KMemoryRegionAttr_NoUserMap));

    constexpr inline const auto KMemoryRegionType_KernelTraceBuffer = KMemoryRegionType_DramReservedBase.DeriveSparse(0, 3, 0).SetAttribute(KMemoryRegionAttr_LinearMapped).SetAttribute(KMemoryRegionAttr_UserReadOnly);
    constexpr inline const auto KMemoryRegionType_OnMemoryBootImage = KMemoryRegionType_DramReservedBase.DeriveSparse(0, 3, 1);
    constexpr inline const auto KMemoryRegionType_DTB               = KMemoryRegionType_DramReservedBase.DeriveSparse(0, 3, 2);
    static_assert(KMemoryRegionType_KernelTraceBuffer.GetValue() == (0xD6 | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_UserReadOnly));
    static_assert(KMemoryRegionType_OnMemoryBootImage.GetValue() == 0x156);
    static_assert(KMemoryRegionType_DTB.GetValue() == 0x256);


    constexpr inline const auto KMemoryRegionType_DramPoolPartition = KMemoryRegionType_DramHeapBase.DeriveAttribute(KMemoryRegionAttr_NoUserMap);
    static_assert(KMemoryRegionType_DramPoolPartition.GetValue() == (0x26 | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_NoUserMap));

    constexpr inline const auto KMemoryRegionType_DramPoolManagement = KMemoryRegionType_DramPoolPartition.DeriveTransition(0, 2).DeriveTransition().SetAttribute(KMemoryRegionAttr_CarveoutProtected);
    constexpr inline const auto KMemoryRegionType_DramUserPool       = KMemoryRegionType_DramPoolPartition.DeriveTransition(1, 2).DeriveTransition();
    static_assert(KMemoryRegionType_DramPoolManagement.GetValue() == (0x166 | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_CarveoutProtected));
    static_assert(KMemoryRegionType_DramUserPool.GetValue()       == (0x1A6 | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_NoUserMap));

    constexpr inline const auto KMemoryRegionType_DramApplicationPool     = KMemoryRegionType_DramUserPool.Derive(4, 0);
    constexpr inline const auto KMemoryRegionType_DramAppletPool          = KMemoryRegionType_DramUserPool.Derive(4, 1);
    constexpr inline const auto KMemoryRegionType_DramSystemNonSecurePool = KMemoryRegionType_DramUserPool.Derive(4, 2);
    constexpr inline const auto KMemoryRegionType_DramSystemPool          = KMemoryRegionType_DramUserPool.Derive(4, 3).SetAttribute(KMemoryRegionAttr_CarveoutProtected);
    static_assert(KMemoryRegionType_DramApplicationPool    .GetValue()   == (0x7A6  | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_NoUserMap));
    static_assert(KMemoryRegionType_DramAppletPool         .GetValue()   == (0xBA6  | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_NoUserMap));
    static_assert(KMemoryRegionType_DramSystemNonSecurePool.GetValue()   == (0xDA6  | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_NoUserMap));
    static_assert(KMemoryRegionType_DramSystemPool         .GetValue()   == (0x13A6 | KMemoryRegionAttr_LinearMapped | KMemoryRegionAttr_NoUserMap | KMemoryRegionAttr_CarveoutProtected));

    constexpr inline const auto KMemoryRegionType_VirtualDramHeapBase          = KMemoryRegionType_Dram.DeriveSparse(1, 3, 0);
    constexpr inline const auto KMemoryRegionType_VirtualDramKernelPtHeap      = KMemoryRegionType_Dram.DeriveSparse(1, 3, 1);
    constexpr inline const auto KMemoryRegionType_VirtualDramKernelTraceBuffer = KMemoryRegionType_Dram.DeriveSparse(1, 3, 2);
    static_assert(KMemoryRegionType_VirtualDramHeapBase         .GetValue() == 0x1A);
    static_assert(KMemoryRegionType_VirtualDramKernelPtHeap     .GetValue() == 0x2A);
    static_assert(KMemoryRegionType_VirtualDramKernelTraceBuffer.GetValue() == 0x4A);

    constexpr inline const auto KMemoryRegionType_VirtualDramKernelInitPt   = KMemoryRegionType_VirtualDramHeapBase.Derive(3, 0);
    constexpr inline const auto KMemoryRegionType_VirtualDramPoolManagement = KMemoryRegionType_VirtualDramHeapBase.Derive(3, 1);
    constexpr inline const auto KMemoryRegionType_VirtualDramUserPool       = KMemoryRegionType_VirtualDramHeapBase.Derive(3, 2);
    static_assert(KMemoryRegionType_VirtualDramKernelInitPt  .GetValue() == 0x19A);
    static_assert(KMemoryRegionType_VirtualDramPoolManagement.GetValue() == 0x29A);
    static_assert(KMemoryRegionType_VirtualDramUserPool      .GetValue() == 0x31A);

    /* NOTE: For unknown reason, the pools are derived out-of-order here. */
    /* It's worth eventually trying to understand why Nintendo made this choice. */
                                                                                                             /* UNUSED: .Derive(6, 0); */
                                                                                                             /* UNUSED: .Derive(6, 1); */
    constexpr inline const auto KMemoryRegionType_VirtualDramAppletPool          = KMemoryRegionType_VirtualDramUserPool.Derive(6, 2);
    constexpr inline const auto KMemoryRegionType_VirtualDramApplicationPool     = KMemoryRegionType_VirtualDramUserPool.Derive(6, 3);
    constexpr inline const auto KMemoryRegionType_VirtualDramSystemNonSecurePool = KMemoryRegionType_VirtualDramUserPool.Derive(6, 4);
    constexpr inline const auto KMemoryRegionType_VirtualDramSystemPool          = KMemoryRegionType_VirtualDramUserPool.Derive(6, 5);
    static_assert(KMemoryRegionType_VirtualDramAppletPool         .GetValue()   == 0x1B1A);
    static_assert(KMemoryRegionType_VirtualDramApplicationPool    .GetValue()   == 0x271A);
    static_assert(KMemoryRegionType_VirtualDramSystemNonSecurePool.GetValue()   == 0x2B1A);
    static_assert(KMemoryRegionType_VirtualDramSystemPool         .GetValue()   == 0x331A);

    constexpr inline const auto KMemoryRegionType_ArchDeviceBase  = KMemoryRegionType_Kernel.DeriveTransition(0, 1).SetSparseOnly();
    constexpr inline const auto KMemoryRegionType_BoardDeviceBase = KMemoryRegionType_Kernel.DeriveTransition(0, 2).SetDenseOnly();
    static_assert(KMemoryRegionType_ArchDeviceBase .GetValue()   == 0x5);
    static_assert(KMemoryRegionType_BoardDeviceBase.GetValue()   == 0x5);

    #if   defined(ATMOSPHERE_ARCH_ARM64)
        #include <mesosphere/arch/arm64/kern_k_memory_region_device_types.inc>
    #elif defined(ATMOSPHERE_ARCH_ARM)
        #include <mesosphere/arch/arm/kern_k_memory_region_device_types.inc>
    #else
        /* Default to no architecture devices. */
        constexpr inline const auto NumArchitectureDeviceRegions = 0;
    #endif
    static_assert(NumArchitectureDeviceRegions >= 0);

    #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)
        #include <mesosphere/board/nintendo/nx/kern_k_memory_region_device_types.inc>
    #else
        /* Default to no board devices. */
        constexpr inline const auto NumBoardDeviceRegions = 0;
    #endif
    static_assert(NumBoardDeviceRegions >= 0);

    constexpr inline const auto KMemoryRegionType_KernelCode  = KMemoryRegionType_Kernel.DeriveSparse(1, 4, 0);
    constexpr inline const auto KMemoryRegionType_KernelStack = KMemoryRegionType_Kernel.DeriveSparse(1, 4, 1);
    constexpr inline const auto KMemoryRegionType_KernelMisc  = KMemoryRegionType_Kernel.DeriveSparse(1, 4, 2);
    constexpr inline const auto KMemoryRegionType_KernelSlab  = KMemoryRegionType_Kernel.DeriveSparse(1, 4, 3);
    static_assert(KMemoryRegionType_KernelCode .GetValue() == 0x19);
    static_assert(KMemoryRegionType_KernelStack.GetValue() == 0x29);
    static_assert(KMemoryRegionType_KernelMisc .GetValue() == 0x49);
    static_assert(KMemoryRegionType_KernelSlab .GetValue() == 0x89);

    constexpr inline const auto KMemoryRegionType_KernelMiscDerivedBase = KMemoryRegionType_KernelMisc.DeriveTransition();
    static_assert(KMemoryRegionType_KernelMiscDerivedBase.GetValue() == 0x149);

                                                                                                         /* UNUSED: .Derive(7, 0); */
    constexpr inline const auto KMemoryRegionType_KernelMiscMainStack      = KMemoryRegionType_KernelMiscDerivedBase.Derive(7, 1);
    constexpr inline const auto KMemoryRegionType_KernelMiscMappedDevice   = KMemoryRegionType_KernelMiscDerivedBase.Derive(7, 2);
    constexpr inline const auto KMemoryRegionType_KernelMiscExceptionStack = KMemoryRegionType_KernelMiscDerivedBase.Derive(7, 3);
    constexpr inline const auto KMemoryRegionType_KernelMiscUnknownDebug   = KMemoryRegionType_KernelMiscDerivedBase.Derive(7, 4);
                                                                                                         /* UNUSED: .Derive(7, 5); */
    constexpr inline const auto KMemoryRegionType_KernelMiscIdleStack = KMemoryRegionType_KernelMiscDerivedBase.Derive(7, 6);
    static_assert(KMemoryRegionType_KernelMiscMainStack     .GetValue() == 0xB49);
    static_assert(KMemoryRegionType_KernelMiscMappedDevice  .GetValue() == 0xD49);
    static_assert(KMemoryRegionType_KernelMiscExceptionStack.GetValue() == 0x1349);
    static_assert(KMemoryRegionType_KernelMiscUnknownDebug  .GetValue() == 0x1549);
    static_assert(KMemoryRegionType_KernelMiscIdleStack     .GetValue() == 0x2349);

    constexpr inline const auto KMemoryRegionType_KernelTemp = KMemoryRegionType_Kernel.Advance(2).Derive(2, 0);
    static_assert(KMemoryRegionType_KernelTemp.GetValue() == 0x31);

    constexpr ALWAYS_INLINE KMemoryRegionType GetTypeForVirtualLinearMapping(u32 type_id) {
        if (KMemoryRegionType_KernelTraceBuffer.IsAncestorOf(type_id)) {
            return KMemoryRegionType_VirtualDramKernelTraceBuffer;
        } else if (KMemoryRegionType_DramKernelPtHeap.IsAncestorOf(type_id)) {
            return KMemoryRegionType_VirtualDramKernelPtHeap;
        } else {
            return KMemoryRegionType_Dram;
        }
    }

}
