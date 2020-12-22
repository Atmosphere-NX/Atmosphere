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
#include <mesosphere/kern_k_address_space_info.hpp>
#include <mesosphere/kern_select_page_table.hpp>

namespace ams::kern {

    class KInitialProcessHeader {
        private:
            static constexpr u32 Magic = util::FourCC<'K','I','P','1'>::Code;
        private:
            u32 m_magic;
            u8 m_name[12];
            u64 m_program_id;
            u32 m_version;
            u8 m_priority;
            u8 m_ideal_core_id;
            u8 m_1E;
            u8 m_flags;
            u32 m_rx_address;
            u32 m_rx_size;
            u32 m_rx_compressed_size;
            u32 m_affinity_mask;
            u32 m_ro_address;
            u32 m_ro_size;
            u32 m_ro_compressed_size;
            u32 m_stack_size;
            u32 m_rw_address;
            u32 m_rw_size;
            u32 m_rw_compressed_size;
            u32 m_4C;
            u32 m_bss_address;
            u32 m_bss_size;
            u32 m_pad[(0x80 - 0x58) / sizeof(u32)];
            u32 m_capabilities[0x80 / sizeof(u32)];
        public:
            constexpr bool IsValid() const { return m_magic == Magic; }

            constexpr void GetName(char *dst, size_t size) const {
                std::memset(dst, 0, size);
                std::memcpy(dst, m_name, std::min(sizeof(m_name), size));
            }

            constexpr const u32 *GetCapabilities()  const { return m_capabilities; }
            constexpr size_t GetNumCapabilities()   const { return util::size(m_capabilities); }

            constexpr u64 GetProgramId()            const { return m_program_id; }
            constexpr u32 GetVersion()              const { return m_version; }
            constexpr u8  GetPriority()             const { return m_priority; }
            constexpr u8  GetIdealCoreId()          const { return m_ideal_core_id; }

            constexpr bool IsRxCompressed()         const { return (m_flags & (1 << 0)); }
            constexpr bool IsRoCompressed()         const { return (m_flags & (1 << 1)); }
            constexpr bool IsRwCompressed()         const { return (m_flags & (1 << 2)); }
            constexpr bool Is64Bit()                const { return (m_flags & (1 << 3)); }
            constexpr bool Is64BitAddressSpace()    const { return (m_flags & (1 << 4)); }
            constexpr bool UsesSecureMemory()       const { return (m_flags & (1 << 5)); }

            constexpr u32 GetRxAddress()            const { return m_rx_address; }
            constexpr u32 GetRxSize()               const { return m_rx_size; }
            constexpr u32 GetRxCompressedSize()     const { return m_rx_compressed_size; }
            constexpr u32 GetRoAddress()            const { return m_ro_address; }
            constexpr u32 GetRoSize()               const { return m_ro_size; }
            constexpr u32 GetRoCompressedSize()     const { return m_ro_compressed_size; }
            constexpr u32 GetRwAddress()            const { return m_rw_address; }
            constexpr u32 GetRwSize()               const { return m_rw_size; }
            constexpr u32 GetRwCompressedSize()     const { return m_rw_compressed_size; }
            constexpr u32 GetBssAddress()           const { return m_bss_address; }
            constexpr u32 GetBssSize()              const { return m_bss_size; }

            constexpr u32 GetAffinityMask()         const { return m_affinity_mask; }
            constexpr u32 GetStackSize()            const { return m_stack_size; }
    };
    static_assert(sizeof(KInitialProcessHeader) == 0x100);

    class KInitialProcessReader {
        private:
            KInitialProcessHeader *m_kip_header;
        public:
            constexpr KInitialProcessReader() : m_kip_header() { /* ... */ }

            constexpr const u32 *GetCapabilities()  const { return m_kip_header->GetCapabilities(); }
            constexpr size_t GetNumCapabilities()   const { return m_kip_header->GetNumCapabilities(); }

            constexpr size_t GetBinarySize() const {
                return sizeof(*m_kip_header) + m_kip_header->GetRxCompressedSize() + m_kip_header->GetRoCompressedSize() + m_kip_header->GetRwCompressedSize();
            }

            constexpr size_t GetSize() const {
                if (const size_t bss_size = m_kip_header->GetBssSize(); bss_size != 0) {
                    return m_kip_header->GetBssAddress() + m_kip_header->GetBssSize();
                } else {
                    return m_kip_header->GetRwAddress() + m_kip_header->GetRwSize();
                }
            }

            constexpr u8  GetPriority()             const { return m_kip_header->GetPriority(); }
            constexpr u8  GetIdealCoreId()          const { return m_kip_header->GetIdealCoreId(); }
            constexpr u32 GetAffinityMask()         const { return m_kip_header->GetAffinityMask(); }
            constexpr u32 GetStackSize()            const { return m_kip_header->GetStackSize(); }

            constexpr bool Is64Bit()                const { return m_kip_header->Is64Bit(); }
            constexpr bool Is64BitAddressSpace()    const { return m_kip_header->Is64BitAddressSpace(); }
            constexpr bool UsesSecureMemory()       const { return m_kip_header->UsesSecureMemory(); }

            bool Attach(u8 *bin) {
                if (KInitialProcessHeader *header = reinterpret_cast<KInitialProcessHeader *>(bin); header->IsValid()) {
                    m_kip_header = header;
                    return true;
                } else {
                    return false;
                }
            }

            Result MakeCreateProcessParameter(ams::svc::CreateProcessParameter *out, bool enable_aslr) const;
            Result Load(KProcessAddress address, const ams::svc::CreateProcessParameter &params) const;
            Result SetMemoryPermissions(KProcessPageTable &page_table, const ams::svc::CreateProcessParameter &params) const;
    };

}
