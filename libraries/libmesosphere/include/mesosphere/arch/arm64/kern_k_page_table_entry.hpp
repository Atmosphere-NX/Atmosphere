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
#include <mesosphere/kern_select_cpu.hpp>
#include <mesosphere/kern_k_typed_address.hpp>

namespace ams::kern::arch::arm64 {

    constexpr size_t BlocksPerContiguousBlock = 0x10;
    constexpr size_t BlocksPerTable           = PageSize / sizeof(u64);

    constexpr size_t L1BlockSize           = 1_GB;
    constexpr size_t L1ContiguousBlockSize = BlocksPerContiguousBlock * L1BlockSize;
    constexpr size_t L2BlockSize           = 2_MB;
    constexpr size_t L2ContiguousBlockSize = BlocksPerContiguousBlock * L2BlockSize;
    constexpr size_t L3BlockSize           = PageSize;
    constexpr size_t L3ContiguousBlockSize = BlocksPerContiguousBlock * L3BlockSize;

    class PageTableEntry {
        public:
            struct InvalidTag{};
            struct TableTag{};
            struct BlockTag{};
            struct SeparateContiguousTag{};

            enum Permission : u64 {
                Permission_KernelRWX = ((0ul << 53) | (1ul << 54) | (0ul << 6)),
                Permission_KernelRX  = ((0ul << 53) | (1ul << 54) | (2ul << 6)),
                Permission_KernelR   = ((1ul << 53) | (1ul << 54) | (2ul << 6)),
                Permission_KernelRW  = ((1ul << 53) | (1ul << 54) | (0ul << 6)),

                Permission_UserRX    = ((1ul << 53) | (0ul << 54) | (3ul << 6)),
                Permission_UserR     = ((1ul << 53) | (1ul << 54) | (3ul << 6)),
                Permission_UserRW    = ((1ul << 53) | (1ul << 54) | (1ul << 6)),
            };

            enum Shareable : u64 {
                Shareable_NonShareable   = (0 << 8),
                Shareable_OuterShareable = (2 << 8),
                Shareable_InnerShareable = (3 << 8),
            };

            /* Official attributes are: */
            /*    0x00, 0x04, 0xFF, 0x44. 4-7 are unused. */
            enum PageAttribute : u64 {
                PageAttribute_Device_nGnRnE            = (0 << 2),
                PageAttribute_Device_nGnRE             = (1 << 2),
                PageAttribute_NormalMemory             = (2 << 2),
                PageAttribute_NormalMemoryNotCacheable = (3 << 2),
            };

            enum AccessFlag : u64 {
                AccessFlag_NotAccessed = (0 << 10),
                AccessFlag_Accessed    = (1 << 10),
            };

            enum MappingFlag : u64 {
                MappingFlag_NotMapped = (0 << 0),
                MappingFlag_Mapped    = (1 << 0),
            };

            enum SoftwareReservedBit : u8 {
                SoftwareReservedBit_None                    = 0,
                SoftwareReservedBit_DisableMergeHead        = (1u << 0),
                SoftwareReservedBit_DisableMergeHeadAndBody = (1u << 1),
                SoftwareReservedBit_DisableMergeHeadTail    = (1u << 2),
                SoftwareReservedBit_Valid                   = (1u << 3),
            };

            static constexpr ALWAYS_INLINE std::underlying_type<SoftwareReservedBit>::type EncodeSoftwareReservedBits(bool head, bool head_body, bool tail) {
                return (head ? SoftwareReservedBit_DisableMergeHead : SoftwareReservedBit_None) | (head_body ? SoftwareReservedBit_DisableMergeHeadAndBody : SoftwareReservedBit_None) | (tail ? SoftwareReservedBit_DisableMergeHeadTail : SoftwareReservedBit_None);
            }

            enum ExtensionFlag : u64 {
                ExtensionFlag_DisableMergeHead        = (static_cast<u64>(SoftwareReservedBit_DisableMergeHead)        << 55),
                ExtensionFlag_DisableMergeHeadAndBody = (static_cast<u64>(SoftwareReservedBit_DisableMergeHeadAndBody) << 55),
                ExtensionFlag_DisableMergeTail        = (static_cast<u64>(SoftwareReservedBit_DisableMergeHeadTail)    << 55),
                ExtensionFlag_Valid                   = (static_cast<u64>(SoftwareReservedBit_Valid)                   << 55),

