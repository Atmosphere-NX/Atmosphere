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

namespace ams::svc::codegen::impl {

    class LayoutConversionBase {
        public:
            enum class OperationKind {
                Move,
                LoadAndStore,
                PackAndUnpack,
                Scatter,
                Invalid,
            };

            class OperationMoveImpl;
            class OperationLoadAndStoreImpl;
            class OperationPackAndUnpackImpl;
            class OperationScatterImpl;

            class OperationBase{};

            template<OperationKind _Kind, size_t RS, size_t PS, size_t SO, size_t PIdx, size_t... SIdx>
            class Operation : public OperationBase {
                public:
                    static constexpr OperationKind Kind = _Kind;
                    static constexpr size_t RegisterSize = RS;
                    static constexpr size_t PassedSize = PS;
                    static constexpr size_t StackOffset = SO;
                    static constexpr size_t ProcedureIndex = PIdx;

                    static constexpr size_t NumSvcIndices = sizeof...(SIdx);
                    static constexpr std::array<size_t, sizeof...(SIdx)> SvcIndices = { SIdx... };
                    static constexpr std::index_sequence<SIdx...> SvcIndexSequence = {};

                    template<size_t I>
                    static constexpr size_t SvcIndex = SvcIndices[I];

                    template<typename F>
                    static void ForEachSvcIndex(F f) {
                        (f(SIdx), ...);
                    }

                    using ImplType = typename std::conditional<Kind == OperationKind::Move,          OperationMoveImpl,
                                     typename std::conditional<Kind == OperationKind::LoadAndStore,  OperationLoadAndStoreImpl,
                                     typename std::conditional<Kind == OperationKind::PackAndUnpack, OperationPackAndUnpackImpl,
                                     typename std::conditional<Kind == OperationKind::Scatter,       OperationScatterImpl,
                                              void>::type>::type>::type>::type;

                    template<size_t NPIdx>
                    using ModifiedType = Operation<Kind, RS, PS, SO, NPIdx, SIdx...>;
            };

            template<size_t RS, size_t PS, size_t PIdx, size_t SIdx>
            using OperationMove = Operation<OperationKind::Move, RS, PS, 0, PIdx, SIdx>;

            template<size_t RS, size_t PS, size_t PIdx, size_t SIdx>
            using OperationLoadAndStore = Operation<OperationKind::LoadAndStore, RS, PS, 0, PIdx, SIdx>;

            template<size_t RS, size_t PS, size_t PIdx, size_t SIdx0, size_t SIdx1>
            using OperationPackAndUnpack = Operation<OperationKind::PackAndUnpack, RS, PS, 0, PIdx, SIdx0, SIdx1>;

            template<size_t RS, size_t PS, size_t SO, size_t PIdx, size_t... SIdx>
            using OperationScatter = Operation<OperationKind::Scatter, RS, PS, SO, PIdx, SIdx...>;

            class OperationMoveImpl {
                public:
                    template<typename Operation, size_t N>
                    static constexpr bool CanGenerateCodeForSvcInvocationToKernelProcedure(RegisterAllocator<N> allocator) {
                        static_assert(Operation::Kind == OperationKind::Move);
                        allocator.Free(Operation::template SvcIndex<0>);
                        return allocator.TryAllocate(Operation::ProcedureIndex);
                    }

                    template<typename Operation, size_t N>
                    static constexpr void GenerateCodeForSvcInvocationToKernelProcedure(MetaCodeGenerator &mcg, RegisterAllocator<N> &allocator) {
                        static_assert(Operation::Kind == OperationKind::Move);
                        allocator.Free(Operation::template SvcIndex<0>);
                        allocator.Allocate(Operation::ProcedureIndex);
                        mcg.template MoveRegister<Operation::ProcedureIndex, Operation::template SvcIndex<0>>();
                    }
            };

            class OperationLoadAndStoreImpl {
                public:
                    template<typename Operation, size_t N>
                    static constexpr bool CanGenerateCodeForSvcInvocationToKernelProcedure(RegisterAllocator<N> allocator) {
                        static_assert(Operation::Kind == OperationKind::LoadAndStore);
                        allocator.Free(Operation::template SvcIndex<0>);
                        return true;
                    }

