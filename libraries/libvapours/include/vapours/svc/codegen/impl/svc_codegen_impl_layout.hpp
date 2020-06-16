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

namespace ams::svc::codegen::impl {

    class ParameterLayout {
        public:
            static constexpr size_t MaxParameters = 8;
        private:
            static constexpr size_t InvalidIndex = std::numeric_limits<size_t>::max();
        public:
            /* ABI parameters. */
            Abi abi;

            /* Parameter storage. */
            size_t num_parameters;
            Parameter parameters[MaxParameters];
        public:
            constexpr explicit ParameterLayout(Abi a)
                : abi(a), num_parameters(0), parameters()
            { /* ... */ }

            constexpr void AddSingle(Parameter::Identifier id, ArgumentType type, size_t ts, size_t ps, bool p, Storage s, size_t idx) {
                for (size_t i = 0; i < this->num_parameters; i++) {
                    if (this->parameters[i].Is(id)) {
                        this->parameters[i].AddLocation(Location(s, idx));
                        return;
                    }
                }
                this->parameters[this->num_parameters++] = Parameter(id, type, ts, ps, p, Location(s, idx));
            }

            constexpr size_t Add(Parameter::Identifier id, ArgumentType type, size_t ts, size_t ps, bool p, Storage s, size_t i) {
                size_t required_registers = 0;

                while (required_registers * this->abi.register_size < ps) {
                    this->AddSingle(id, type, ts, ps, p, s, i++);
                    required_registers++;
                }

                return required_registers;
            }

            constexpr bool UsesLocation(Location l) const {
                for (size_t i = 0; i < this->num_parameters; i++) {
                    if (this->parameters[i].UsesLocation(l)) {
                        return true;
                    }
                }
                return false;
            }

            constexpr bool UsesRegister(size_t i) const {
                return this->UsesLocation(Location(Storage::Register, i));
            }

            constexpr bool IsRegisterFree(size_t i) const {
                return !(this->UsesRegister(i));
            }

            constexpr size_t GetNumParameters() const {
                return this->num_parameters;
            }

            constexpr Parameter GetParameter(size_t i) const {
                return this->parameters[i];
            }

            constexpr bool HasParameter(Parameter::Identifier id) const {
                for (size_t i = 0; i < this->num_parameters; i++) {
                    if (this->parameters[i].Is(id)) {
                        return true;
                    }
                }
                return false;
            }

            constexpr Parameter GetParameter(Parameter::Identifier id) const {
                for (size_t i = 0; i < this->num_parameters; i++) {
                    if (this->parameters[i].Is(id)) {
                        return this->parameters[i];
                    }
                }
                std::abort();
            }
    };

    class ProcedureLayout {
        public:
            Abi abi;
            ParameterLayout input;
            ParameterLayout output;
        private:
            template<typename AbiType, typename ArgType>
            constexpr void ProcessArgument(size_t i, size_t &NGRN, size_t &NSAA) {
                /* We currently don't implement support for floating point types. */
                static_assert(!std::is_floating_point<ArgType>::value);
                static_assert(!std::is_same<ArgType, void>::value);

                constexpr size_t ArgumentTypeSize = AbiType::template Size<ArgType>;
                constexpr bool   PassedByPointer  = IsPassedByPointer<AbiType, ArgType>;
                constexpr size_t ArgumentPassSize = PassedByPointer ? AbiType::PointerSize : ArgumentTypeSize;

                /* TODO: Is there ever a case where this is not the correct alignment? */
                constexpr size_t ArgumentAlignment = ArgumentPassSize;

                /* Ensure NGRN is aligned. */
                if constexpr (ArgumentAlignment > AbiType::RegisterSize) {
                    NGRN += (NGRN & 1);
                }

                /* TODO: We don't support splitting arguments between registers and stack, but AAPCS32 does. */
                /*       Is this a problem? Nintendo seems to not ever do this. */

                auto id = Parameter::Identifier("FunctionParameter", i);

                /* Allocate integral types specially per aapcs. */
                constexpr ArgumentType Type = GetArgumentType<ArgType>;
                const size_t registers_available = AbiType::RegisterCount - NGRN;
                if constexpr (!PassedByPointer && IsIntegralOrUserPointer<ArgType> && ArgumentTypeSize > AbiType::RegisterSize) {
                    if (registers_available >= 2) {
                        this->input.Add(id, Type, ArgumentTypeSize, ArgumentPassSize, PassedByPointer, Storage::Register, NGRN);
                        NGRN += 2;
                    } else {
                        /* Argument went on stack, so stop allocating arguments in registers. */
                        NGRN = AbiType::RegisterCount;

                        NSAA += (NSAA & 1);
                        this->input.Add(id, Type, ArgumentTypeSize, ArgumentPassSize, PassedByPointer, Storage::Stack, NSAA);
                        NSAA += 2;
                    }
                } else {
                    if (ArgumentPassSize <= AbiType::RegisterSize * registers_available) {
                        NGRN += this->input.Add(id, Type, ArgumentTypeSize, ArgumentPassSize, PassedByPointer, Storage::Register, NGRN);
                    } else {
                        /* Argument went on stack, so stop allocating arguments in registers. */
                        NGRN = AbiType::RegisterCount;

                        /* TODO: Stack pointer alignment is only ensured for aapcs64. */
                        /* What should we do here? */

                        NSAA += this->input.Add(id, Type, ArgumentTypeSize, ArgumentPassSize, PassedByPointer, Storage::Stack, NSAA);
                    }
                }
            }
        public:
            constexpr explicit ProcedureLayout(Abi a) : abi(a), input(a), output(a) { /* ... */ }

