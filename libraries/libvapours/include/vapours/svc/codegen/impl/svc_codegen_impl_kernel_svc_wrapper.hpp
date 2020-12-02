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
#include <vapours/svc/codegen/impl/svc_codegen_impl_parameter.hpp>
#include <vapours/svc/codegen/impl/svc_codegen_impl_layout.hpp>
#include <vapours/svc/codegen/impl/svc_codegen_impl_meta_code.hpp>
#include <vapours/svc/codegen/impl/svc_codegen_impl_layout_conversion.hpp>
#include <vapours/svc/codegen/impl/svc_codegen_impl_code_generator.hpp>

namespace ams::svc::codegen::impl {

    template<typename, typename, typename, typename, typename>
    class KernelSvcWrapperHelperImpl;

    template<typename _SvcAbiType, typename _UserAbiType, typename _KernelAbiType, typename ReturnType, typename... ArgumentTypes>
    class KernelSvcWrapperHelperImpl<_SvcAbiType, _UserAbiType, _KernelAbiType, ReturnType, std::tuple<ArgumentTypes...>> {
        private:
            static constexpr bool TryToPerformCoalescingOptimizations = true;

            template<MetaCode::OperationKind PairKind, MetaCode::OperationKind SingleKind, size_t InvalidRegisterId, size_t MaxStackIndex>
            static constexpr void CoalesceOperations(MetaCodeGenerator &out_mcg, const std::array<size_t, MaxStackIndex> stack_modified, size_t stack_top) {
                enum class State { WaitingForRegister, ParsingRegister, ParsedRegister, EmittingCode };
                State cur_state = State::WaitingForRegister;
                size_t num_regs = 0;
                size_t registers[2] = { InvalidRegisterId, InvalidRegisterId };
                size_t widths[2] = {};
                size_t index = 0;
                size_t store_base = 0;
                while (index < stack_top) {
                    if (cur_state == State::WaitingForRegister) {
                        while (stack_modified[index] == InvalidRegisterId && index < stack_top) {
                            index++;
                        }
                        cur_state = State::ParsingRegister;
                    } else if (cur_state == State::ParsingRegister) {
                        const size_t start_index = index;
                        if (num_regs == 0) {
                            store_base = start_index;
                        }
                        const size_t reg = stack_modified[index];
                        registers[num_regs] = reg;
                        while (index < stack_top && index < start_index + KernelAbiType::RegisterSize && stack_modified[index] == reg) {
                            widths[num_regs]++;
                            index++;
                        }
                        num_regs++;
                        cur_state = State::ParsedRegister;
                    } else if (cur_state == State::ParsedRegister) {
                        if (num_regs == 2 || stack_modified[index] == InvalidRegisterId) {
                            cur_state = State::EmittingCode;
                        } else {
                            cur_state = State::ParsingRegister;
                        }
                    } else if (cur_state == State::EmittingCode) {
                        /* Emit an operation! */
                        MetaCode::Operation st_op = {};

                        if (num_regs == 2) {
                            if (registers[0] == registers[1]) {
                                std::abort();
                            }
                            if (widths[0] == widths[1]) {
                                st_op.kind = PairKind;
                                st_op.num_parameters = 4;
                                st_op.parameters[0] = registers[0];
                                st_op.parameters[1] = registers[1];
                                st_op.parameters[2] = store_base;
                                st_op.parameters[3] = widths[0];
                            } else {
                                std::abort();
                            }
                        } else if (num_regs == 1) {
                            st_op.kind = SingleKind;
                            st_op.num_parameters = 3;
                            st_op.parameters[0] = registers[0];
                            st_op.parameters[1] = store_base;
                            st_op.parameters[2] = widths[0];
                        } else {
                            std::abort();
                        }

                        out_mcg.AddOperationDirectly(st_op);

                        /* Go back to beginning of parse. */
                        for (size_t i = 0; i < num_regs; i++) {
                            registers[i] = InvalidRegisterId;
                            widths[i] = 0;
                        }
                        num_regs = 0;
                        cur_state = State::WaitingForRegister;
                    } else {
                        std::abort();
                    }
                }

                if (cur_state == State::ParsedRegister) {
                    /* Emit an operation! */
                    if (num_regs == 2 && widths[0] == widths[1]) {
                        MetaCode::Operation st_op = {};
                        st_op.kind = PairKind;
                        st_op.num_parameters = 4;
                        st_op.parameters[0] = registers[0];
                        st_op.parameters[1] = registers[1];
                        st_op.parameters[2] = store_base;
                        st_op.parameters[3] = widths[0];
                        out_mcg.AddOperationDirectly(st_op);
                    } else {
                        for (size_t i = 0; i < num_regs; i++) {
                            MetaCode::Operation st_op = {};
                            st_op.kind = SingleKind;
                            st_op.num_parameters = 3;
                            st_op.parameters[0] = registers[i];
                            st_op.parameters[1] = store_base;
                            st_op.parameters[2] = widths[i];

                            store_base += widths[i];
                            out_mcg.AddOperationDirectly(st_op);
                        }
                    }
                }
            }

