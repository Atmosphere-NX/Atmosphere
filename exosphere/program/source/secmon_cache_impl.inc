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

namespace {

    ALWAYS_INLINE int FloorLog2(int v) {
        return BITSIZEOF(u32) - (hw::CountLeadingZeros(static_cast<u32>(v)) + 1);
    }

    ALWAYS_INLINE int CeilLog2(int v) {
        const int log = FloorLog2(v);
        return ((1 << log) == v) ? log : log + 1;
    }

    void FlushDataCacheTo(int loc) {
        for (int level = 0; level < loc; ++level) {
            /* Set the selection register. */
            {
                util::BitPack32 csselr = {};
                csselr.Set<hw::CsselrEl1::InD>(0);
                csselr.Set<hw::CsselrEl1::Level>(level);
                HW_CPU_SET_CSSELR_EL1(csselr);
            }

            /* Ensure that reordering doesn't occur around this operation. */
            hw::InstructionSynchronizationBarrier();

            /* Get ccsidr. */
            util::BitPack32 ccsidr;
            HW_CPU_GET_CCSIDR_EL1(ccsidr);

            /* Get cache size id info. */
            const int num_sets  = ccsidr.Get<hw::CcsidrEl1::NumSets>() + 1;
            const int num_ways  = ccsidr.Get<hw::CcsidrEl1::Associativity>() + 1;
            const int line_size = ccsidr.Get<hw::CcsidrEl1::LineSize>() + 4;

            const int way_shift = 32 - FloorLog2(num_ways);
            const int set_shift = line_size;

            for (int way = 0; way <= num_ways; way++) {
                for (int set = 0; set <= num_sets; set++) {
                    const u64 value = (static_cast<u64>(way) << way_shift) | (static_cast<u64>(set) << set_shift) | (static_cast<u64>(level) << 1);
                    __asm__ __volatile__("dc cisw, %[value]" :: [value]"r"(value) : "memory");
                }
            }
        }
    }

    void FlushDataCacheFrom(int loc) {
        for (int level = loc - 1; level >= 0; --level) {
            /* Set the selection register. */
            {
                util::BitPack32 csselr = {};
                csselr.Set<hw::CsselrEl1::InD>(0);
                csselr.Set<hw::CsselrEl1::Level>(level);
                HW_CPU_SET_CSSELR_EL1(csselr);
            }

            /* Ensure that reordering doesn't occur around this operation. */
            hw::InstructionSynchronizationBarrier();

            /* Get ccsidr. */
            util::BitPack32 ccsidr;
            HW_CPU_GET_CCSIDR_EL1(ccsidr);

            /* Get cache size id info. */
            const int num_sets  = ccsidr.Get<hw::CcsidrEl1::NumSets>() + 1;
            const int num_ways  = ccsidr.Get<hw::CcsidrEl1::Associativity>() + 1;
            const int line_size = ccsidr.Get<hw::CcsidrEl1::LineSize>() + 4;

            const int way_shift = 32 - FloorLog2(num_ways);
            const int set_shift = line_size;

            for (int way = 0; way <= num_ways; way++) {
                for (int set = 0; set <= num_sets; set++) {
                    const u64 value = (static_cast<u64>(way) << way_shift) | (static_cast<u64>(set) << set_shift) | (static_cast<u64>(level) << 1);
                    __asm__ __volatile__("dc cisw, %[value]" :: [value]"r"(value) : "memory");
                }
            }
        }
    }

    void InvalidateDataCacheTo(int loc) {
        for (int level = 0; level < loc; ++level) {
            /* Set the selection register. */
            {
                util::BitPack32 csselr = {};
                csselr.Set<hw::CsselrEl1::InD>(0);
                csselr.Set<hw::CsselrEl1::Level>(level);
                HW_CPU_SET_CSSELR_EL1(csselr);
            }

            /* Ensure that reordering doesn't occur around this operation. */
            hw::InstructionSynchronizationBarrier();

            /* Get ccsidr. */
            util::BitPack32 ccsidr;
            HW_CPU_GET_CCSIDR_EL1(ccsidr);

            /* Get cache size id info. */
            const int num_sets  = ccsidr.Get<hw::CcsidrEl1::NumSets>() + 1;
            const int num_ways  = ccsidr.Get<hw::CcsidrEl1::Associativity>() + 1;
            const int line_size = ccsidr.Get<hw::CcsidrEl1::LineSize>() + 4;

            const int way_shift = 32 - FloorLog2(num_ways);
            const int set_shift = line_size;

            for (int way = 0; way <= num_ways; way++) {
                for (int set = 0; set <= num_sets; set++) {
                    const u64 value = (static_cast<u64>(way) << way_shift) | (static_cast<u64>(set) << set_shift) | (static_cast<u64>(level) << 1);
                    __asm__ __volatile__("dc isw, %[value]" :: [value]"r"(value) : "memory");
                }
            }
        }
    }

}

void FlushEntireDataCache() {
    util::BitPack32 clidr;
    HW_CPU_GET_CLIDR_EL1(clidr);
    FlushDataCacheTo(clidr.Get<hw::ClidrEl1::Loc>());
}

void FlushEntireDataCacheLocal() {
    util::BitPack32 clidr;
    HW_CPU_GET_CLIDR_EL1(clidr);
    FlushDataCacheFrom(clidr.Get<hw::ClidrEl1::Louis>());
}

void InvalidateEntireDataCache() {
    util::BitPack32 clidr;
    HW_CPU_GET_CLIDR_EL1(clidr);
    InvalidateDataCacheTo(clidr.Get<hw::ClidrEl1::Loc>());
}

void EnsureMappingConsistency() {
    ::ams::hw::DataSynchronizationBarrierInnerShareable();
    ::ams::hw::InvalidateEntireTlb();
    ::ams::hw::DataSynchronizationBarrierInnerShareable();

    ::ams::hw::InstructionSynchronizationBarrier();
}

void EnsureMappingConsistency(uintptr_t address) {
    ::ams::hw::DataSynchronizationBarrierInnerShareable();
    ::ams::hw::InvalidateTlb(address);
    ::ams::hw::DataSynchronizationBarrierInnerShareable();

    ::ams::hw::InstructionSynchronizationBarrier();
}

void EnsureInstructionConsistency() {
    ::ams::hw::DataSynchronizationBarrierInnerShareable();
    ::ams::hw::InvalidateEntireInstructionCache();
    ::ams::hw::DataSynchronizationBarrierInnerShareable();

    ::ams::hw::InstructionSynchronizationBarrier();
}