            template<typename AbiType, typename ReturnType, typename... ArgumentTypes>
            static constexpr ProcedureLayout Create() {
                ProcedureLayout layout(Abi::Convert<AbiType>());

                /* 1. The Next General-purpose Register Number (NGRN) is set to zero. */
                [[maybe_unused]] size_t NGRN = 0;

                /* 2. The next stacked argument address (NSAA) is set to the current stack-pointer value (SP). */
                [[maybe_unused]] size_t NSAA = 0; /* Should be considered an offset from stack pointer. */

                /* 3. Handle the return type. */
                /* TODO: It's unclear how to handle the non-integral and too-large case. */
                if constexpr (!std::is_same<ReturnType, void>::value) {
                    constexpr size_t ReturnTypeSize = AbiType::template Size<ReturnType>;
                    layout.output.Add(Parameter::Identifier("ReturnType"), ArgumentType::Invalid, ReturnTypeSize, ReturnTypeSize, false, Storage::Register, 0);
                    static_assert(IsIntegral<ReturnType> || ReturnTypeSize <= AbiType::RegisterSize);
                }

                /* Process all arguments, in order. */
                size_t i = 0;
                (layout.ProcessArgument<AbiType, ArgumentTypes>(i++, NGRN, NSAA), ...);

                return layout;
            }

            constexpr ParameterLayout GetInputLayout() const {
                return this->input;
            }

            constexpr ParameterLayout GetOutputLayout() const {
                return this->output;
            }

            constexpr Parameter GetParameter(Parameter::Identifier id) const {
                if (this->input.HasParameter(id)) {
                    return this->input.GetParameter(id);
                } else {
                    return this->output.GetParameter(id);
                }
            }
    };

    class SvcInvocationLayout {
        public:
            Abi abi;
            ParameterLayout input;
            ParameterLayout output;
        private:
            template<typename F>
            constexpr void ForEachInputArgument(ParameterLayout param_layout, F f) {
                /* We want to iterate over the parameters in sorted order. */
                std::array<size_t, ParameterLayout::MaxParameters> map = {};
                const size_t num_parameters = param_layout.GetNumParameters();
                for (size_t i = 0; i < num_parameters; i++) {
                    map[i] = i;
                }
                for (size_t i = 1; i < num_parameters; i++) {
                    for (size_t j = i; j > 0 && param_layout.GetParameter(map[j-1]).GetLocation(0) > param_layout.GetParameter(map[j]).GetLocation(0); j--) {
                        std::swap(map[j], map[j-1]);
                    }
                }

                for (size_t i = 0; i < param_layout.GetNumParameters(); i++) {
                    const auto Parameter = param_layout.GetParameter(map[i]);
                    if (Parameter.GetArgumentType() == ArgumentType::In && !Parameter.IsPassedByPointer()) {
                        f(Parameter);
                    }
                }
                for (size_t i = 0; i < param_layout.GetNumParameters(); i++) {
                    const auto Parameter = param_layout.GetParameter(map[i]);
                    if (Parameter.GetArgumentType() == ArgumentType::InUserPointer) {
                        f(Parameter);
                    }
                }
                for (size_t i = 0; i < param_layout.GetNumParameters(); i++) {
                    const auto Parameter = param_layout.GetParameter(map[i]);
                    if (Parameter.GetArgumentType() == ArgumentType::OutUserPointer) {
                        f(Parameter);
                    }
                }
            }