            /* Basic optimization of store coalescing. */
            template<typename Conversion, typename... OperationTypes, size_t N>
            static constexpr bool TryPrepareForKernelProcedureToSvcInvocationCoalescing(std::tuple<OperationTypes...>, MetaCodeGenerator &out_mcg, RegisterAllocator<N> &out_allocator) {
                /* For debugging, allow ourselves to disable these optimizations. */
                if constexpr (!TryToPerformCoalescingOptimizations) {
                    return false;
                }

                /* Generate expected code. */
                MetaCodeGenerator mcg;
                RegisterAllocator<N> allocator = out_allocator;
                (Conversion::template GenerateCode<OperationTypes, CodeGenerationKind::PrepareForKernelProcedureToSvcInvocation>(mcg, allocator), ...);
                MetaCode mc = mcg.GetMetaCode();

                /* This is a naive optimization pass. */
                /* We want to reorder code of the form: */
                /*  - Store to Stack sequence 0... */
                /*  - Load Stack Address 0 */
                /*  - Store to Stack 1... */
                /*  - Load Stack Address 1 */
                /* Into the form: */
                /*  - Store to stack Sequence 0 + 1... */
                /*  - Load Stack Address 0 + 1... */
                /* But only if they are semantically equivalent. */

                /* We'll do a simple, naive pass to check if any registers are stored to stack that are modified. */
                /* This shouldn't happen in any cases we care about, so we can probably get away with it. */
                /* TODO: Eventually this should be e.g. operation.ModifiesRegister() / operation.CanReorderBefore() */
                /*       However, this will be more work, and if it's not necessary it can be put off until it is. */
                constexpr size_t MaxStackIndex     = 0x100;
                constexpr size_t InvalidRegisterId = N;
                bool register_modified[N] = {};
                std::array<size_t, N> stack_address_loaded = {};
                for (size_t i = 0; i < N; i++) { stack_address_loaded[i] = MaxStackIndex; }
                std::array<size_t, MaxStackIndex> stack_modified = {};
                for (size_t i = 0; i < MaxStackIndex; i++) { stack_modified[i] = InvalidRegisterId; }
                size_t stack_top = 0;
                for (size_t i = 0; i < mc.GetNumOperations(); i++) {
                    const auto mco = mc.GetOperation(i);
                    if (mco.kind == MetaCode::OperationKind::StoreToStack) {
                        if (register_modified[mco.parameters[0]]) {
                            return false;
                        }
                        const size_t offset = mco.parameters[1];
                        const size_t width = mco.parameters[2] == 0 ? KernelAbiType::RegisterSize : mco.parameters[2];
                        for (size_t j = 0; j < width; j++) {
                            const size_t index = offset + j;
                            if (index >= MaxStackIndex) {
                                std::abort();
                            }
                            if (stack_modified[index] != InvalidRegisterId) {
                                return false;
                            }
                            stack_modified[index] = mco.parameters[0];
                            stack_top = std::max(index + 1, stack_top);
                        }
                    } else if (mco.kind == MetaCode::OperationKind::LoadStackAddress) {
                        if (stack_address_loaded[mco.parameters[0]] != MaxStackIndex) {
                            return false;
                        }
                        if (register_modified[mco.parameters[0]]) {
                            return false;
                        }
                        if (mco.parameters[1] >= MaxStackIndex) {
                            std::abort();
                        }
                        stack_address_loaded[mco.parameters[0]] = mco.parameters[1];
                        register_modified[mco.parameters[0]] = true;
                    } else {
                        /* TODO: Better operation reasoning process. */
                        return false;
                    }
                }

                /* Looks like we can reorder! */
                /* Okay, let's do this the naive way, too. */
                constexpr auto PairKind   = MetaCode::OperationKind::StorePairToStack;
                constexpr auto SingleKind = MetaCode::OperationKind::StoreToStack;
                CoalesceOperations<PairKind, SingleKind, InvalidRegisterId>(out_mcg, stack_modified, stack_top);
                for (size_t i = 0; i < N; i++) {
                    if (stack_address_loaded[i] != MaxStackIndex) {
                        MetaCode::Operation load_op = {};
                        load_op.kind = MetaCode::OperationKind::LoadStackAddress;
                        load_op.num_parameters = 2;
                        load_op.parameters[0] = i;
                        load_op.parameters[1] = stack_address_loaded[i];
                        out_mcg.AddOperationDirectly(load_op);
                    }
                }

                /* Ensure the out allocator state is correct. */
                out_allocator = allocator;

                return true;
            }