                    template<typename Operation, size_t N>
                    static constexpr void GenerateCodeForSvcInvocationToKernelProcedure(MetaCodeGenerator &mcg, RegisterAllocator<N> &allocator) {
                        static_assert(Operation::Kind == OperationKind::LoadAndStore);
                        allocator.Free(Operation::template SvcIndex<0>);
                        constexpr size_t StackOffset = Operation::ProcedureIndex * Operation::RegisterSize;
                        mcg.template StoreToStack<Operation::template SvcIndex<0>, StackOffset>();
                    }
            };

            class OperationPackAndUnpackImpl {
                public:
                    template<typename Operation, size_t N>
                    static constexpr bool CanGenerateCodeForSvcInvocationToKernelProcedure(RegisterAllocator<N> allocator) {
                        static_assert(Operation::Kind == OperationKind::PackAndUnpack);
                        allocator.Free(Operation::template SvcIndex<0>);
                        allocator.Free(Operation::template SvcIndex<1>);
                        return allocator.TryAllocate(Operation::ProcedureIndex);
                    }

                    template<typename Operation, size_t N>
                    static constexpr void GenerateCodeForSvcInvocationToKernelProcedure(MetaCodeGenerator &mcg, RegisterAllocator<N> &allocator) {
                        static_assert(Operation::Kind == OperationKind::PackAndUnpack);
                        allocator.Free(Operation::template SvcIndex<0>);
                        allocator.Free(Operation::template SvcIndex<1>);
                        allocator.Allocate(Operation::ProcedureIndex);
                        mcg.template Pack<Operation::ProcedureIndex, Operation::template SvcIndex<0>, Operation::template SvcIndex<1>>();
                    }

                    template<typename Operation>
                    static constexpr void GenerateCodeForPrepareForKernelProcedureToSvcInvocation(MetaCodeGenerator &mcg) {
                        static_assert(Operation::Kind == OperationKind::PackAndUnpack);
                        AMS_UNUSED(mcg);
                    }

                    template<typename Operation>
                    static constexpr void GenerateCodeForKernelProcedureToSvcInvocation(MetaCodeGenerator &mcg) {
                        static_assert(Operation::Kind == OperationKind::PackAndUnpack);
                        mcg.template Unpack<Operation::template SvcIndex<0>, Operation::template SvcIndex<1>, Operation::ProcedureIndex>();
                    }
            };

            class OperationScatterImpl {
                public:
                    template<typename Operation, size_t N>
                    static constexpr bool CanGenerateCodeForSvcInvocationToKernelProcedure(RegisterAllocator<N> allocator) {
                        static_assert(Operation::Kind == OperationKind::Scatter);
                        [&allocator]<size_t... SvcIndex>(std::index_sequence<SvcIndex...>) {
                            (allocator.Free(SvcIndex), ...);
                        }(Operation::SvcIndexSequence);
                        return allocator.TryAllocate(Operation::ProcedureIndex);
                    }

                    template<typename Operation, size_t N>
                    static constexpr void GenerateCodeForSvcInvocationToKernelProcedure(MetaCodeGenerator &mcg, RegisterAllocator<N> &allocator) {
                        static_assert(Operation::Kind == OperationKind::Scatter);
                        [&allocator]<size_t... SvcIndex>(std::index_sequence<SvcIndex...>) {
                            (allocator.Free(SvcIndex), ...);
                        }(Operation::SvcIndexSequence);
                        allocator.Allocate(Operation::ProcedureIndex);

                        [&mcg]<size_t... Is>(std::index_sequence<Is...>) {
                            (mcg.template StoreToStack<Operation::template SvcIndex<Is>, Operation::StackOffset + Operation::RegisterSize * (Is), Operation::RegisterSize>(), ...);
                        }(std::make_index_sequence<Operation::NumSvcIndices>());

                        mcg.template LoadStackAddress<Operation::ProcedureIndex, Operation::StackOffset>();
                    }

                    template<typename Operation>
                    static constexpr void GenerateCodeForPrepareForKernelProcedureToSvcInvocation(MetaCodeGenerator &mcg) {
                        static_assert(Operation::Kind == OperationKind::Scatter);

                        [&mcg]<size_t... Is>(std::index_sequence<Is...>) {
                            (mcg.template StoreToStack<Operation::template SvcIndex<Is>, Operation::StackOffset + Operation::RegisterSize * (Is), Operation::RegisterSize>(), ...);
                        }(std::make_index_sequence<Operation::NumSvcIndices>());

                        mcg.template LoadStackAddress<Operation::ProcedureIndex, Operation::StackOffset>();
                    }

