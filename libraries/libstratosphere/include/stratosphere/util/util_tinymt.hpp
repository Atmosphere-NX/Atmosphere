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

#pragma once
#include <vapours.hpp>

namespace ams::util {

    /* Implementation of TinyMT (mersenne twister RNG). */
    /* Like Nintendo, we will use the sample parameters. */
    class TinyMT {
        public:
            static constexpr size_t NumStateWords = 4;

            struct State {
                u32 data[NumStateWords];
            };
        private:
            static constexpr u32 ParamMat1  = 0x8F7011EE;
            static constexpr u32 ParamMat2  = 0xFC78FF1F;
            static constexpr u32 ParamTmat  = 0x3793FDFF;

            static constexpr u32 ParamMult  = 0x6C078965;
            static constexpr u32 ParamPlus  = 0x0019660D;
            static constexpr u32 ParamXor   = 0x5D588B65;

            static constexpr u32 TopBitmask = 0x7FFFFFFF;

            static constexpr int MinimumInitIterations   = 8;
            static constexpr int NumDiscardedInitOutputs = 8;
        private:
            State state;
        private:
            /* Internal API. */
            void FinalizeInitialization();

            u32 GenerateRandomU24() { return (this->GenerateRandomU32() >> 8); }

            static void GenerateInitialValuePlus(TinyMT::State *state, int index, u32 value);
            static void GenerateInitialValueXor(TinyMT::State *state, int index);
        public:
            /* Public API. */

            /* Initialization. */
            void Initialize(u32 seed);
            void Initialize(const u32 *seed, int seed_count);

            /* State management. */
            void GetState(TinyMT::State *out) const;
            void SetState(const TinyMT::State *state);

            /* Random generation. */
            void GenerateRandomBytes(void *dst, size_t size);
            u32  GenerateRandomU32();

            inline u64 GenerateRandomU64() {
                const u32 lo = this->GenerateRandomU32();
                const u32 hi = this->GenerateRandomU32();
                return (static_cast<u64>(hi) << 32) | static_cast<u64>(lo);
            }

            inline float  GenerateRandomF32() {
                /* Floats have 24 bits of mantissa. */
                constexpr int MantissaBits = 24;
                return GenerateRandomU24() * (1.0f / (1ul << MantissaBits));
            }

            inline double GenerateRandomF64() {
                /* Doubles have 53 bits of mantissa. */
                /* The smart way to generate 53 bits of random would be to use 32 bits */
                /* from the first rnd32() call, and then 21 from the second. */
                /* Nintendo does not. They use (32 - 5) = 27 bits from the first rnd32() */
                /* call, and (32 - 6) bits from the second. We'll do what they do, but */
                /* There's not a clear reason why. */
                constexpr int MantissaBits = 53;
                constexpr int Shift1st  = (64 - MantissaBits) / 2;
                constexpr int Shift2nd  = (64 - MantissaBits) - Shift1st;

                const u32 first  = (this->GenerateRandomU32() >> Shift1st);
                const u32 second = (this->GenerateRandomU32() >> Shift2nd);

                return (1.0 * first * (1ul << (32 - Shift2nd)) + second) * (1.0 / (1ul << MantissaBits));
            }
    };

}