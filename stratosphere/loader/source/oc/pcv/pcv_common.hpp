/*
 * Copyright (C) Switch-OC-Suite
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

namespace ams::ldr::oc::pcv {

typedef struct cvb_coefficients {
    s32 c0 = 0;
    s32 c1 = 0;
    s32 c2 = 0;
    s32 c3 = 0;
    s32 c4 = 0;
    s32 c5 = 0;
} cvb_coefficients;

typedef struct cvb_entry_t {
    u64 freq;
    cvb_coefficients cvb_dfll_param;
    cvb_coefficients cvb_pll_param;
} cvb_entry_t;
static_assert(sizeof(cvb_entry_t) == 0x38);

typedef struct emc_dvb_dvfs_table_t {
    u64 freq;
    s32 volt[4] = {0};
} emc_dvb_dvfs_table_t;

typedef struct __attribute__((packed)) div_nmp {
    u8 divn_shift;
    u8 divn_width;
    u8 divm_shift;
    u8 divm_width;
    u8 divp_shift;
    u8 divp_width;
    u8 override_divn_shift;
    u8 override_divm_shift;
    u8 override_divp_shift;
} div_nmp;

typedef struct __attribute__((packed)) clk_pll_param {
    u32 freq;
    u32 input_min;
    u32 input_max;
    u32 cf_min;
    u32 cf_max;
    u32 vco_min;
    u32 vco_max;
    s32 lock_delay;
    u32 fixed_rate;
    u32 unk_0;
    struct div_nmp *div_nmp;
    u32 unk_1[4];
    void (*unk_fn)(u64* unk_struct); // set_defaults?
} clk_pll_param;

typedef struct __attribute__((packed)) dvfs_rail {
    u32 id;
    u32 unk_0[5];
    u32 freq;
    u32 unk_1[8];
    u32 unk_flag;
    u32 min_mv;
    u32 step_mv;
    u32 max_mv;
    u32 unk_2[11];
} dvfs_rail;

typedef struct __attribute__((packed)) regulator {
    u64 id;
    const char* name;
    u32 type;
    union {
        struct {
            u32 volt_reg;
            u32 step_uv;
            u32 min_uv;
            u32 default_uv;
            u32 max_uv;
            u32 unk_0[2];
        } type_1;
        struct {
            u32 unk_0;
            u32 step_uv;
            u32 unk_1;
            u32 min_uv;
            u32 max_uv;
            u32 unk_2;
            u32 default_uv;
        } type_2_3;
    };
    u32 unk_x[60];
} regulator;
static_assert(sizeof(regulator) == 0x120);

constexpr u32 CpuClkOSLimit   = 1785'000;

constexpr u32 EmcClkOSLimit   = 1600'000;

#define R_SKIP() R_SUCCEED()

// Count 32 / Index 31 is reserved to be empty
constexpr size_t DvfsTableEntryCount = 32;
constexpr size_t DvfsTableEntryLimit = DvfsTableEntryCount - 1;

template<typename T>
size_t GetDvfsTableEntryCount(T* table_head) {
    using NT = std::remove_const_t<std::remove_volatile_t<T>>;

    auto is_empty = [](NT* entry) {
        uint8_t* m = reinterpret_cast<uint8_t *>(entry);
        for (size_t i = 0; i < sizeof(NT); i++) {
            if (*(m + i))
                return false;
        }
        return true;
    };

    NT* table = const_cast<NT *>(table_head);
    size_t count = 0;
    while (count < DvfsTableEntryLimit) {
        if (is_empty(table++)) {
            return count;
        }
        count++;
    }
    return DvfsTableEntryLimit;
}

template<typename T>
T* GetDvfsTableLastEntry(T* table_head) {
    using NT = std::remove_const_t<std::remove_volatile_t<T>>;

    NT* table = const_cast<NT *>(table_head);
    size_t count = GetDvfsTableEntryCount(table_head);
    if (!count) {
        return nullptr;
    }
    size_t index = count - 1;
    return table + index;
}

}