                    template<typename Operation>
                    static constexpr void GenerateCodeForKernelProcedureToSvcInvocation(MetaCodeGenerator &mcg) {
                        static_assert(Operation::Kind == OperationKind::Scatter);

                        [&mcg]<size_t... Is>(std::index_sequence<Is...>) {
                            (mcg.template LoadFromStack<Operation::template SvcIndex<Is>, Operation::StackOffset + Operation::RegisterSize * (Is), Operation::RegisterSize>(), ...);
                        }(std::make_index_sequence<Operation::NumSvcIndices>());
                    }
            };
    };

    template<typename _SvcAbiType, typename _UserAbiType, typename _KernelAbiType, typename ReturnType, typename... ArgumentTypes>
    class LayoutConversion {
        public:
            using SvcAbiType = _SvcAbiType;
            using UserAbiType = _UserAbiType;
            using KernelAbiType = _KernelAbiType;

            static constexpr auto LayoutForUser      = ProcedureLayout::Create<UserAbiType, ReturnType, ArgumentTypes...>();
            static constexpr auto LayoutForSvc       = SvcInvocationLayout::Create<SvcAbiType>(LayoutForUser);
            static constexpr auto LayoutForKernel    = ProcedureLayout::Create<KernelAbiType, ReturnType, ArgumentTypes...>();
        private:
            template<bool Input, size_t ParameterIndex = 0, size_t Used = 0>
            static constexpr size_t DetermineUsedStackIndices() {
                [[maybe_unused]] constexpr auto Procedure      = LayoutForKernel;
                [[maybe_unused]] constexpr ParameterLayout Svc = Input ? LayoutForSvc.GetInputLayout() : LayoutForSvc.GetOutputLayout();

                if constexpr (ParameterIndex >= Svc.GetNumParameters()) {
                    /* Base case: we're done. */
                    return Used;
                } else {
                    /* We're processing more parameters. */
                    constexpr Parameter SvcParam       = Svc.GetParameter(ParameterIndex);
                    constexpr Parameter ProcedureParam = Procedure.GetParameter(SvcParam.GetIdentifier());

                    if constexpr (SvcParam.IsPassedByPointer() == ProcedureParam.IsPassedByPointer()) {
                        /* We're not scattering, so stack won't be used. */
                        return DetermineUsedStackIndices<Input, ParameterIndex + 1, Used>();
                    } else {
                        /* We're scattering, and so we're using stack. */
                        static_assert(ProcedureParam.GetNumLocations() == 1);

                        constexpr size_t IndicesPerRegister = KernelAbiType::RegisterSize / SvcAbiType::RegisterSize;
                        static_assert(IndicesPerRegister > 0);

                        constexpr size_t RequiredCount = util::AlignUp(SvcParam.GetNumLocations(), IndicesPerRegister) / IndicesPerRegister;

                        return DetermineUsedStackIndices<Input, ParameterIndex + 1, Used + RequiredCount>();
                    }
                }
            }

            static constexpr size_t AbiUsedStackIndices = [] {
                constexpr auto KernLayout = LayoutForKernel.GetInputLayout();

                size_t used = 0;
                for (size_t i = 0; i < KernLayout.GetNumParameters(); i++) {
                    const auto Param = KernLayout.GetParameter(i);
                    for (size_t j = 0; j < Param.GetNumLocations(); j++) {
                        const auto Loc = Param.GetLocation(j);
                        if (Loc.GetStorage() == Storage::Stack) {
                            used = std::max(used, Loc.GetIndex() + 1);
                        }
                    }
                }

                return used;
            }();

            static constexpr size_t BeforeUsedStackIndices = DetermineUsedStackIndices<true>();
            static constexpr size_t AfterUsedStackIndices  = DetermineUsedStackIndices<false>();

