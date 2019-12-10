/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

#include <stratosphere.hpp>

namespace ams::util {

    namespace {

        constexpr inline u32 XorByShifted27(u32 value) {
            return value ^ (value >> 27);
        }

        constexpr inline u32 XorByShifted30(u32 value) {
            return value ^ (value >> 30);
        }

    }

    void TinyMT::GenerateInitialValuePlus(TinyMT::State *state, int index, u32 value) {
        u32 &state0 = state->data[(index + 0) % NumStateWords];
        u32 &state1 = state->data[(index + 1) % NumStateWords];
        u32 &state2 = state->data[(index + 2) % NumStateWords];
        u32 &state3 = state->data[(index + 3) % NumStateWords];

        const u32 x = XorByShifted27(state0 ^ state1 ^ state3) * ParamPlus;
        const u32 y = x + index + value;

        state0  = y;
        state1 += x;
        state2 += y;
    }

    void TinyMT::GenerateInitialValueXor(TinyMT::State *state, int index) {
        u32 &state0 = state->data[(index + 0) % NumStateWords];
        u32 &state1 = state->data[(index + 1) % NumStateWords];
        u32 &state2 = state->data[(index + 2) % NumStateWords];
        u32 &state3 = state->data[(index + 3) % NumStateWords];

        const u32 x = XorByShifted27(state0 + state1 + state3) * ParamXor;
        const u32 y = x - index;

        state0  = y;
        state1 ^= x;
        state2 ^= y;
    }

    void TinyMT::Initialize(u32 seed) {
        this->state.data[0] = seed;
        this->state.data[1] = ParamMat1;
        this->state.data[2] = ParamMat2;
        this->state.data[3] = ParamTmat;

        for (int i = 1; i < MinimumInitIterations; i++) {
            const u32 mixed = XorByShifted30(this->state.data[(i - 1) % NumStateWords]);
            this->state.data[i % NumStateWords] ^= mixed * ParamMult + i;
        }

        this->FinalizeInitialization();
    }

    void TinyMT::Initialize(const u32 *seed, int seed_count) {
        this->state.data[0] = 0;
        this->state.data[1] = ParamMat1;
        this->state.data[2] = ParamMat2;
        this->state.data[3] = ParamTmat;

        {
            const int num_init_iterations = std::max(seed_count + 1, MinimumInitIterations) - 1;

            GenerateInitialValuePlus(&this->state, 0, seed_count);

            for (int i = 0; i < num_init_iterations; i++) {
                GenerateInitialValuePlus(&this->state, (i + 1) % NumStateWords, (i < seed_count) ? seed[i] : 0);
            }

            for (int i = 0; i < static_cast<int>(NumStateWords); i++) {
                GenerateInitialValueXor(&this->state, (i + 1 + num_init_iterations) % NumStateWords);
            }
        }

        this->FinalizeInitialization();
    }

    void TinyMT::FinalizeInitialization() {
        const u32 state0 = this->state.data[0] & TopBitmask;
        const u32 state1 = this->state.data[1];
        const u32 state2 = this->state.data[2];
        const u32 state3 = this->state.data[3];

        if (state0 == 0 && state1 == 0 && state2 == 0 && state3 == 0) {
            this->state.data[0] = 'T';
            this->state.data[1] = 'I';
            this->state.data[2] = 'N';
            this->state.data[3] = 'Y';
        }

        for (int i = 0; i < NumDiscardedInitOutputs; i++) {
            this->GenerateRandomU32();
        }
    }


    void TinyMT::GetState(TinyMT::State *out) const {
        std::memcpy(out->data, this->state.data, sizeof(this->state));
    }

    void TinyMT::SetState(const TinyMT::State *state) {
        std::memcpy(this->state.data, state->data, sizeof(this->state));
    }

    void TinyMT::GenerateRandomBytes(void *dst, size_t size) {
        const uintptr_t start         = reinterpret_cast<uintptr_t>(dst);
        const uintptr_t end           = start + size;
        const uintptr_t aligned_start = util::AlignUp(start, 4);
        const uintptr_t aligned_end   = util::AlignDown(end, 4);

        /* Make sure we're aligned. */
        if (start < aligned_start) {
            const u32 rnd = this->GenerateRandomU32();
            std::memcpy(dst, &rnd, aligned_start - start);
        }

        /* Write as many aligned u32s as we can. */
        {
            u32 *       cur_dst = reinterpret_cast<u32 *>(aligned_start);
            u32 * const end_dst = reinterpret_cast<u32 *>(aligned_end);

            while (cur_dst < end_dst) {
                *(cur_dst++) = this->GenerateRandomU32();
            }
        }

        /* Handle any leftover unaligned data. */
        if (aligned_end < end) {
            const u32 rnd = this->GenerateRandomU32();
            std::memcpy(reinterpret_cast<void *>(aligned_end), &rnd, end - aligned_end);
        }
    }

    u32  TinyMT::GenerateRandomU32() {
        /* Advance state. */
        const u32 x0 = (this->state.data[0] & TopBitmask) ^ this->state.data[1] ^ this->state.data[2];
        const u32 y0 = this->state.data[3];
        const u32 x1 = x0 ^ (x0 << 1);
        const u32 y1 = y0 ^ (y0 >> 1) ^ x1;

        const u32 state0 = this->state.data[1];
              u32 state1 = this->state.data[2];
              u32 state2 = x1 ^ (y1 << 10);
        const u32 state3 = y1;

        if ((y1 & 1) != 0) {
            state1 ^= ParamMat1;
            state2 ^= ParamMat2;
        }

        this->state.data[0] = state0;
        this->state.data[1] = state1;
        this->state.data[2] = state2;
        this->state.data[3] = state3;

        /* Temper. */
        const u32 t1 = state0 + (state2 >> 8);
              u32 t0 = state3 ^ t1;

        if ((t1 & 1) != 0) {
            t0 ^= ParamTmat;
        }

        return t0;
    }

}