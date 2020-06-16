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
#include <exosphere.hpp>

namespace ams::gic {

    namespace {

        struct GicDistributor {
            u32 ctlr;
            u32 typer;
            u32 iidr;
            u32 reserved_0x0c;
            u32 statusr;
            u32 reserved_0x14[3];
            u32 impldef_0x20[8];
            u32 setspi_nsr;
            u32 reserved_0x44;
            u32 clrspi_nsr;
            u32 reserved_0x4c;
            u32 setspi_sr;
            u32 reserved_0x54;
            u32 clrspi_sr;
            u32 reserved_0x5c[9];
            u32 igroupr[32];
            u32 isenabler[32];
            u32 icenabler[32];
            u32 ispendr[32];
            u32 icpendr[32];
            u32 isactiver[32];
            u32 icactiver[32];
            union {
                u8  bytes[1020];
                u32 words[255];
            } ipriorityr;
            u32 _0x7fc;
            union {
                u8  bytes[1020];
                u32 words[255];
            } itargetsr;
            u32 _0xbfc;
            u32 icfgr[64];
            u32 igrpmodr[32];
            u32 _0xd80[32];
            u32 nsacr[64];
            u32 sgir;
            u32 _0xf04[3];
            u32 cpendsgir[4];
            u32 spendsgir[4];
            u32 reserved_0xf30[52];

            static constexpr size_t SgirCpuTargetListShift = 16;

            enum SgirTargetListFilter : u32 {
                SgirTargetListFilter_CpuTargetList = (0 << 24),
                SgirTargetListFilter_Others        = (1 << 24),
                SgirTargetListFilter_Self          = (2 << 24),
                SgirTargetListFilter_Reserved      = (3 << 24),
            };
        };
        static_assert(util::is_pod<GicDistributor>::value);
        static_assert(sizeof(GicDistributor) == 0x1000);
        static_assert(sizeof(GicDistributor) == secmon::MemoryRegionPhysicalDeviceGicDistributor.GetSize());

        struct GicCpuInterface {
            u32 ctlr;
            u32 pmr;
            u32 bpr;
            u32 iar;
            u32 eoir;
            u32 rpr;
            u32 hppir;
            u32 abpr;
            u32 aiar;
            u32 aeoir;
            u32 ahppir;
            u32 statusr;
            u32 reserved_30[4];
            u32 impldef_40[36];
            u32 apr[4];
            u32 nsapr[4];
            u32 reserved_f0[3];
            u32 iidr;
            u32 reserved_100[960];
            u32 dir;
            u32 _0x1004[1023];
        };
        static_assert(util::is_pod<GicCpuInterface>::value);
        static_assert(sizeof(GicCpuInterface) == 0x2000);
        static_assert(sizeof(GicCpuInterface) == secmon::MemoryRegionPhysicalDeviceGicCpuInterface.GetSize());

        constexpr inline int InterruptWords = InterruptCount / BITSIZEOF(u32);
        constexpr inline int SpiIndex       = BITSIZEOF(u32);

        constinit uintptr_t g_distributor_address   = secmon::MemoryRegionPhysicalDeviceGicDistributor.GetAddress();
        constinit uintptr_t g_cpu_interface_address = secmon::MemoryRegionPhysicalDeviceGicCpuInterface.GetAddress();

        volatile GicDistributor *GetDistributor() {
            return reinterpret_cast<volatile GicDistributor *>(g_distributor_address);
        }

        volatile GicCpuInterface *GetCpuInterface() {
            return reinterpret_cast<volatile GicCpuInterface *>(g_cpu_interface_address);
        }

        void ReadWrite(uintptr_t address, int width, int i, u32 value) {
            /* This code will never be invoked with a negative interrupt id. */
            AMS_ASSUME(i >= 0);

            const int scale = BITSIZEOF(u32) / width;
            const int word  = i / scale;
            const int bit   = (i % scale) * width;

            const u32 mask = ((1u << width) - 1) << bit;

            const uintptr_t reg_addr = address + sizeof(u32) * word;
            const u32 old = reg::Read(reg_addr) & ~mask;
            reg::Write(reg_addr, old | ((value << bit) & mask));
        }

        void Write(uintptr_t address, int width, int i, u32 value) {
            /* This code will never be invoked with a negative interrupt id. */
            AMS_ASSUME(i >= 0);

            const int scale = BITSIZEOF(u32) / width;
            const int word  = i / scale;
            const int bit   = (i % scale) * width;

            reg::Write(address + sizeof(u32) * word, value << bit);
        }

    }

    void SetRegisterAddress(uintptr_t distributor_address, uintptr_t cpu_interface_address) {
        g_distributor_address   = distributor_address;
        g_cpu_interface_address = cpu_interface_address;
    }

    void InitializeCommon() {
        /* Get the gicd registers. */
        auto *gicd = GetDistributor();

        /* Set IGROUPR for to be FFs. */
        for (int i = SpiIndex / BITSIZEOF(u32); i < InterruptWords; ++i) {
            gicd->igroupr[i] = 0xFFFFFFFFu;
        }

        /* Set IPRIORITYR for spi interrupts to be 0x80. */
        for (int i = SpiIndex; i < InterruptCount; ++i) {
            gicd->ipriorityr.bytes[i] = 0x80;
        }

        /* Enable group 0. */
        gicd->ctlr = 1;
    }

    void InitializeCoreUnique() {
        /* Get the registers. */
        auto *gicd = GetDistributor();
        auto *gicc = GetCpuInterface();

        /* Set IGROUPR0 to be FFs. */
        gicd->igroupr[0] = 0xFFFFFFFFu;

        /* Set IPRIORITYR for core local interrupts to be 0x80. */
        for (int i = 0; i < SpiIndex; ++i) {
            gicd->ipriorityr.bytes[i] = 0x80;
        }

        /* Enable group 0 as FIQs. */
        gicc->ctlr = 0x1D9;

        /* Set PMR. */
        gicc->pmr = 0x80;

        /* Set BPR. */
        gicc->bpr = 7;
    }

    void SetPriority(int interrupt_id, int priority) {
        ReadWrite(g_distributor_address + offsetof(GicDistributor, ipriorityr), BITSIZEOF(u8), interrupt_id, priority);
    }

    void SetInterruptGroup(int interrupt_id, int group) {
        ReadWrite(g_distributor_address + offsetof(GicDistributor, igroupr), 1, interrupt_id, group);
    }

    void SetEnable(int interrupt_id, bool enable) {
        Write(g_distributor_address + offsetof(GicDistributor, isenabler), 1, interrupt_id, enable);
    }

    void SetSpiTargetCpu(int interrupt_id, u32 cpu_mask) {
        ReadWrite(g_distributor_address + offsetof(GicDistributor, itargetsr), BITSIZEOF(u8), interrupt_id, cpu_mask);
    }

    void SetSpiMode(int interrupt_id, InterruptMode mode) {
        ReadWrite(g_distributor_address + offsetof(GicDistributor, icfgr), 2, interrupt_id, static_cast<u32>(mode) << 1);
    }

    void SetPending(int interrupt_id) {
        Write(g_distributor_address + offsetof(GicDistributor, ispendr), 1, interrupt_id, 1);
    }

    int GetInterruptRequestId() {
        return reg::Read(GetCpuInterface()->iar);
    }

    void SetEndOfInterrupt(int interrupt_id) {
        reg::Write(GetCpuInterface()->eoir, interrupt_id);
    }

}