            template<bool Input, size_t ParameterIndex, size_t LocationIndex>
            static constexpr auto ZipMoveOperations() {
                constexpr auto Procedure      = LayoutForKernel;
                constexpr ParameterLayout Svc = Input ? LayoutForSvc.GetInputLayout() : LayoutForSvc.GetOutputLayout();

                static_assert(ParameterIndex < Svc.GetNumParameters());

                constexpr Parameter SvcParam       = Svc.GetParameter(ParameterIndex);
                constexpr Parameter ProcedureParam = Procedure.GetParameter(SvcParam.GetIdentifier());

                static_assert(SvcParam.IsPassedByPointer() == ProcedureParam.IsPassedByPointer());
                static_assert(SvcParam.GetNumLocations() == ProcedureParam.GetNumLocations());

                if constexpr (LocationIndex >= SvcParam.GetNumLocations()) {
                    /* Base case: we're done. */
                    return std::tuple<>{};
                } else {
                    constexpr Location SvcLoc       = SvcParam.GetLocation(LocationIndex);
                    constexpr Location ProcedureLoc = ProcedureParam.GetLocation(LocationIndex);

                    if constexpr (SvcLoc == ProcedureLoc) {
                        /* No need to emit an operation if we're not changing where we are. */
                        return ZipMoveOperations<Input, ParameterIndex, LocationIndex + 1>();
                    } else {
                        /* Svc location needs to be in a register. */
                        static_assert(SvcLoc.GetStorage() == Storage::Register);

                        constexpr size_t Size = KernelAbiType::RegisterSize;

                        if constexpr (ProcedureLoc.GetStorage() == Storage::Register) {
                            using OperationType = LayoutConversionBase::OperationMove<Size, Size, ProcedureLoc.GetIndex(), SvcLoc.GetIndex()>;
                            constexpr auto cur_op = std::make_tuple(OperationType{});
                            return std::tuple_cat(cur_op, ZipMoveOperations<Input, ParameterIndex, LocationIndex + 1>());
                        } else {
                            using OperationType = LayoutConversionBase::OperationLoadAndStore<Size, Size, ProcedureLoc.GetIndex(), SvcLoc.GetIndex()>;
                            constexpr auto cur_op = std::make_tuple(OperationType{});
                            return std::tuple_cat(cur_op, ZipMoveOperations<Input, ParameterIndex, LocationIndex + 1>());
                        }
                    }
                }
            }

