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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/svc/svc_types.hpp>

namespace ams::svc::codegen::impl {

    template<typename T>
    constexpr inline bool IsIntegral = std::is_integral<T>::value;

    template<>
    constexpr inline bool IsIntegral<::ams::svc::Address> = true;

    template<>
    constexpr inline bool IsIntegral<::ams::svc::Size> = true;

    template<typename T>
    constexpr inline bool IsKUserPointer = std::is_base_of<ams::kern::svc::impl::KUserPointerTag, T>::value;

    template<typename T>
    constexpr inline bool IsIntegralOrUserPointer = IsIntegral<T> || IsUserPointer<T> || IsKUserPointer<T>;

    template<size_t... Is1, size_t... Is2>
    constexpr std::index_sequence<Is1..., Is2...> IndexSequenceCat(std::index_sequence<Is1...>, std::index_sequence<Is2...>) {
        return std::index_sequence<Is1..., Is2...>{};
    }

    template<size_t... Is>
    constexpr inline std::array<size_t, sizeof...(Is)> ConvertToArray(std::index_sequence<Is...>) {
        return std::array<size_t, sizeof...(Is)>{ Is... };
    }

    template<auto Function>
    class FunctionTraits {
        private:
            template<typename R, typename... A>
            static R GetReturnTypeImpl(R(*)(A...));

            template<typename R, typename... A>
            static std::tuple<A...> GetArgsImpl(R(*)(A...));
        public:
            using ReturnType        = decltype(GetReturnTypeImpl(Function));
            using ArgsType          = decltype(GetArgsImpl(Function));
    };

    enum class CodeGenerationKind {
        SvcInvocationToKernelProcedure,
        PrepareForKernelProcedureToSvcInvocation,
        KernelProcedureToSvcInvocation,
        Invalid,
    };

    enum class ArgumentType {
        In,
        Out,
        InUserPointer,
        OutUserPointer,
        Invalid,
    };

    template<typename T>
    constexpr inline ArgumentType GetArgumentType = [] {
        static_assert(!std::is_reference<T>::value, "SVC ABI: Reference types not allowed.");
        static_assert(sizeof(T) <= sizeof(uint64_t), "SVC ABI: Type too large");
        if constexpr (std::is_pointer<T>::value) {
            static_assert(!std::is_const<typename std::remove_pointer<T>::type>::value, "SVC ABI: Output (T*) must not be const");
            return ArgumentType::Out;
        } else if constexpr (IsUserPointer<T> || IsKUserPointer<T>) {
            if constexpr (T::IsInput) {
                return ArgumentType::InUserPointer;
            } else {
                return ArgumentType::OutUserPointer;
            }
        } else {
            return ArgumentType::In;
        }
    }();

    template<size_t RS, size_t RC, size_t ARC, size_t PC>
    struct AbiType {
        static constexpr size_t RegisterSize = RS;
        static constexpr size_t RegisterCount = RC;
        static constexpr size_t ArgumentRegisterCount = ARC;
        static constexpr size_t PointerSize = PC;

        template<typename T>
        static constexpr size_t GetSize() {
            if constexpr (std::is_same<T, ::ams::svc::Address>::value || std::is_same<T, ::ams::svc::Size>::value || IsUserPointer<T> || IsKUserPointer<T>) {
                return PointerSize;
            } else if constexpr(std::is_pointer<T>::value) {
                /* Out parameter. */
                return GetSize<typename std::remove_pointer<T>::type>();
            } else if constexpr (std::is_same<T, void>::value) {
                return 0;
            } else {
                return sizeof(T);
            }
        }

        template<typename T>
        static constexpr inline size_t Size = GetSize<T>();
    };

    using Aarch64Lp64Abi  = AbiType<8, 8, 8, 8>;
    using Aarch64Ilp32Abi = AbiType<8, 8, 8, 4>;
    using Aarch32Ilp32Abi = AbiType<4, 4, 4, 4>;

    using Aarch64SvcInvokeAbi = AbiType<8, 8, 8, 8>;
    using Aarch32SvcInvokeAbi = AbiType<4, 8, 4, 4>;

    struct Abi {
        size_t register_size;
        size_t register_count;
        size_t pointer_size;

        template<typename AbiType>
        static constexpr Abi Convert() { return { AbiType::RegisterSize, AbiType::RegisterCount, AbiType::PointerSize }; }
    };

    template<typename AbiType, typename ArgType>
    constexpr inline bool IsPassedByPointer = [] {
        if (GetArgumentType<ArgType> != ArgumentType::In) {
            return true;
        }

        return (!IsIntegral<ArgType> && AbiType::template Size<ArgType> > AbiType::RegisterSize);
    }();

    template<size_t N>
    class RegisterAllocator {
        public:
            std::array<bool, N> map;
        public:
            constexpr explicit RegisterAllocator() : map() { /* ... */ }

            constexpr bool IsAllocated(size_t i) const { return this->map[i]; }
            constexpr bool IsFree(size_t i) const { return !this->IsAllocated(i); }

            constexpr void Allocate(size_t i) {
                if (this->IsAllocated(i)) {
                    std::abort();
                }

                this->map[i] = true;
            }

            constexpr bool TryAllocate(size_t i) {
                if (this->IsAllocated(i)) {
                    return false;
                }

                this->map[i] = true;
                return true;
            }

            constexpr size_t AllocateFirstFree() {
                for (size_t i = 0; i < N; i++) {
                    if (!this->IsAllocated(i)) {
                        this->map[i] = true;
                        return i;
                    }
                }

                std::abort();
            }

            constexpr void Free(size_t i) {
                if (!this->IsAllocated(i)) {
                    std::abort();
                }

                this->map[i] = false;
            }

            constexpr size_t GetRegisterCount() const {
                return N;
            }
    };


}