            /* Basic optimization of load coalescing. */
            template<typename Conversion, typename... OperationTypes, size_t N>
            static constexpr bool TryKernelProcedureToSvcInvocationCoalescing(std::tuple<OperationTypes...>, MetaCodeGenerator &out_mcg, RegisterAllocator<N> &out_allocator) {
                /* For debugging, allow ourselves to disable these optimizations. */
                if constexpr (!TryToPerformCoalescingOptimizations) {
                    return false;
                }

                /* Generate expected code. */
                MetaCodeGenerator mcg;
                RegisterAllocator<N> allocator = out_allocator;
                (Conversion::template GenerateCode<OperationTypes, CodeGenerationKind::KernelProcedureToSvcInvocation>(mcg, allocator), ...);
                MetaCode mc = mcg.GetMetaCode();

                /* This is a naive optimization pass. */
                /* We want to coalesce all sequential stack loads, if possible. */
                /* But only if they are semantically equivalent. */

                /* We'll do a simple, naive pass to check if any registers are used after being loaded from stack that. */
                /* This shouldn't happen in any cases we care about, so we can probably get away with it. */
                /* TODO: Eventually this should be e.g. operation.ModifiesRegister() / operation.CanReorderBefore() */
                /*       However, this will be more work, and if it's not necessary it can be put off until it is. */
                constexpr size_t MaxStackIndex     = 0x100;
                constexpr size_t InvalidRegisterId = N;
                bool register_modified[N] = {};
                std::array<size_t, N> stack_offset_loaded = {};
                for (size_t i = 0; i < N; i++) { stack_offset_loaded[i] = MaxStackIndex; }
                std::array<size_t, MaxStackIndex> stack_modified = {};
                for (size_t i = 0; i < MaxStackIndex; i++) { stack_modified[i] = InvalidRegisterId; }
                size_t stack_top = 0;
                for (size_t i = 0; i < mc.GetNumOperations(); i++) {
                    const auto mco = mc.GetOperation(i);
                    if (mco.kind == MetaCode::OperationKind::Unpack) {
                        if (register_modified[mco.parameters[0]] || register_modified[mco.parameters[1]] || register_modified[mco.parameters[2]]) {
                            return false;
                        }
                        register_modified[mco.parameters[0]] = true;
                        register_modified[mco.parameters[1]] = true;
                    } else if (mco.kind == MetaCode::OperationKind::LoadFromStack) {
                        if (stack_offset_loaded[mco.parameters[0]] != MaxStackIndex) {
                            return false;
                        }
                        if (register_modified[mco.parameters[0]] != false) {
                            return false;
                        }
                        if (mco.parameters[1] >= MaxStackIndex) {
                            std::abort();
                        }
                        stack_offset_loaded[mco.parameters[0]] = mco.parameters[1];
                        register_modified[mco.parameters[0]] = true;

                        const size_t offset = mco.parameters[1];
                        const size_t width = mco.parameters[2] == 0 ? KernelAbiType::RegisterSize : mco.parameters[2];
                        for (size_t j = 0; j < width; j++) {
                            const size_t index = offset + j;
                            if (index >= MaxStackIndex) {
                                std::abort();
                            }
                            if (stack_modified[index] != InvalidRegisterId) {
                                return false;
                            }
                            stack_modified[index] = mco.parameters[0];
                            stack_top = std::max(index + 1, stack_top);
                        }
                    } else {
                        /* TODO: Better operation reasoning process. */
                        return false;
                    }
                }

                /* Any operations that don't load from stack, we can just re-add. */
                for (size_t i = 0; i < mc.GetNumOperations(); i++) {
                    const auto mco = mc.GetOperation(i);
                    if (mco.kind != MetaCode::OperationKind::LoadFromStack) {
                        out_mcg.AddOperationDirectly(mco);
                    }
                }
                constexpr auto PairKind   = MetaCode::OperationKind::LoadPairFromStack;
                constexpr auto SingleKind = MetaCode::OperationKind::LoadFromStack;
                CoalesceOperations<PairKind, SingleKind, InvalidRegisterId>(out_mcg, stack_modified, stack_top);

                /* Ensure the out allocator state is correct. */
                out_allocator = allocator;

                return true;
            }

