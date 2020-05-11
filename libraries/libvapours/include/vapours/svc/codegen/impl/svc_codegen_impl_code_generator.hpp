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
#include <vapours/svc/codegen/impl/svc_codegen_impl_common.hpp>

namespace ams::svc::codegen::impl {

    #define SVC_CODEGEN_FOR_I_FROM_0_TO_64(HANDLER, ...)                                                                    \
        HANDLER( 0, ## __VA_ARGS__); HANDLER( 1, ## __VA_ARGS__); HANDLER( 2, ## __VA_ARGS__); HANDLER( 3, ## __VA_ARGS__); \
        HANDLER( 4, ## __VA_ARGS__); HANDLER( 5, ## __VA_ARGS__); HANDLER( 6, ## __VA_ARGS__); HANDLER( 7, ## __VA_ARGS__); \
        HANDLER( 8, ## __VA_ARGS__); HANDLER( 9, ## __VA_ARGS__); HANDLER(10, ## __VA_ARGS__); HANDLER(11, ## __VA_ARGS__); \
        HANDLER(12, ## __VA_ARGS__); HANDLER(13, ## __VA_ARGS__); HANDLER(14, ## __VA_ARGS__); HANDLER(15, ## __VA_ARGS__); \
        HANDLER(16, ## __VA_ARGS__); HANDLER(17, ## __VA_ARGS__); HANDLER(18, ## __VA_ARGS__); HANDLER(19, ## __VA_ARGS__); \
        HANDLER(20, ## __VA_ARGS__); HANDLER(21, ## __VA_ARGS__); HANDLER(22, ## __VA_ARGS__); HANDLER(23, ## __VA_ARGS__); \
        HANDLER(24, ## __VA_ARGS__); HANDLER(25, ## __VA_ARGS__); HANDLER(26, ## __VA_ARGS__); HANDLER(27, ## __VA_ARGS__); \
        HANDLER(28, ## __VA_ARGS__); HANDLER(29, ## __VA_ARGS__); HANDLER(30, ## __VA_ARGS__); HANDLER(31, ## __VA_ARGS__); \
        HANDLER(32, ## __VA_ARGS__); HANDLER(33, ## __VA_ARGS__); HANDLER(34, ## __VA_ARGS__); HANDLER(35, ## __VA_ARGS__); \
        HANDLER(36, ## __VA_ARGS__); HANDLER(37, ## __VA_ARGS__); HANDLER(38, ## __VA_ARGS__); HANDLER(39, ## __VA_ARGS__); \
        HANDLER(40, ## __VA_ARGS__); HANDLER(41, ## __VA_ARGS__); HANDLER(42, ## __VA_ARGS__); HANDLER(43, ## __VA_ARGS__); \
        HANDLER(44, ## __VA_ARGS__); HANDLER(45, ## __VA_ARGS__); HANDLER(46, ## __VA_ARGS__); HANDLER(47, ## __VA_ARGS__); \
        HANDLER(48, ## __VA_ARGS__); HANDLER(49, ## __VA_ARGS__); HANDLER(50, ## __VA_ARGS__); HANDLER(51, ## __VA_ARGS__); \
        HANDLER(52, ## __VA_ARGS__); HANDLER(53, ## __VA_ARGS__); HANDLER(54, ## __VA_ARGS__); HANDLER(55, ## __VA_ARGS__); \
        HANDLER(56, ## __VA_ARGS__); HANDLER(57, ## __VA_ARGS__); HANDLER(58, ## __VA_ARGS__); HANDLER(59, ## __VA_ARGS__); \
        HANDLER(60, ## __VA_ARGS__); HANDLER(61, ## __VA_ARGS__); HANDLER(62, ## __VA_ARGS__); HANDLER(63, ## __VA_ARGS__);


    class Aarch64CodeGenerator {
        private:
            struct RegisterPair {
                size_t First;
                size_t Second;
            };

            template<size_t ...Registers>
            struct RegisterPairHelper;

            template<size_t First, size_t Second, size_t ...Rest>
            struct RegisterPairHelper<First, Second, Rest...> {
                static constexpr size_t PairCount = 1 + RegisterPairHelper<Rest...>::PairCount;
                static constexpr std::array<RegisterPair, PairCount> Pairs = [] {
                    std::array<RegisterPair, PairCount> pairs = {};
                    pairs[0] = RegisterPair{First, Second};
                    if constexpr (RegisterPairHelper<Rest...>::PairCount) {
                        for (size_t i = 0; i < RegisterPairHelper<Rest...>::PairCount; i++) {
                            pairs[1+i] = RegisterPairHelper<Rest...>::Pairs[i];
                        }
                    }
                    return pairs;
                }();
            };

            template<size_t First, size_t Second>
            struct RegisterPairHelper<First, Second> {
                static constexpr size_t PairCount = 1;
                static constexpr std::array<RegisterPair, PairCount> Pairs = { RegisterPair{First, Second} };
            };

            template<size_t First>
            struct RegisterPairHelper<First> {
                static constexpr size_t PairCount = 0;
                static constexpr std::array<RegisterPair, 0> Pairs = {};
            };

            template<size_t Reg>
            static ALWAYS_INLINE void ClearRegister() {
                __asm__ __volatile__("mov     x%c[r], xzr" :: [r]"i"(Reg) : "memory");
            }

            template<size_t Reg>
            static ALWAYS_INLINE void SaveRegister() {
                __asm__ __volatile__("str     x%c[r], [sp, -16]!" :: [r]"i"(Reg) : "memory");
            }

            template<size_t Reg>
            static ALWAYS_INLINE void RestoreRegister() {
                __asm__ __volatile__("ldr     x%c[r], [sp], 16" :: [r]"i"(Reg) : "memory");
            }

            template<size_t Reg0, size_t Reg1>
            static ALWAYS_INLINE void SaveRegisterPair() {
                __asm__ __volatile__("stp     x%c[r0], x%c[r1], [sp, -16]!" :: [r0]"i"(Reg0), [r1]"i"(Reg1) : "memory");
            }

            template<size_t Reg0, size_t Reg1>
            static ALWAYS_INLINE void RestoreRegisterPair() {
                __asm__ __volatile__("ldp     x%c[r0], x%c[r1], [sp], 16" :: [r0]"i"(Reg0), [r1]"i"(Reg1) : "memory");
            }

            template<size_t First, size_t... Rest>
            static ALWAYS_INLINE void SaveRegistersImpl() {
                #define SVC_CODEGEN_HANDLER(n) \
                    do { if constexpr ((63 - n) < Pairs.size()) { SaveRegisterPair<Pairs[(63 - n)].First, Pairs[(63 - n)].Second>(); } } while (0)

                if constexpr (sizeof...(Rest) % 2 == 1) {
                    /* Even number of registers. */
                    constexpr auto Pairs = RegisterPairHelper<First, Rest...>::Pairs;
                    static_assert(Pairs.size() <= 8);
                    SVC_CODEGEN_FOR_I_FROM_0_TO_64(SVC_CODEGEN_HANDLER)
                } else if constexpr (sizeof...(Rest) > 0) {
                    /* Odd number of registers. */
                    constexpr auto Pairs = RegisterPairHelper<Rest...>::Pairs;
                    static_assert(Pairs.size() <= 8);
                    SVC_CODEGEN_FOR_I_FROM_0_TO_64(SVC_CODEGEN_HANDLER)

                    SaveRegister<First>();
                } else {
                    /* Only one register. */
                    SaveRegister<First>();
                }

                #undef SVC_CODEGEN_HANDLER
            }

            template<size_t First, size_t... Rest>
            static ALWAYS_INLINE void RestoreRegistersImpl() {
                #define SVC_CODEGEN_HANDLER(n) \
                    do { if constexpr (n < Pairs.size()) { RestoreRegisterPair<Pairs[n].First, Pairs[n].Second>(); } } while (0)

                if constexpr (sizeof...(Rest) % 2 == 1) {
                    /* Even number of registers. */
                    constexpr auto Pairs = RegisterPairHelper<First, Rest...>::Pairs;
                    static_assert(Pairs.size() <= 8);
                    SVC_CODEGEN_FOR_I_FROM_0_TO_64(SVC_CODEGEN_HANDLER)
                } else if constexpr (sizeof...(Rest) > 0) {
                    /* Odd number of registers. */
                    RestoreRegister<First>();

                    constexpr auto Pairs = RegisterPairHelper<Rest...>::Pairs;
                    static_assert(Pairs.size() <= 8);
                    SVC_CODEGEN_FOR_I_FROM_0_TO_64(SVC_CODEGEN_HANDLER)
                } else {
                    /* Only one register. */
                    RestoreRegister<First>();
                }

                #undef SVC_CODEGEN_HANDLER
            }

        public:
            template<size_t... Registers>
            static ALWAYS_INLINE void SaveRegisters() {
                if constexpr (sizeof...(Registers) > 0) {
                    SaveRegistersImpl<Registers...>();
                }
            }

            template<size_t... Registers>
            static ALWAYS_INLINE void RestoreRegisters() {
                if constexpr (sizeof...(Registers) > 0) {
                    RestoreRegistersImpl<Registers...>();
                }
            }

            template<size_t... Registers>
            static ALWAYS_INLINE void ClearRegisters() {
                static_assert(sizeof...(Registers) <= 8);
                (ClearRegister<Registers>(), ...);
            }

            template<size_t Size>
            static ALWAYS_INLINE void AllocateStackSpace() {
                if constexpr (Size > 0) {
                    __asm__ __volatile__("sub     sp, sp, %c[size]" :: [size]"i"(util::AlignUp(Size, 16)) : "memory");
                }
            }

            template<size_t Size>
            static ALWAYS_INLINE void FreeStackSpace() {
                if constexpr (Size > 0) {
                    __asm__ __volatile__("add     sp, sp, %c[size]" :: [size]"i"(util::AlignUp(Size, 16)) : "memory");
                }
            }

            template<size_t Dst, size_t Src>
            static ALWAYS_INLINE void MoveRegister() {
                __asm__ __volatile__("mov     x%c[dst], x%c[src]" :: [dst]"i"(Dst), [src]"i"(Src) : "memory");
            }

            template<size_t Reg, size_t Offset, size_t Size>
            static ALWAYS_INLINE void LoadFromStack() {
                if constexpr (Size == 4) {
                    __asm__ __volatile__("ldr     w%c[r], [sp, %c[offset]]" :: [r]"i"(Reg), [offset]"i"(Offset) : "memory");
                } else if constexpr (Size == 8) {
                    __asm__ __volatile__("ldr     x%c[r], [sp, %c[offset]]" :: [r]"i"(Reg), [offset]"i"(Offset) : "memory");
                } else {
                    static_assert(Size != Size);
                }
            }

            template<size_t Reg0, size_t Reg1, size_t Offset, size_t Size>
            static ALWAYS_INLINE void LoadPairFromStack() {
                if constexpr (Size == 4) {
                    __asm__ __volatile__("ldp     w%c[r0], w%c[r1], [sp, %c[offset]]" :: [r0]"i"(Reg0), [r1]"i"(Reg1), [offset]"i"(Offset) : "memory");
                } else if constexpr (Size == 8) {
                    __asm__ __volatile__("ldp     x%c[r0], x%c[r1], [sp, %c[offset]]" :: [r0]"i"(Reg0), [r1]"i"(Reg1), [offset]"i"(Offset) : "memory");
                } else {
                    static_assert(Size != Size);
                }
            }

            template<size_t Reg, size_t Offset, size_t Size>
            static ALWAYS_INLINE void StoreToStack() {
                if constexpr (Size == 4) {
                    __asm__ __volatile__("str     w%c[r], [sp, %c[offset]]" :: [r]"i"(Reg), [offset]"i"(Offset) : "memory");
                } else if constexpr (Size == 8) {
                    __asm__ __volatile__("str     x%c[r], [sp, %c[offset]]" :: [r]"i"(Reg), [offset]"i"(Offset) : "memory");
                } else {
                    static_assert(Size != Size);
                }
            }

            template<size_t Reg0, size_t Reg1, size_t Offset, size_t Size>
            static ALWAYS_INLINE void StorePairToStack() {
                if constexpr (Size == 4) {
                    __asm__ __volatile__("stp     w%c[r0], w%c[r1], [sp, %c[offset]]" :: [r0]"i"(Reg0), [r1]"i"(Reg1), [offset]"i"(Offset) : "memory");
                } else if constexpr (Size == 8) {
                    __asm__ __volatile__("stp     x%c[r0], x%c[r1], [sp, %c[offset]]" :: [r0]"i"(Reg0), [r1]"i"(Reg1), [offset]"i"(Offset) : "memory");
                } else {
                    static_assert(Size != Size);
                }
            }

            template<size_t Dst, size_t Low, size_t High>
            static ALWAYS_INLINE void Pack() {
                __asm__ __volatile__("orr     x%c[dst], x%c[low], x%c[high], lsl #32" :: [dst]"i"(Dst), [low]"i"(Low), [high]"i"(High) : "memory");
            }

            template<size_t Low, size_t High, size_t Src>
            static ALWAYS_INLINE void Unpack() {
                if constexpr (Src != Low) {
                    MoveRegister<Src, Low>();
                }

                __asm__ __volatile__("lsr     x%c[high], x%c[src], #32" :: [high]"i"(High), [src]"i"(Src) : "memory");
            }

            template<size_t Dst, size_t Offset>
            static ALWAYS_INLINE void LoadStackAddress() {
                if constexpr (Offset > 0) {
                    __asm__ __volatile__("add     x%c[dst], sp, %c[offset]" :: [dst]"i"(Dst), [offset]"i"(Offset) : "memory");
                } else if constexpr (Offset == 0) {
                    __asm__ __volatile__("mov     x%c[dst], sp" :: [dst]"i"(Dst) : "memory");
                }
            }
    };

    class Aarch32CodeGenerator {
        /* TODO */
    };

    template<typename CodeGenerator, auto MetaCode>
    static ALWAYS_INLINE void GenerateCodeForMetaCode() {
        constexpr size_t NumOperations = MetaCode.GetNumOperations();
        static_assert(NumOperations <= 64);
        #define SVC_CODEGEN_HANDLER(n) do { if constexpr (n < NumOperations) { constexpr auto Operation = MetaCode.GetOperation(n); GenerateCodeForOperation<CodeGenerator, Operation>(); } } while (0)
        SVC_CODEGEN_FOR_I_FROM_0_TO_64(SVC_CODEGEN_HANDLER)
        #undef SVC_CODEGEN_HANDLER
    }

    #undef SVC_CODEGEN_FOR_I_FROM_0_TO_64

}