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

    class MetaCode {
        public:
            static constexpr size_t MaxOperations = 0x40;

            enum class OperationKind {
                SaveRegisters,
                RestoreRegisters,
                ClearRegisters,
                AllocateStackSpace,
                FreeStackSpace,
                MoveRegister,
                LoadFromStack,
                LoadPairFromStack,
                StoreToStack,
                StorePairToStack,
                Pack,
                Unpack,
                LoadStackAddress,
            };

            static constexpr const char *GetOperationKindName(OperationKind k) {
                #define META_CODE_OPERATION_KIND_ENUM_CASE(s) case OperationKind::s: return #s
                switch (k) {
                    META_CODE_OPERATION_KIND_ENUM_CASE(SaveRegisters);
                    META_CODE_OPERATION_KIND_ENUM_CASE(RestoreRegisters);
                    META_CODE_OPERATION_KIND_ENUM_CASE(ClearRegisters);
                    META_CODE_OPERATION_KIND_ENUM_CASE(AllocateStackSpace);
                    META_CODE_OPERATION_KIND_ENUM_CASE(FreeStackSpace);
                    META_CODE_OPERATION_KIND_ENUM_CASE(MoveRegister);
                    META_CODE_OPERATION_KIND_ENUM_CASE(LoadFromStack);
                    META_CODE_OPERATION_KIND_ENUM_CASE(LoadPairFromStack);
                    META_CODE_OPERATION_KIND_ENUM_CASE(StoreToStack);
                    META_CODE_OPERATION_KIND_ENUM_CASE(StorePairToStack);
                    META_CODE_OPERATION_KIND_ENUM_CASE(Pack);
                    META_CODE_OPERATION_KIND_ENUM_CASE(Unpack);
                    META_CODE_OPERATION_KIND_ENUM_CASE(LoadStackAddress);
                    default:
                        std::abort();
                }
                #undef META_CODE_OPERATION_KIND_ENUM_CASE
            }

            struct Operation {
                OperationKind kind;
                size_t num_parameters;
                size_t parameters[16];
            };

            template<OperationKind Kind, size_t... Is>
            static constexpr inline Operation MakeOperation() {
                Operation op = {};
                static_assert(sizeof...(Is) <= sizeof(op.parameters) / sizeof(op.parameters[0]));

                op.kind = Kind;
                op.num_parameters = sizeof...(Is);

                size_t i = 0;
                ((op.parameters[i++] = Is), ...);

                return op;
            }
        public:
            size_t num_operations;
            std::array<Operation, MaxOperations> operations;
        public:
            constexpr explicit MetaCode() : num_operations(0), operations() { /* ... */ }

            constexpr size_t GetNumOperations() const {
                return this->num_operations;
            }

            constexpr Operation GetOperation(size_t i) const {
                return this->operations[i];
            }

            constexpr void AddOperation(Operation op) {
                this->operations[this->num_operations++] = op;
            }
    };

    template<auto Operation>
    static constexpr auto GetOperationParameterSequence() {
        constexpr size_t NumParameters = Operation.num_parameters;

        return []<size_t... Is>(std::index_sequence<Is...>) {
            return std::index_sequence<Operation.parameters[Is]...>{};
        }(std::make_index_sequence<NumParameters>());
    }

    template<typename CodeGenerator, MetaCode::OperationKind Kind, size_t... Parameters>
    static ALWAYS_INLINE void GenerateCodeForOperationImpl(std::index_sequence<Parameters...>) {
        #define META_CODE_OPERATION_KIND_GENERATE_CODE(KIND) else if constexpr (Kind == MetaCode::OperationKind::KIND) { CodeGenerator::template KIND<Parameters...>(); }
        if constexpr (false) { /* ... */ }
        META_CODE_OPERATION_KIND_GENERATE_CODE(SaveRegisters)
        META_CODE_OPERATION_KIND_GENERATE_CODE(RestoreRegisters)
        META_CODE_OPERATION_KIND_GENERATE_CODE(ClearRegisters)
        META_CODE_OPERATION_KIND_GENERATE_CODE(AllocateStackSpace)
        META_CODE_OPERATION_KIND_GENERATE_CODE(FreeStackSpace)
        META_CODE_OPERATION_KIND_GENERATE_CODE(MoveRegister)
        META_CODE_OPERATION_KIND_GENERATE_CODE(LoadFromStack)
        META_CODE_OPERATION_KIND_GENERATE_CODE(LoadPairFromStack)
        META_CODE_OPERATION_KIND_GENERATE_CODE(StoreToStack)
        META_CODE_OPERATION_KIND_GENERATE_CODE(StorePairToStack)
        META_CODE_OPERATION_KIND_GENERATE_CODE(Pack)
        META_CODE_OPERATION_KIND_GENERATE_CODE(Unpack)
        META_CODE_OPERATION_KIND_GENERATE_CODE(LoadStackAddress)
        else { static_assert(Kind != Kind, "Unknown MetaOperationKind"); }
        #undef META_CODE_OPERATION_KIND_GENERATE_CODE
    }

    template<typename CodeGenerator, auto Operation>
    static ALWAYS_INLINE void GenerateCodeForOperation() {
        GenerateCodeForOperationImpl<CodeGenerator, Operation.kind>(GetOperationParameterSequence<Operation>());
    }

    class MetaCodeGenerator {
        private:
            using OperationKind = typename MetaCode::OperationKind;
        private:
            MetaCode meta_code;
        public:
            constexpr explicit MetaCodeGenerator() : meta_code() { /* ... */ }

            constexpr MetaCode GetMetaCode() const {
                return this->meta_code;
            }

            constexpr void AddOperationDirectly(MetaCode::Operation op) {
                this->meta_code.AddOperation(op);
            }

            template<size_t... Registers>
            constexpr void SaveRegisters() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::SaveRegisters, Registers...>();
                this->meta_code.AddOperation(op);
            }

            template<size_t... Registers>
            constexpr void RestoreRegisters() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::RestoreRegisters, Registers...>();
                this->meta_code.AddOperation(op);
            }

            template<size_t... Registers>
            constexpr void ClearRegisters() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::ClearRegisters, Registers...>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Size>
            constexpr void AllocateStackSpace() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::AllocateStackSpace, Size>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Size>
            constexpr void FreeStackSpace() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::FreeStackSpace, Size>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Dst, size_t Src>
            constexpr void MoveRegister() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::MoveRegister, Dst, Src>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Reg, size_t Offset, size_t Size = 0>
            constexpr void LoadFromStack() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::LoadFromStack, Reg, Offset, Size>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Reg0, size_t Reg1, size_t Offset, size_t Size>
            constexpr void LoadPairFromStack() {
                static_assert(Offset % Size == 0);
                constexpr auto op = MetaCode::MakeOperation<OperationKind::LoadPairFromStack, Reg0, Reg1, Offset, Size>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Reg, size_t Offset, size_t Size = 0>
            constexpr void StoreToStack() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::StoreToStack, Reg, Offset, Size>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Reg0, size_t Reg1, size_t Offset, size_t Size>
            constexpr void StorePairToStack() {
                static_assert(Offset % Size == 0);
                constexpr auto op = MetaCode::MakeOperation<OperationKind::StorePairToStack, Reg0, Reg1, Offset, Size>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Dst, size_t Low, size_t High>
            constexpr void Pack() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::Pack, Dst, Low, High>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Low, size_t High, size_t Src>
            constexpr void Unpack() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::Unpack, Low, High, Src>();
                this->meta_code.AddOperation(op);
            }

            template<size_t Dst, size_t Offset>
            constexpr void LoadStackAddress() {
                constexpr auto op = MetaCode::MakeOperation<OperationKind::LoadStackAddress, Dst, Offset>();
                this->meta_code.AddOperation(op);
            }
    };

}