            template<typename F>
            constexpr void ForEachInputPointerArgument(ParameterLayout param_layout, F f) {
                for (size_t i = 0; i < param_layout.GetNumParameters(); i++) {
                    const auto Parameter = param_layout.GetParameter(i);
                    if (Parameter.GetArgumentType() == ArgumentType::In && Parameter.IsPassedByPointer()) {
                        f(Parameter);
                    }
                }
            }

            template<typename F>
            constexpr void ForEachOutputArgument(ParameterLayout param_layout, F f) {
                for (size_t i = 0; i < param_layout.GetNumParameters(); i++) {
                    const auto Parameter = param_layout.GetParameter(i);
                    if (Parameter.GetArgumentType() == ArgumentType::Out) {
                        f(Parameter);
                    }
                }
            }

            template<size_t N>
            static constexpr void AddRegisterParameter(ParameterLayout &dst_layout, RegisterAllocator<N> &reg_allocator, Parameter param) {
                for (size_t i = 0; i < param.GetNumLocations(); i++) {
                    const auto location = param.GetLocation(i);
                    if (location.GetStorage() == Storage::Register) {
                        reg_allocator.Allocate(location.GetIndex());
                        dst_layout.AddSingle(param.GetIdentifier(), param.GetArgumentType(), param.GetTypeSize(), param.GetPassedSize(), param.IsPassedByPointer(), Storage::Register, location.GetIndex());
                    }
                }
            }

            template<size_t N>
            static constexpr void AddStackParameter(ParameterLayout &dst_layout, RegisterAllocator<N> &reg_allocator, Parameter param) {
                for (size_t i = 0; i < param.GetNumLocations(); i++) {
                    const auto location = param.GetLocation(i);
                    if (location.GetStorage() == Storage::Stack) {
                        const size_t free_reg = reg_allocator.AllocateFirstFree();
                        dst_layout.AddSingle(param.GetIdentifier(), param.GetArgumentType(), param.GetTypeSize(), param.GetPassedSize(), param.IsPassedByPointer(), Storage::Register, free_reg);
                    }
                }
            }

            template<typename AbiType, size_t N>
            static constexpr void AddIndirectParameter(ParameterLayout &dst_layout, RegisterAllocator<N> &reg_allocator, Parameter param) {
                const size_t type_size = param.GetTypeSize();
                for (size_t sz = 0; sz < type_size; sz += AbiType::RegisterSize) {
                    const size_t free_reg = reg_allocator.AllocateFirstFree();
                    dst_layout.AddSingle(param.GetIdentifier(), param.GetArgumentType(), type_size, type_size, false, Storage::Register, free_reg);
                }
            }
        public:
            constexpr explicit SvcInvocationLayout(Abi a) : abi(a), input(a), output(a) { /* ... */ }

            template<typename AbiType>
            static constexpr SvcInvocationLayout Create(ProcedureLayout procedure_layout) {
                SvcInvocationLayout layout(Abi::Convert<AbiType>());
                RegisterAllocator<AbiType::RegisterCount> input_register_allocator, output_register_allocator;

                /* Input first wants to map in register -> register */
                layout.ForEachInputArgument(procedure_layout.GetInputLayout(), [&](Parameter parameter) {
                    AddRegisterParameter(layout.input, input_register_allocator, parameter);
                });

                /* And then input wants to map in stack -> stack */
                layout.ForEachInputArgument(procedure_layout.GetInputLayout(), [&](Parameter parameter) {
                    AddStackParameter(layout.input, input_register_allocator, parameter);
                });

                /* And then input wants to map in indirects -> register */
                layout.ForEachInputPointerArgument(procedure_layout.GetInputLayout(), [&](Parameter parameter) {
                    AddIndirectParameter<AbiType>(layout.input, input_register_allocator, parameter);
                });

                /* Handle the return type. */
                if (procedure_layout.GetOutputLayout().GetNumParameters() > 0) {
                    if (procedure_layout.GetOutputLayout().GetNumParameters() != 1) {
                        std::abort();
                    }
                    const auto return_param = procedure_layout.GetOutputLayout().GetParameter(0);
                    if (return_param.GetIdentifier() != Parameter::Identifier("ReturnType")) {
                        std::abort();
                    }
                    AddRegisterParameter(layout.output, output_register_allocator, return_param);
                }

                /* Handle other outputs. */
                layout.ForEachOutputArgument(procedure_layout.GetInputLayout(), [&](Parameter parameter) {
                    AddIndirectParameter<AbiType>(layout.output, output_register_allocator, parameter);
                });

                return layout;
            }

            constexpr ParameterLayout GetInputLayout() const {
                return this->input;
            }

            constexpr ParameterLayout GetOutputLayout() const {
                return this->output;
            }
    };


}