                ExtensionFlag_ValidAndMapped = (ExtensionFlag_Valid | MappingFlag_Mapped),
                ExtensionFlag_TestTableMask  = (ExtensionFlag_Valid | (1ul << 1)),
            };

            enum Type : u64 {
                Type_None    = 0x0,
                Type_L1Block = ExtensionFlag_Valid,
                Type_L1Table = 0x2,
                Type_L2Block = ExtensionFlag_Valid,
                Type_L2Table = 0x2,
                Type_L3Block = ExtensionFlag_TestTableMask,
            };

            enum ContigType : u64 {
                ContigType_NotContiguous = (0x0ul << 52),
                ContigType_Contiguous    = (0x1ul << 52),
            };
        protected:
            u64 m_attributes;
        public:
            /* Take in a raw attribute. */
            constexpr explicit ALWAYS_INLINE PageTableEntry() : m_attributes() { /* ... */ }
            constexpr explicit ALWAYS_INLINE PageTableEntry(u64 attr) : m_attributes(attr) { /* ... */ }

            constexpr explicit ALWAYS_INLINE PageTableEntry(InvalidTag) : m_attributes(0) { /* ... */ }

            /* Extend a previous attribute. */
            constexpr explicit ALWAYS_INLINE PageTableEntry(const PageTableEntry &rhs, u64 new_attr) : m_attributes(rhs.m_attributes | new_attr) { /* ... */ }

            /* Construct a new attribute. */
            constexpr explicit ALWAYS_INLINE PageTableEntry(Permission perm, PageAttribute p_a, Shareable share, MappingFlag m)
                : m_attributes(static_cast<u64>(perm) | static_cast<u64>(AccessFlag_Accessed) | static_cast<u64>(p_a) | static_cast<u64>(share) | static_cast<u64>(m))
            {
                /* ... */
            }

            /* Construct a table. */
            constexpr explicit ALWAYS_INLINE PageTableEntry(TableTag, KPhysicalAddress phys_addr, bool is_kernel, bool pxn, size_t ref_count)
                : PageTableEntry(((is_kernel ? 0x3ul : 0) << 60) | (static_cast<u64>(pxn) << 59) | GetInteger(phys_addr) | (ref_count << 2) | 0x3)
            {
                /* ... */
            }

            /* Construct a block. */
            constexpr explicit ALWAYS_INLINE PageTableEntry(BlockTag, KPhysicalAddress phys_addr, const PageTableEntry &attr, u8 sw_reserved_bits, bool contig, bool page)
                : PageTableEntry(attr, (static_cast<u64>(sw_reserved_bits) << 55) | (static_cast<u64>(contig) << 52) | GetInteger(phys_addr) | (page ? ExtensionFlag_TestTableMask : ExtensionFlag_Valid))
            {
                /* ... */
            }
            constexpr explicit ALWAYS_INLINE PageTableEntry(BlockTag, KPhysicalAddress phys_addr, const PageTableEntry &attr, SeparateContiguousTag)
                : PageTableEntry(attr, GetInteger(phys_addr))
            {
                /* ... */
            }
        protected:
            constexpr ALWAYS_INLINE u64 GetBits(size_t offset, size_t count) const {
                return (m_attributes >> offset) & ((1ul << count) - 1);
            }

            constexpr ALWAYS_INLINE u64 SelectBits(size_t offset, size_t count) const {
                return m_attributes & (((1ul << count) - 1) << offset);
            }

            constexpr ALWAYS_INLINE void SetBits(size_t offset, size_t count, u64 value) {
                const u64 mask = ((1ul << count) - 1) << offset;
                m_attributes &= ~mask;
                m_attributes |= (value & (mask >> offset)) << offset;
            }

            constexpr ALWAYS_INLINE void SetBitsDirect(size_t offset, size_t count, u64 value) {
                const u64 mask = ((1ul << count) - 1) << offset;
                m_attributes &= ~mask;
                m_attributes |= (value & mask);
            }