            template<typename... T>
            struct TypeIndexFilter {

                template<auto UseArray, typename X, typename Y>
                struct Helper;

                template<auto UseArray, size_t...Index>
                struct Helper<UseArray, std::tuple<>, std::index_sequence<Index...>> {
                    using Type = std::tuple<>;
                };

                template<auto UseArray, typename HeadType, typename... TailType, size_t HeadIndex, size_t... TailIndex>
                struct Helper<UseArray, std::tuple<HeadType, TailType...>, std::index_sequence<HeadIndex, TailIndex...>> {

                    using LastHeadType = std::tuple<HeadType>;
                    using LastNullType = std::tuple<>;

                    using LastType = typename std::conditional<!UseArray[HeadIndex], LastHeadType, LastNullType>::type;

                    using NextTailType = std::tuple<TailType...>;
                    using NextTailSequence  = std::index_sequence<TailIndex...>;

                    using NextType = typename std::conditional<!UseArray[HeadIndex],
                                                               decltype(std::tuple_cat(std::declval<LastHeadType>(), std::declval<typename Helper<UseArray, NextTailType, NextTailSequence>::Type>())),
                                                               decltype(std::tuple_cat(std::declval<LastNullType>(), std::declval<typename Helper<UseArray, NextTailType, NextTailSequence>::Type>()))
                                                              >::type;

                    using Type = typename std::conditional<sizeof...(TailType) == 0, LastType, NextType>::type;

                };

                template<auto UseArray>
                using FilteredTupleType = typename Helper<UseArray, std::tuple<T...>, decltype(std::make_index_sequence<sizeof...(T)>())>::Type;
            };

            template<auto Allocator, typename FirstOperation, typename...OtherOperations>
            static constexpr auto GetModifiedOperations(std::tuple<FirstOperation, OtherOperations...>) {
                constexpr size_t ModifyRegister = [] {
                    auto allocator = Allocator;
                    return allocator.AllocateFirstFree();
                }();

                using ModifiedFirstOperation = typename FirstOperation::template ModifiedType<ModifyRegister>;
                using NewMoveOperation  = typename LayoutConversionBase::template OperationMove<FirstOperation::RegisterSize, FirstOperation::PassedSize, FirstOperation::ProcedureIndex, ModifyRegister>;
                return std::tuple<ModifiedFirstOperation, OtherOperations..., NewMoveOperation>{};
            }