            template<bool Input, size_t ParameterIndex, size_t StackIndex>
            static constexpr auto DetermineConversionOperations() {
                [[maybe_unused]] constexpr auto Procedure      = LayoutForKernel;
                [[maybe_unused]] constexpr ParameterLayout Svc = Input ? LayoutForSvc.GetInputLayout() : LayoutForSvc.GetOutputLayout();
                [[maybe_unused]] constexpr std::array<size_t, Svc.GetNumParameters()> ParameterMap = []<auto CapturedSvc>(){
                    /* We want to iterate over the parameters in sorted order. */
                    std::array<size_t, CapturedSvc.GetNumParameters()> map{};
                    const size_t num_parameters = CapturedSvc.GetNumParameters();
                    for (size_t i = 0; i < num_parameters; i++) {
                        map[i] = i;
                    }
                    for (size_t i = 1; i < num_parameters; i++) {
                        for (size_t j = i; j > 0 && CapturedSvc.GetParameter(map[j-1]).GetLocation(0) > CapturedSvc.GetParameter(map[j]).GetLocation(0); j--) {
                            std::swap(map[j], map[j-1]);
                        }
                    }
                    return map;
                }.template operator()<Svc>();

                if constexpr (ParameterIndex >= Svc.GetNumParameters()) {
                    /* Base case: we're done. */
                    if constexpr (Input) {
                        static_assert(StackIndex == BeforeUsedStackIndices + AbiUsedStackIndices);
                    } else {
                        static_assert(StackIndex == AfterUsedStackIndices + BeforeUsedStackIndices + AbiUsedStackIndices);
                    }
                    return std::tuple<>{};
                } else {
                    /* We're processing more parameters. */
                    constexpr Parameter SvcParam       = Svc.GetParameter(ParameterMap[ParameterIndex]);
                    constexpr Parameter ProcedureParam = Procedure.GetParameter(SvcParam.GetIdentifier());

                    if constexpr (SvcParam.IsPassedByPointer() == ProcedureParam.IsPassedByPointer()) {
                        if constexpr (SvcParam.GetNumLocations() == ProcedureParam.GetNumLocations()) {
                            /* Normal moves and loads/stores. */
                            return std::tuple_cat(ZipMoveOperations<Input, ParameterMap[ParameterIndex], 0>(), DetermineConversionOperations<Input, ParameterIndex + 1, StackIndex>());
                        } else {
                            /* We're packing. */
                            /* Make sure we're handling the 2 -> 1 case. */
                            static_assert(SvcParam.GetNumLocations()       == 2);
                            static_assert(ProcedureParam.GetNumLocations() == 1);

                            constexpr Location ProcedureLoc = ProcedureParam.GetLocation(0);
                            constexpr Location SvcLoc0      = SvcParam.GetLocation(0);
                            constexpr Location SvcLoc1      = SvcParam.GetLocation(1);
                            static_assert(ProcedureLoc.GetStorage() == Storage::Register);
                            static_assert(SvcLoc0.GetStorage()      == Storage::Register);
                            static_assert(SvcLoc1.GetStorage()      == Storage::Register);

                            constexpr size_t Size = KernelAbiType::RegisterSize;

                            using OperationType = LayoutConversionBase::OperationPackAndUnpack<Size, Size, ProcedureLoc.GetIndex(), SvcLoc0.GetIndex(), SvcLoc1.GetIndex()>;

                            constexpr auto cur_op = std::make_tuple(OperationType{});

                            return std::tuple_cat(cur_op, DetermineConversionOperations<Input, ParameterIndex + 1, StackIndex>());
                        }
                    } else {
                        /* One operation, since we're scattering. */
                        static_assert(ProcedureParam.GetNumLocations() == 1);
                        constexpr Location ProcedureLoc = ProcedureParam.GetLocation(0);

                        constexpr size_t IndicesPerRegister = KernelAbiType::RegisterSize / SvcAbiType::RegisterSize;
                        static_assert(IndicesPerRegister > 0);

                        constexpr size_t RequiredCount = util::AlignUp(SvcParam.GetNumLocations(), IndicesPerRegister) / IndicesPerRegister;

                        if constexpr (ProcedureLoc.GetStorage() == Storage::Register) {
                            /* Scattering. In register during kernel call. */
                            constexpr size_t RegisterSize = SvcAbiType::RegisterSize;
                            constexpr size_t PassedSize   = ProcedureParam.GetTypeSize();

                            constexpr auto SvcIndexSequence = []<auto CapturedSvcParam, size_t... Is>(std::index_sequence<Is...>) {
                                return std::index_sequence<CapturedSvcParam.GetLocation(Is).GetIndex()...>{};
                            }.template operator()<SvcParam>(std::make_index_sequence<SvcParam.GetNumLocations()>());

                            constexpr auto OperationValue = []<auto CapturedProcedureLoc, size_t... Is>(std::index_sequence<Is...>) {
                                return LayoutConversionBase::OperationScatter<RegisterSize, PassedSize, StackIndex * KernelAbiType::RegisterSize, CapturedProcedureLoc.GetIndex(), Is...>{};
                            }.template operator()<ProcedureLoc>(SvcIndexSequence);

                            constexpr auto cur_op = std::make_tuple(OperationValue);

                            return std::tuple_cat(cur_op, DetermineConversionOperations<Input, ParameterIndex + 1, StackIndex + RequiredCount>());
                        } else {
                            /* TODO: How should on-stack-during-kernel-call be handled? */
                            static_assert(ProcedureLoc.GetStorage() == Storage::Register);
                        }
                    }
                }
            }

            static constexpr size_t PreserveRegisterStartIndex = SvcAbiType::ArgumentRegisterCount;
            static constexpr size_t PreserveRegisterEndIndex   = std::min(KernelAbiType::ArgumentRegisterCount, SvcAbiType::RegisterCount);
            static constexpr size_t ClearRegisterStartIndex    = 0;
            static constexpr size_t ClearRegisterEndIndex      = std::min(KernelAbiType::ArgumentRegisterCount, SvcAbiType::RegisterCount);

            template<size_t Index>
            static constexpr bool ShouldPreserveRegister = (PreserveRegisterStartIndex <= Index && Index < PreserveRegisterEndIndex) &&
                                                           LayoutForSvc.GetInputLayout().IsRegisterFree(Index) && LayoutForSvc.GetOutputLayout().IsRegisterFree(Index);

            template<size_t Index>
            static constexpr bool ShouldClearRegister    = (ClearRegisterStartIndex <= Index && Index < ClearRegisterEndIndex) &&
                                                           LayoutForSvc.GetOutputLayout().IsRegisterFree(Index) && !ShouldPreserveRegister<Index>;