            constexpr ALWAYS_INLINE void SetBit(size_t offset, bool enabled) {
                const u64 mask = 1ul << offset;
                if (enabled) {
                    m_attributes |= mask;
                } else {
                    m_attributes &= ~mask;
                }
            }
        public:
            constexpr ALWAYS_INLINE u8 GetSoftwareReservedBits()            const { return this->GetBits(55, 3); }
            constexpr ALWAYS_INLINE bool IsHeadMergeDisabled()              const { return (this->GetSoftwareReservedBits() & SoftwareReservedBit_DisableMergeHead) != 0; }
            constexpr ALWAYS_INLINE bool IsHeadAndBodyMergeDisabled()       const { return (this->GetSoftwareReservedBits() & SoftwareReservedBit_DisableMergeHeadAndBody) != 0; }
            constexpr ALWAYS_INLINE bool IsTailMergeDisabled()              const { return (this->GetSoftwareReservedBits() & SoftwareReservedBit_DisableMergeHeadTail) != 0; }
            constexpr ALWAYS_INLINE bool IsHeadOrHeadAndBodyMergeDisabled() const { return (this->GetSoftwareReservedBits() & (SoftwareReservedBit_DisableMergeHead | SoftwareReservedBit_DisableMergeHeadAndBody)) != 0; }
            constexpr ALWAYS_INLINE bool IsUserExecuteNever()               const { return this->GetBits(54, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsPrivilegedExecuteNever()         const { return this->GetBits(53, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsContiguous()                     const { return this->GetBits(52, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsGlobal()                         const { return this->GetBits(11, 1) == 0; }
            constexpr ALWAYS_INLINE AccessFlag GetAccessFlag()              const { return static_cast<AccessFlag>(this->SelectBits(10, 1)); }
            constexpr ALWAYS_INLINE Shareable GetShareable()                const { return static_cast<Shareable>(this->SelectBits(8, 2)); }
            constexpr ALWAYS_INLINE PageAttribute GetPageAttribute()        const { return static_cast<PageAttribute>(this->SelectBits(2, 3)); }
            constexpr ALWAYS_INLINE int GetAccessFlagInteger()              const { return static_cast<int>(this->GetBits(10, 1)); }
            constexpr ALWAYS_INLINE int GetShareableInteger()               const { return static_cast<int>(this->GetBits(8, 2)); }
            constexpr ALWAYS_INLINE int GetPageAttributeInteger()           const { return static_cast<int>(this->GetBits(2, 3)); }
            constexpr ALWAYS_INLINE bool IsReadOnly()                       const { return this->GetBits(7, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsUserAccessible()                 const { return this->GetBits(6, 1) != 0; }
            constexpr ALWAYS_INLINE bool IsNonSecure()                      const { return this->GetBits(5, 1) != 0; }

            constexpr ALWAYS_INLINE u64 GetTestTableMask()                  const { return (m_attributes & ExtensionFlag_TestTableMask); }

            constexpr ALWAYS_INLINE bool IsBlock()                          const { return (m_attributes & ExtensionFlag_TestTableMask) == ExtensionFlag_Valid; }
            constexpr ALWAYS_INLINE bool IsPage()                           const { return (m_attributes & ExtensionFlag_TestTableMask) == ExtensionFlag_TestTableMask; }
            constexpr ALWAYS_INLINE bool IsTable()                          const { return (m_attributes & ExtensionFlag_TestTableMask) == 2; }
            constexpr ALWAYS_INLINE bool IsEmpty()                          const { return (m_attributes & ExtensionFlag_TestTableMask) == 0; }

            constexpr ALWAYS_INLINE KPhysicalAddress GetTable()             const { return this->SelectBits(12, 36); }

            constexpr ALWAYS_INLINE bool IsMappedBlock()                    const { return this->GetBits(0, 2) == 1; }
            constexpr ALWAYS_INLINE bool IsMappedTable()                    const { return this->GetBits(0, 2) == 3; }
            constexpr ALWAYS_INLINE bool IsMappedEmpty()                    const { return this->GetBits(0, 2) == 0; }
            constexpr ALWAYS_INLINE bool IsMapped()                         const { return this->GetBits(0, 1) != 0; }

            constexpr ALWAYS_INLINE decltype(auto) SetUserExecuteNever(bool en)       { this->SetBit(54, en); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetPrivilegedExecuteNever(bool en) { this->SetBit(53, en); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetContiguous(bool en)             { this->SetBit(52, en); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetGlobal(bool en)                 { this->SetBit(11, !en); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetAccessFlag(AccessFlag f)        { this->SetBitsDirect(10, 1, f); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetShareable(Shareable s)          { this->SetBitsDirect(8, 2, s); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetReadOnly(bool en)               { this->SetBit(7, en); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetUserAccessible(bool en)         { this->SetBit(6, en); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetPageAttribute(PageAttribute a)  { this->SetBitsDirect(2, 3, a); return *this; }
            constexpr ALWAYS_INLINE decltype(auto) SetMapped(bool m)                  { static_assert(static_cast<u64>(MappingFlag_Mapped == (1 << 0))); this->SetBit(0, m); return *this; }

            constexpr ALWAYS_INLINE size_t GetTableReferenceCount() const { return this->GetBits(2, 10); }
            constexpr ALWAYS_INLINE decltype(auto) SetTableReferenceCount(size_t num) { this->SetBits(2, 10, num); return *this; }

            constexpr ALWAYS_INLINE decltype(auto) OpenTableReferences(size_t num) { MESOSPHERE_ASSERT(this->GetTableReferenceCount() + num <= BlocksPerTable + 1); return this->SetTableReferenceCount(this->GetTableReferenceCount() + num); }
            constexpr ALWAYS_INLINE decltype(auto) CloseTableReferences(size_t num) { MESOSPHERE_ASSERT(this->GetTableReferenceCount() >= num); return this->SetTableReferenceCount(this->GetTableReferenceCount() - num); }

            constexpr ALWAYS_INLINE decltype(auto) SetValid() { MESOSPHERE_ASSERT((m_attributes & ExtensionFlag_Valid) == 0); m_attributes |= ExtensionFlag_Valid; return *this; }

            constexpr ALWAYS_INLINE u64 GetEntryTemplateForMerge() const {
                constexpr u64 BaseMask = (0xFFFF000000000FFFul & ~static_cast<u64>((0x1ul << 52) | ExtensionFlag_TestTableMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail));
                return m_attributes & BaseMask;
            }

            constexpr ALWAYS_INLINE bool IsForMerge(u64 attr) const {
                constexpr u64 BaseMaskForMerge = ~static_cast<u64>(ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail);
                return (m_attributes & BaseMaskForMerge) == attr;
            }

            static constexpr ALWAYS_INLINE u64 GetEntryTemplateForSeparateContiguousMask(size_t idx) {
                constexpr u64 BaseMask = (0xFFFF000000000FFFul & ~static_cast<u64>((0x1ul << 52) | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail));
                if (idx == 0) {
                    return BaseMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < BlocksPerContiguousBlock - 1) {
                    return BaseMask;
                } else {
                    return BaseMask | ExtensionFlag_DisableMergeTail;
                }
            }

            constexpr ALWAYS_INLINE u64 GetEntryTemplateForSeparateContiguous(size_t idx) const {
                return m_attributes & GetEntryTemplateForSeparateContiguousMask(idx);
            }

            static constexpr ALWAYS_INLINE u64 GetEntryTemplateForSeparateMask(size_t idx) {
                constexpr u64 BaseMask = (0xFFFF000000000FFFul & ~static_cast<u64>((0x1ul << 52) | ExtensionFlag_TestTableMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail));
                if (idx == 0) {
                    return BaseMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < BlocksPerContiguousBlock) {
                    return BaseMask | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < BlocksPerTable - 1) {
                    return BaseMask;
                } else {
                    return BaseMask | ExtensionFlag_DisableMergeTail;
                }
            }

            constexpr ALWAYS_INLINE u64 GetEntryTemplateForSeparate(size_t idx) const {
                return m_attributes & GetEntryTemplateForSeparateMask(idx);
            }

            constexpr ALWAYS_INLINE u64 GetRawAttributesUnsafe() const {
                return m_attributes;
            }

            constexpr ALWAYS_INLINE u64 GetRawAttributesUnsafeForSwap() const {
                return m_attributes;
            }
        protected:
            constexpr ALWAYS_INLINE u64 GetRawAttributes() const {
                return m_attributes;
            }
    };

    static_assert(sizeof(PageTableEntry) == sizeof(u64));

    constexpr inline PageTableEntry InvalidPageTableEntry = PageTableEntry(PageTableEntry::InvalidTag{});

    constexpr inline size_t MaxPageTableEntries = PageSize / sizeof(PageTableEntry);

    class L1PageTableEntry : public PageTableEntry {
        public:
            constexpr explicit ALWAYS_INLINE L1PageTableEntry(InvalidTag) : PageTableEntry(InvalidTag{}) { /* ... */ }

            constexpr explicit ALWAYS_INLINE L1PageTableEntry(TableTag, KPhysicalAddress phys_addr, bool pxn)
                : PageTableEntry((0x3ul << 60) | (static_cast<u64>(pxn) << 59) | GetInteger(phys_addr) | 0x3)
            {
                /* ... */
            }

            constexpr explicit ALWAYS_INLINE L1PageTableEntry(TableTag, KPhysicalAddress phys_addr, bool is_kernel, bool pxn)
                : PageTableEntry(((is_kernel ? 0x3ul : 0) << 60) | (static_cast<u64>(pxn) << 59) | GetInteger(phys_addr) | 0x3)
            {
                /* ... */
            }

            constexpr explicit ALWAYS_INLINE L1PageTableEntry(BlockTag, KPhysicalAddress phys_addr, const PageTableEntry &attr, u8 sw_reserved_bits, bool contig)
                : PageTableEntry(attr, (static_cast<u64>(sw_reserved_bits) << 55) | (static_cast<u64>(contig) << 52) | GetInteger(phys_addr) | 0x1)
            {
                /* ... */
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetBlock() const {
                return this->SelectBits(30, 18);
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetTable() const {
                return this->SelectBits(12, 36);
            }

            constexpr ALWAYS_INLINE bool GetTable(KPhysicalAddress &out) const {
                if (this->IsTable()) {
                    out = this->GetTable();
                    return true;
                } else {
                    return false;
                }
            }

            static constexpr ALWAYS_INLINE u64 GetEntryTemplateForL2BlockMask(size_t idx) {
                constexpr u64 BaseMask = (0xFFF0000000000FFFul & ~static_cast<u64>((0x1ul << 52) | ExtensionFlag_TestTableMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail));
                if (idx == 0) {
                    return BaseMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < L2ContiguousBlockSize / L2BlockSize) {
                    return BaseMask | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < (L1BlockSize - L2ContiguousBlockSize) / L2BlockSize) {
                    return BaseMask;
                } else {
                    return BaseMask | ExtensionFlag_DisableMergeTail;
                }
            }

            constexpr ALWAYS_INLINE u64 GetEntryTemplateForL2Block(size_t idx) const {
                return m_attributes & GetEntryTemplateForL2BlockMask(idx);
            }

            constexpr ALWAYS_INLINE bool IsCompatibleWithAttribute(const PageTableEntry &rhs, u8 sw_reserved_bits, bool contig) const {
                /* Check whether this has the same permission/etc as the desired attributes. */
                return L1PageTableEntry(BlockTag{}, this->GetBlock(), rhs, sw_reserved_bits, contig).GetRawAttributes() == this->GetRawAttributes();
            }
    };

    class L2PageTableEntry : public PageTableEntry {
        public:
            constexpr explicit ALWAYS_INLINE L2PageTableEntry(InvalidTag) : PageTableEntry(InvalidTag{}) { /* ... */ }

            constexpr explicit ALWAYS_INLINE L2PageTableEntry(TableTag, KPhysicalAddress phys_addr, bool pxn)
                : PageTableEntry((0x3ul << 60) | (static_cast<u64>(pxn) << 59) | GetInteger(phys_addr) | 0x3)
            {
                /* ... */
            }

            constexpr explicit ALWAYS_INLINE L2PageTableEntry(TableTag, KPhysicalAddress phys_addr, bool is_kernel, bool pxn)
                : PageTableEntry(((is_kernel ? 0x3ul : 0) << 60) | (static_cast<u64>(pxn) << 59) | GetInteger(phys_addr) | 0x3)
            {
                /* ... */
            }

            constexpr explicit ALWAYS_INLINE L2PageTableEntry(BlockTag, KPhysicalAddress phys_addr, const PageTableEntry &attr, u8 sw_reserved_bits, bool contig)
                : PageTableEntry(attr, (static_cast<u64>(sw_reserved_bits) << 55) | (static_cast<u64>(contig) << 52) | GetInteger(phys_addr) | 0x1)
            {
                /* ... */
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetBlock() const {
                return this->SelectBits(21, 27);
            }

            constexpr ALWAYS_INLINE KPhysicalAddress GetTable() const {
                return this->SelectBits(12, 36);
            }

            constexpr ALWAYS_INLINE bool GetTable(KPhysicalAddress &out) const {
                if (this->IsTable()) {
                    out = this->GetTable();
                    return true;
                } else {
                    return false;
                }
            }

            static constexpr ALWAYS_INLINE u64 GetEntryTemplateForL2BlockMask(size_t idx) {
                constexpr u64 BaseMask = (0xFFF0000000000FFFul & ~static_cast<u64>((0x1ul << 52) | ExtensionFlag_TestTableMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail));
                if (idx == 0) {
                    return BaseMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < (L2ContiguousBlockSize / L2BlockSize) - 1) {
                    return BaseMask;
                } else {
                    return BaseMask | ExtensionFlag_DisableMergeTail;
                }
            }

            constexpr ALWAYS_INLINE u64 GetEntryTemplateForL2Block(size_t idx) const {
                return m_attributes & GetEntryTemplateForL2BlockMask(idx);
            }

            static constexpr ALWAYS_INLINE u64 GetEntryTemplateForL3BlockMask(size_t idx) {
                constexpr u64 BaseMask = (0xFFF0000000000FFFul & ~static_cast<u64>((0x1ul << 52) | ExtensionFlag_TestTableMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail));
                if (idx == 0) {
                    return BaseMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < L3ContiguousBlockSize / L3BlockSize) {
                    return BaseMask | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < (L2BlockSize - L3ContiguousBlockSize) / L3BlockSize) {
                    return BaseMask;
                } else {
                    return BaseMask | ExtensionFlag_DisableMergeTail;
                }
            }

            constexpr ALWAYS_INLINE u64 GetEntryTemplateForL3Block(size_t idx) const {
                return m_attributes & GetEntryTemplateForL3BlockMask(idx);
            }

            constexpr ALWAYS_INLINE bool IsCompatibleWithAttribute(const PageTableEntry &rhs, u8 sw_reserved_bits, bool contig) const {
                /* Check whether this has the same permission/etc as the desired attributes. */
                return L2PageTableEntry(BlockTag{}, this->GetBlock(), rhs, sw_reserved_bits, contig).GetRawAttributes() == this->GetRawAttributes();
            }
    };

    class L3PageTableEntry : public PageTableEntry {
        public:
            constexpr explicit ALWAYS_INLINE L3PageTableEntry(InvalidTag) : PageTableEntry(InvalidTag{}) { /* ... */ }

            constexpr explicit ALWAYS_INLINE L3PageTableEntry(BlockTag, KPhysicalAddress phys_addr, const PageTableEntry &attr, u8 sw_reserved_bits, bool contig)
                : PageTableEntry(attr, (static_cast<u64>(sw_reserved_bits) << 55) | (static_cast<u64>(contig) << 52) | GetInteger(phys_addr) | 0x3)
            {
                /* ... */
            }

            constexpr ALWAYS_INLINE bool IsBlock() const { return (GetRawAttributes() & ExtensionFlag_TestTableMask) == ExtensionFlag_TestTableMask; }
            constexpr ALWAYS_INLINE bool IsMappedBlock() const { return this->GetBits(0, 2) == 3; }

            constexpr ALWAYS_INLINE KPhysicalAddress GetBlock() const {
                return this->SelectBits(12, 36);
            }

            static constexpr ALWAYS_INLINE u64 GetEntryTemplateForL3BlockMask(size_t idx) {
                constexpr u64 BaseMask = (0xFFF0000000000FFFul & ~static_cast<u64>((0x1ul << 52) | ExtensionFlag_TestTableMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody | ExtensionFlag_DisableMergeTail));
                if (idx == 0) {
                    return BaseMask | ExtensionFlag_DisableMergeHead | ExtensionFlag_DisableMergeHeadAndBody;
                } else if (idx < (L3ContiguousBlockSize / L3BlockSize) - 1) {
                    return BaseMask;
                } else {
                    return BaseMask | ExtensionFlag_DisableMergeTail;
                }
            }

            constexpr ALWAYS_INLINE u64 GetEntryTemplateForL3Block(size_t idx) const {
                return m_attributes & GetEntryTemplateForL3BlockMask(idx);
            }

            constexpr ALWAYS_INLINE bool IsCompatibleWithAttribute(const PageTableEntry &rhs, u8 sw_reserved_bits, bool contig) const {
                /* Check whether this has the same permission/etc as the desired attributes. */
                return L3PageTableEntry(BlockTag{}, this->GetBlock(), rhs, sw_reserved_bits, contig).GetRawAttributes() == this->GetRawAttributes();
            }
    };

    constexpr inline L1PageTableEntry InvalidL1PageTableEntry = L1PageTableEntry(PageTableEntry::InvalidTag{});
    constexpr inline L2PageTableEntry InvalidL2PageTableEntry = L2PageTableEntry(PageTableEntry::InvalidTag{});
    constexpr inline L3PageTableEntry InvalidL3PageTableEntry = L3PageTableEntry(PageTableEntry::InvalidTag{});

}