            template<typename Conversion, auto Allocator, typename FirstOperation, typename... OtherOperations>
            static constexpr auto GenerateBeforeOperations(MetaCodeGenerator &mcg, std::tuple<FirstOperation, OtherOperations...> ops) -> RegisterAllocator<Allocator.GetRegisterCount()> {
                constexpr size_t NumOperations = 1 + sizeof...(OtherOperations);
                using OperationsTuple = decltype(ops);
                using FilterHelper = TypeIndexFilter<FirstOperation, OtherOperations...>;

                constexpr auto ProcessOperation = []<typename Operation>(MetaCodeGenerator &pr_mcg, auto &allocator) {
                    if (Conversion::template CanGenerateCode<Operation, CodeGenerationKind::SvcInvocationToKernelProcedure>(allocator)) {
                        Conversion::template GenerateCode<Operation, CodeGenerationKind::SvcInvocationToKernelProcedure>(pr_mcg, allocator);
                        return true;
                    }
                    return false;
                };

                constexpr auto ProcessResults = []<auto AllocatorVal, auto ProcessOp, typename... Operations>(std::tuple<Operations...>) {
                    auto allocator = AllocatorVal;
                    MetaCodeGenerator pr_mcg;
                    auto use_array = std::array<bool, NumOperations>{ ProcessOp.template operator()<Operations>(pr_mcg, allocator)... };
                    return std::make_tuple(use_array, allocator, pr_mcg);
                }.template operator()<Allocator, ProcessOperation>(OperationsTuple{});

                constexpr auto CanGenerate    = std::get<0>(ProcessResults);
                constexpr auto AfterAllocator = std::get<1>(ProcessResults);
                constexpr auto GeneratedCode  = std::get<2>(ProcessResults).GetMetaCode();

                for (size_t i = 0; i < GeneratedCode.GetNumOperations(); i++) {
                    mcg.AddOperationDirectly(GeneratedCode.GetOperation(i));
                }

                using FilteredOperations = typename FilterHelper::FilteredTupleType<CanGenerate>;
                static_assert(std::tuple_size<FilteredOperations>::value <= NumOperations);
                if constexpr (std::tuple_size<FilteredOperations>::value > 0) {
                    if constexpr (std::tuple_size<FilteredOperations>::value != NumOperations) {
                        return GenerateBeforeOperations<Conversion, AfterAllocator>(mcg, FilteredOperations{});
                    } else {
                        /* No progress was made, so we need to make a change. */
                        constexpr auto ModifiedOperations = GetModifiedOperations<AfterAllocator>(FilteredOperations{});
                        return GenerateBeforeOperations<Conversion, AfterAllocator>(mcg, ModifiedOperations);
                    }
                } else {
                    return AfterAllocator;
                }
            }

            static constexpr MetaCode GenerateOriginalBeforeMetaCode() {
                MetaCodeGenerator mcg;
                RegisterAllocator<KernelAbiType::RegisterCount> allocator;
                static_assert(SvcAbiType::RegisterCount == KernelAbiType::RegisterCount);

                /* Reserve registers used by the input layout. */
                constexpr auto InitialAllocator = [] {
                    RegisterAllocator<KernelAbiType::RegisterCount> initial_allocator;
                    for (size_t i = 0; i < SvcAbiType::RegisterCount; i++) {
                        if (Conversion::LayoutForSvc.GetInputLayout().UsesRegister(i)) {
                            initial_allocator.Allocate(i);
                        }
                    }
                    return initial_allocator;
                }();

                /* Save every register that needs to be preserved to the stack. */
                if constexpr (Conversion::NumPreserveRegisters > 0) {
                    [&mcg]<size_t... Is>(std::index_sequence<Is...>) {
                        mcg.template SaveRegisters<Is...>();
                    }(typename Conversion::PreserveRegisters{});
                }

                /* Allocate space on the stack for parameters that need it. */
                if constexpr (UsedStackSpace > 0) {
                    mcg.template AllocateStackSpace<UsedStackSpace>();
                }

                /* Generate code for before operations. */
                if constexpr (Conversion::NumBeforeOperations > 0) {
                    allocator = GenerateBeforeOperations<Conversion, InitialAllocator>(mcg, typename Conversion::BeforeOperations{});
                } else {
                    allocator = InitialAllocator;
                }

                /* Generate code for after operations. */
                if constexpr (Conversion::NumAfterOperations > 0) {
                    if (!TryPrepareForKernelProcedureToSvcInvocationCoalescing<Conversion>(typename Conversion::AfterOperations{}, mcg, allocator)) {
                        /* We're not eligible for the straightforward optimization. */
                        [&mcg, &allocator]<size_t... Is>(std::index_sequence<Is...>) {
                            (Conversion::template GenerateCode<typename std::tuple_element<Is, typename Conversion::AfterOperations>::type, CodeGenerationKind::PrepareForKernelProcedureToSvcInvocation>(mcg, allocator), ...);
                        }(std::make_index_sequence<Conversion::NumAfterOperations>());
                    }
                }

                return mcg.GetMetaCode();
            }
        public:
            using SvcAbiType = _SvcAbiType;
            using UserAbiType = _UserAbiType;
            using KernelAbiType = _KernelAbiType;

            using Conversion = LayoutConversion<SvcAbiType, UserAbiType, KernelAbiType, ReturnType, ArgumentTypes...>;

