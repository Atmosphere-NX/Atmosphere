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
            u32 magic;
            u8 name[12];
            u64 program_id;
            u32 version;
            u8 priority;
            u8 ideal_core_id;
            u8 _1E;
            u8 flags;
            u32 rx_address;
            u32 rx_size;
            u32 rx_compressed_size;
            u32 affinity_mask;
            u32 ro_address;
            u32 ro_size;
            u32 ro_compressed_size;
            u32 stack_size;
            u32 rw_address;
            u32 rw_size;
            u32 rw_compressed_size;
            u32 _4C;
            u32 bss_address;
            u32 bss_size;
            u32 pad[(0x80 - 0x58) / sizeof(u32)];
            u32 capabilities[0x80 / sizeof(u32)];
        public:
            constexpr bool IsValid() const { return this->magic == Magic; }

            constexpr void GetName(char *dst, size_t size) const {
                std::memset(dst, 0, size);
                std::memcpy(dst, this->name, std::min(sizeof(this->name), size));
            }

            constexpr const u32 *GetCapabilities()  const { return this->capabilities; }
            constexpr size_t GetNumCapabilities()   const { return util::size(this->capabilities); }

            constexpr u64 GetProgramId()            const { return this->program_id; }
            constexpr u32 GetVersion()              const { return this->version; }
            constexpr u8  GetPriority()             const { return this->priority; }
            constexpr u8  GetIdealCoreId()          const { return this->ideal_core_id; }

            constexpr bool IsRxCompressed()         const { return (this->flags & (1 << 0)); }
            constexpr bool IsRoCompressed()         const { return (this->flags & (1 << 1)); }
            constexpr bool IsRwCompressed()         const { return (this->flags & (1 << 2)); }
            constexpr bool Is64Bit()                const { return (this->flags & (1 << 3)); }
            constexpr bool Is64BitAddressSpace()    const { return (this->flags & (1 << 4)); }
            constexpr bool UsesSecureMemory()       const { return (this->flags & (1 << 5)); }

            constexpr u32 GetRxAddress()            const { return this->rx_address; }
            constexpr u32 GetRxSize()               const { return this->rx_size; }
            constexpr u32 GetRxCompressedSize()     const { return this->rx_compressed_size; }
            constexpr u32 GetRoAddress()            const { return this->ro_address; }
            constexpr u32 GetRoSize()               const { return this->ro_size; }
            constexpr u32 GetRoCompressedSize()     const { return this->ro_compressed_size; }
            constexpr u32 GetRwAddress()            const { return this->rw_address; }
            constexpr u32 GetRwSize()               const { return this->rw_size; }
            constexpr u32 GetRwCompressedSize()     const { return this->rw_compressed_size; }
            constexpr u32 GetBssAddress()           const { return this->bss_address; }
            constexpr u32 GetBssSize()              const { return this->bss_size; }

            constexpr u32 GetAffinityMask()         const { return this->affinity_mask; }
            constexpr u32 GetStackSize()            const { return this->stack_size; }
    };
    static_assert(sizeof(KInitialProcessHeader) == 0x100);

    class KInitialProcessReader {
        private:
            KInitialProcessHeader *kip_header;
        public:
            constexpr KInitialProcessReader() : kip_header() { /* ... */ }

            constexpr const u32 *GetCapabilities()  const { return this->kip_header->GetCapabilities(); }
            constexpr size_t GetNumCapabilities()   const { return this->kip_header->GetNumCapabilities(); }

            constexpr size_t GetBinarySize() const {
                return sizeof(*kip_header) + this->kip_header->GetRxCompressedSize() + this->kip_header->GetRoCompressedSize() + this->kip_header->GetRwCompressedSize();
            }

            constexpr size_t GetSize() const {
                if (const size_t bss_size = this->kip_header->GetBssSize(); bss_size != 0) {
                    return this->kip_header->GetBssAddress() + this->kip_header->GetBssSize();
                } else {
                    return this->kip_header->GetRwAddress() + this->kip_header->GetRwSize();
                }
            }

            constexpr u8  GetPriority()             const { return this->kip_header->GetPriority(); }
            constexpr u8  GetIdealCoreId()          const { return this->kip_header->GetIdealCoreId(); }
            constexpr u32 GetAffinityMask()         const { return this->kip_header->GetAffinityMask(); }
            constexpr u32 GetStackSize()            const { return this->kip_header->GetStackSize(); }

            constexpr bool Is64Bit()                const { return this->kip_header->Is64Bit(); }
            constexpr bool Is64BitAddressSpace()    const { return this->kip_header->Is64BitAddressSpace(); }
            constexpr bool UsesSecureMemory()       const { return this->kip_header->UsesSecureMemory(); }

            bool Attach(u8 *bin) {
                if (KInitialProcessHeader *header = reinterpret_cast<KInitialProcessHeader *>(bin); header->IsValid()) {
                    this->kip_header = header;
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
