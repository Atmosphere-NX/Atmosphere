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

#include <random>
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/rnd.hpp>

namespace sts::rnd {

    namespace {

        /* Generator type. */
        /* Official HOS uses TinyMT. This is high effort. Let's just use XorShift. */
        /* https://en.wikipedia.org/wiki/Xorshift */
        class XorShiftGenerator {
            public:
                using ResultType = uint32_t;
                using result_type = ResultType;
                static constexpr ResultType (min)() { return std::numeric_limits<ResultType>::min(); }
                static constexpr ResultType (max)() { return std::numeric_limits<ResultType>::max(); }
                static constexpr size_t SeedSize = 4;
            private:
                ResultType random_state[SeedSize];
            public:

                explicit XorShiftGenerator() {
                    /* Seed using process entropy. */
                    u64 val = 0;
                    for (size_t i = 0; i < SeedSize; i++) {
                        R_ASSERT(svcGetInfo(&val, InfoType_RandomEntropy, INVALID_HANDLE, i));
                        this->random_state[i] = ResultType(val);
                    }
                }

                explicit XorShiftGenerator(std::random_device &rd) {
                    for (size_t i = 0; i < SeedSize; i++) {
                        this->random_state[i] = ResultType(rd());
                    }
                }

                ResultType operator()() {
                    ResultType s, t = this->random_state[3];
                    t ^= t << 11;
                    t ^= t >> 8;
                    this->random_state[3] = this->random_state[2]; this->random_state[2] = this->random_state[1]; this->random_state[1] = (s = this->random_state[0]);
                    t ^= s;
                    t ^= s >> 19;
                    this->random_state[0] = t;
                    return t;
                }

                void discard(size_t n) {
                    for (size_t i = 0; i < n; i++) {
                        operator()();
                    }
                }
        };

        /* Generator global. */
        XorShiftGenerator g_rnd_generator;

        /* Templated helpers. */
        template<typename T>
        T GenerateRandom(T max = std::numeric_limits<T>::max()) {
            std::uniform_int_distribution<T> rnd(std::numeric_limits<T>::min(), max);
            return rnd(g_rnd_generator);
        }

    }

    void GenerateRandomBytes(void* _out, size_t size) {
        uintptr_t out = reinterpret_cast<uintptr_t>(_out);
        uintptr_t end = out + size;

        /* Force alignment. */
        if (out % sizeof(u16) && out < end) {
            *reinterpret_cast<u8 *>(out) = GenerateRandom<u8>();
            out += sizeof(u8);
        }
        if (out % sizeof(u32) && out < end) {
            *reinterpret_cast<u16 *>(out) = GenerateRandom<u16>();
            out += sizeof(u16);
        }
        if (out % sizeof(u64) && out < end) {
            *reinterpret_cast<u32 *>(out) = GenerateRandom<u32>();
            out += sizeof(u32);
        }

        /* Perform as many aligned writes as possible. */
        while (out + sizeof(u64) <= end) {
            *reinterpret_cast<u64 *>(out) = GenerateRandom<u64>();
            out += sizeof(u64);
        }

        /* Do remainder writes. */
        if (out + sizeof(u32) <= end) {
            *reinterpret_cast<u32 *>(out) = GenerateRandom<u32>();
            out += sizeof(u32);
        }
        if (out + sizeof(u16) <= end) {
            *reinterpret_cast<u16 *>(out) = GenerateRandom<u16>();
            out += sizeof(u16);
        }
        if (out + sizeof(u8) <= end) {
            *reinterpret_cast<u8 *>(out) = GenerateRandom<u8>();
            out += sizeof(u8);
        }
    }

    u32  GenerateRandomU32(u32 max) {
        return GenerateRandom<u32>(max);
    }

    u64  GenerateRandomU64(u64 max) {
        return GenerateRandom<u64>(max);
    }

}