            static constexpr size_t UsedStackSpace = Conversion::NonAbiUsedStackIndices * KernelAbiType::RegisterSize;

            static constexpr MetaCode OriginalBeforeMetaCode = [] {
                return GenerateOriginalBeforeMetaCode();
            }();

            static constexpr MetaCode OriginalAfterMetaCode = [] {
                MetaCodeGenerator mcg;
                RegisterAllocator<KernelAbiType::RegisterCount> allocator;
                static_assert(SvcAbiType::RegisterCount == KernelAbiType::RegisterCount);

                /* Generate code for after operations. */
                if constexpr (Conversion::NumAfterOperations > 0) {
                    if (!TryKernelProcedureToSvcInvocationCoalescing<Conversion>(typename Conversion::AfterOperations{}, mcg, allocator)) {
                        [&mcg, &allocator]<size_t... Is>(std::index_sequence<Is...>) {
                            (Conversion::template GenerateCode<typename std::tuple_element<Is, typename Conversion::AfterOperations>::type, CodeGenerationKind::KernelProcedureToSvcInvocation>(mcg, allocator), ...);
                        }(std::make_index_sequence<Conversion::NumAfterOperations>());
                    }
                }

                /* Allocate space on the stack for parameters that need it. */
                if constexpr (UsedStackSpace > 0) {
                    mcg.template FreeStackSpace<UsedStackSpace>();
                }

                if constexpr (Conversion::NumClearRegisters > 0) {
                    [&mcg]<size_t... Is>(std::index_sequence<Is...>) {
                        mcg.template ClearRegisters<Is...>();
                    }(typename Conversion::ClearRegisters{});
                }

                /* Restore registers we previously saved to the stack. */
                if constexpr (Conversion::NumPreserveRegisters > 0) {
                    [&mcg]<size_t... Is>(std::index_sequence<Is...>) {
                        mcg.template RestoreRegisters<Is...>();
                    }(typename Conversion::PreserveRegisters{});
                }

                return mcg.GetMetaCode();
            }();

            /* TODO: Implement meta code optimization via separate layer. */
            /*       Right now some basic optimizations are just implemented by the above generators. */
            static constexpr MetaCode OptimizedBeforeMetaCode = OriginalBeforeMetaCode;
            static constexpr MetaCode OptimizedAfterMetaCode  = OriginalAfterMetaCode;
    };

    template<typename _SvcAbiType, typename _UserAbiType, typename _KernelAbiType, auto Function>
    class KernelSvcWrapperHelper {
        private:
            using Traits = FunctionTraits<Function>;
        public:
            using Impl       = KernelSvcWrapperHelperImpl<_SvcAbiType, _UserAbiType, _KernelAbiType, typename Traits::ReturnType, typename Traits::ArgsType>;
            using ReturnType = typename Traits::ReturnType;

            static constexpr bool IsAarch64Kernel = std::is_same<_KernelAbiType, Aarch64Lp64Abi>::value;
            static constexpr bool IsAarch32Kernel = std::is_same<_KernelAbiType, Aarch32Ilp32Abi>::value;
            static_assert(IsAarch64Kernel || IsAarch32Kernel);

            using CodeGenerator = typename std::conditional<IsAarch64Kernel, Aarch64CodeGenerator, Aarch32CodeGenerator>::type;

            static constexpr auto BeforeMetaCode = Impl::OptimizedBeforeMetaCode;
            static constexpr auto AfterMetaCode  = Impl::OptimizedAfterMetaCode;

/* Set omit-frame-pointer to prevent GCC from emitting MOV X29, SP instructions. */
#pragma GCC push_options
#pragma GCC optimize ("-O2")
#pragma GCC optimize ("omit-frame-pointer")

            static ALWAYS_INLINE ReturnType WrapSvcFunction() {
                /* Generate appropriate assembly. */
                GenerateCodeForMetaCode<CodeGenerator, BeforeMetaCode>();
                ON_SCOPE_EXIT { GenerateCodeForMetaCode<CodeGenerator, AfterMetaCode>(); };

                /* Cast the generated function to the generic funciton pointer type. */
                #pragma GCC diagnostic push
                #pragma GCC diagnostic ignored "-Wcast-function-type"
                return reinterpret_cast<ReturnType (*)()>(Function)();
                #pragma GCC diagnostic pop
            }

#pragma GCC pop_options
    };


}