            template<size_t Index = PreserveRegisterStartIndex>
            static constexpr auto DeterminePreserveRegisters() {
                static_assert(PreserveRegisterStartIndex <= Index && Index <= PreserveRegisterEndIndex);

                if constexpr (Index >= PreserveRegisterEndIndex) {
                    /* Base case: we're done. */
                    return std::index_sequence<>{};
                } else {
                    if constexpr (ShouldPreserveRegister<Index>) {
                        /* Preserve this register. */
                        return IndexSequenceCat(std::index_sequence<Index>{}, DeterminePreserveRegisters<Index + 1>());
                    } else {
                        /* We don't need to preserve register, so we can skip onwards. */
                        return IndexSequenceCat(std::index_sequence<>{}, DeterminePreserveRegisters<Index + 1>());
                    }
                }
            }

            template<size_t Index = ClearRegisterStartIndex>
            static constexpr auto DetermineClearRegisters() {
                static_assert(ClearRegisterStartIndex <= Index && Index <= ClearRegisterEndIndex);

                if constexpr (Index >= ClearRegisterEndIndex) {
                    /* Base case: we're done. */
                    return std::index_sequence<>{};
                } else {
                    if constexpr (ShouldClearRegister<Index>) {
                        /* Clear this register. */
                        return IndexSequenceCat(std::index_sequence<Index>{}, DetermineClearRegisters<Index + 1>());
                    } else {
                        /* We don't need to preserve register, so we can skip onwards. */
                        return IndexSequenceCat(std::index_sequence<>{}, DetermineClearRegisters<Index + 1>());
                    }
                }
            }
        public:
            static constexpr size_t NonAbiUsedStackIndices    = AfterUsedStackIndices + BeforeUsedStackIndices;
            using BeforeOperations = decltype(DetermineConversionOperations<true,  0, AbiUsedStackIndices>());
            using AfterOperations  = decltype(DetermineConversionOperations<false, 0, AbiUsedStackIndices + BeforeUsedStackIndices>());

            static constexpr size_t NumBeforeOperations = std::tuple_size<BeforeOperations>::value;
            static constexpr size_t NumAfterOperations  = std::tuple_size<AfterOperations>::value;

            using PreserveRegisters = decltype(DeterminePreserveRegisters());
            using ClearRegisters    = decltype(DetermineClearRegisters());

            static constexpr size_t NumPreserveRegisters = PreserveRegisters::size();
            static constexpr size_t NumClearRegisters    = ClearRegisters::size();

            static constexpr auto   PreserveRegistersArray = ConvertToArray(PreserveRegisters{});
            static constexpr auto   ClearRegistersArray    = ConvertToArray(ClearRegisters{});
        public:
            template<typename Operation, CodeGenerationKind CodeGenKind, size_t N>
            static constexpr bool CanGenerateCode(RegisterAllocator<N> allocator) {
                if constexpr (CodeGenKind == CodeGenerationKind::SvcInvocationToKernelProcedure) {
                    return Operation::ImplType::template CanGenerateCodeForSvcInvocationToKernelProcedure<Operation>(allocator);
                } else {
                    static_assert(CodeGenKind != CodeGenKind, "Invalid CodeGenerationKind");
                }
            }

            template<typename Operation, CodeGenerationKind CodeGenKind, size_t N>
            static constexpr void GenerateCode(MetaCodeGenerator &mcg, RegisterAllocator<N> &allocator) {
                if constexpr (CodeGenKind == CodeGenerationKind::SvcInvocationToKernelProcedure) {
                    Operation::ImplType::template GenerateCodeForSvcInvocationToKernelProcedure<Operation>(mcg, allocator);
                } else if constexpr (CodeGenKind == CodeGenerationKind::PrepareForKernelProcedureToSvcInvocation) {
                    Operation::ImplType::template GenerateCodeForPrepareForKernelProcedureToSvcInvocation<Operation>(mcg);
                } else if constexpr (CodeGenKind == CodeGenerationKind::KernelProcedureToSvcInvocation) {
                    Operation::ImplType::template GenerateCodeForKernelProcedureToSvcInvocation<Operation>(mcg);
                } else {
                    static_assert(CodeGenKind != CodeGenKind, "Invalid CodeGenerationKind");
                }
            }
    };


}