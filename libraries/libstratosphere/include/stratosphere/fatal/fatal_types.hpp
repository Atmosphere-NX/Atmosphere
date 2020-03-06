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
#include <vapours.hpp>
#include <stratosphere/ncm/ncm_ids.hpp>
#include <stratosphere/sf/sf_buffer_tags.hpp>

namespace ams::fatal {

    namespace aarch64 {

        enum RegisterName {
            RegisterName_X0     =  0,
            RegisterName_X1     =  1,
            RegisterName_X2     =  2,
            RegisterName_X3     =  3,
            RegisterName_X4     =  4,
            RegisterName_X5     =  5,
            RegisterName_X6     =  6,
            RegisterName_X7     =  7,
            RegisterName_X8     =  8,
            RegisterName_X9     =  9,
            RegisterName_X10    = 10,
            RegisterName_X11    = 11,
            RegisterName_X12    = 12,
            RegisterName_X13    = 13,
            RegisterName_X14    = 14,
            RegisterName_X15    = 15,
            RegisterName_X16    = 16,
            RegisterName_X17    = 17,
            RegisterName_X18    = 18,
            RegisterName_X19    = 19,
            RegisterName_X20    = 20,
            RegisterName_X21    = 21,
            RegisterName_X22    = 22,
            RegisterName_X23    = 23,
            RegisterName_X24    = 24,
            RegisterName_X25    = 25,
            RegisterName_X26    = 26,
            RegisterName_X27    = 27,
            RegisterName_X28    = 28,
            RegisterName_FP     = 29,
            RegisterName_LR     = 30,

            RegisterName_SP     = 31,
            RegisterName_PC     = 32,

            RegisterName_GeneralPurposeCount,

            RegisterName_PState = 33,
            RegisterName_Afsr0  = 34,
            RegisterName_Afsr1  = 35,
            RegisterName_Esr    = 36,
            RegisterName_Far    = 37,

            RegisterName_Count,
        };

        struct CpuContext {
            using RegisterType = u64;
            static constexpr size_t MaxStackTraceDepth = 0x20;

            static constexpr const char *RegisterNameStrings[RegisterName_Count] = {
                u8"X0",
                u8"X1",
                u8"X2",
                u8"X3",
                u8"X4",
                u8"X5",
                u8"X6",
                u8"X7",
                u8"X8",
                u8"X9",
                u8"X10",
                u8"X11",
                u8"X12",
                u8"X13",
                u8"X14",
                u8"X15",
                u8"X16",
                u8"X17",
                u8"X18",
                u8"X19",
                u8"X20",
                u8"X21",
                u8"X22",
                u8"X23",
                u8"X24",
                u8"X25",
                u8"X26",
                u8"X27",
                u8"X28",
                u8"FP",
                u8"LR",
                u8"SP",
                u8"PC",
                u8"PState",
                u8"Afsr0",
                u8"Afsr1",
                u8"Esr",
                u8"Far",
            };

            /* Registers, exception context. N left names for these fields in fatal .rodata. */
            union {
                struct {
                    union {
                        RegisterType x[RegisterName_GeneralPurposeCount];
                        struct {
                            RegisterType _x[RegisterName_FP];
                            RegisterType fp;
                            RegisterType lr;
                            RegisterType sp;
                            RegisterType pc;
                        };
                    };
                    RegisterType pstate;
                    RegisterType afsr0;
                    RegisterType afsr1;
                    RegisterType esr;
                    RegisterType far;
                };
                RegisterType registers[RegisterName_Count];
            };

            /* Misc. */
            RegisterType stack_trace[MaxStackTraceDepth];
            RegisterType base_address;
            RegisterType register_set_flags;
            u32 stack_trace_size;

            void ClearState() {
                std::memset(this, 0, sizeof(*this));
            }

            void SetProgramIdForAtmosphere(ncm::ProgramId program_id) {
                /* Right now, we mux program ID in through afsr when creport. */
                /* TODO: Better way to do this? */
                this->afsr0 = static_cast<RegisterType>(program_id);
            }

            ncm::ProgramId GetProgramIdForAtmosphere() const {
                return ncm::ProgramId{this->afsr0};
            }

            void SetRegisterValue(RegisterName name, RegisterType value) {
                this->registers[name] = value;
                this->register_set_flags |= (RegisterType(1) << name);
            }

            bool HasRegisterValue(RegisterName name) const {
                return this->register_set_flags & (RegisterType(1) << name);
            }

            void SetBaseAddress(RegisterType base_addr) {
                this->base_address = base_addr;
            }
        };

    }

    namespace aarch32 {

        enum RegisterName {
            RegisterName_R0     =  0,
            RegisterName_R1     =  1,
            RegisterName_R2     =  2,
            RegisterName_R3     =  3,
            RegisterName_R4     =  4,
            RegisterName_R5     =  5,
            RegisterName_R6     =  6,
            RegisterName_R7     =  7,
            RegisterName_R8     =  8,
            RegisterName_R9     =  9,
            RegisterName_R10    = 10,
            RegisterName_FP     = 11,
            RegisterName_IP     = 12,
            RegisterName_LR     = 13,
            RegisterName_SP     = 14,
            RegisterName_PC     = 15,

            RegisterName_GeneralPurposeCount,

            RegisterName_PState = 16,
            RegisterName_Afsr0  = 17,
            RegisterName_Afsr1  = 18,
            RegisterName_Esr    = 29,
            RegisterName_Far    = 20,

            RegisterName_Count,
        };

        struct CpuContext {
            using RegisterType = u32;
            static constexpr size_t MaxStackTraceDepth = 0x20;

            static constexpr const char *RegisterNameStrings[RegisterName_Count] = {
                u8"R0",
                u8"R1",
                u8"R2",
                u8"R3",
                u8"R4",
                u8"R5",
                u8"R6",
                u8"R7",
                u8"R8",
                u8"R9",
                u8"R10",
                u8"FP",
                u8"IP",
                u8"LR",
                u8"SP",
                u8"PC",
                u8"PState",
                u8"Afsr0",
                u8"Afsr1",
                u8"Esr",
                u8"Far",
            };

            /* Registers, exception context. N left names for these fields in fatal .rodata. */
            union {
                struct {
                    union {
                        RegisterType r[RegisterName_GeneralPurposeCount];
                        struct {
                            RegisterType _x[RegisterName_FP];
                            RegisterType fp;
                            RegisterType ip;
                            RegisterType lr;
                            RegisterType sp;
                            RegisterType pc;
                        };
                    };
                    RegisterType pstate;
                    RegisterType afsr0;
                    RegisterType afsr1;
                    RegisterType esr;
                    RegisterType far;
                };
                RegisterType registers[RegisterName_Count];
            };

            /* Misc. Yes, stack_trace_size is really laid out differently than aarch64... */
            RegisterType stack_trace[MaxStackTraceDepth];
            u32 stack_trace_size;
            RegisterType base_address;
            RegisterType register_set_flags;

            void ClearState() {
                std::memset(this, 0, sizeof(*this));
            }

            void SetProgramIdForAtmosphere(ncm::ProgramId program_id) {
                /* Right now, we mux program ID in through afsr when creport. */
                /* TODO: Better way to do this? */
                this->afsr0 = static_cast<RegisterType>(static_cast<u64>(program_id) >> 0);
                this->afsr1 = static_cast<RegisterType>(static_cast<u64>(program_id) >> 32);
            }

            ncm::ProgramId GetProgramIdForAtmosphere() const {
                return ncm::ProgramId{(static_cast<u64>(this->afsr1) << 32ul) | (static_cast<u64>(this->afsr0) << 0ul)};
            }

            void SetRegisterValue(RegisterName name, RegisterType value) {
                this->registers[name] = value;
                this->register_set_flags |= (RegisterType(1) << name);
            }

            bool HasRegisterValue(RegisterName name) const {
                return this->register_set_flags & (RegisterType(1) << name);
            }

            void SetBaseAddress(RegisterType base_addr) {
                this->base_address = base_addr;
            }
        };

    }

    struct CpuContext : sf::LargeData, sf::PrefersMapAliasTransferMode {
        enum Architecture {
            Architecture_Aarch64 = 0,
            Architecture_Aarch32 = 1,
        };

        union {
            aarch64::CpuContext aarch64_ctx;
            aarch32::CpuContext aarch32_ctx;
        };

        Architecture architecture;
        u32 type;

        void ClearState() {
            std::memset(this, 0, sizeof(*this));
        }
    };

    static_assert(std::is_pod<aarch64::CpuContext>::value && sizeof(aarch64::CpuContext) == 0x248, "aarch64::CpuContext definition!");
    static_assert(std::is_pod<aarch32::CpuContext>::value && sizeof(aarch32::CpuContext) == 0xE0,  "aarch32::CpuContext definition!");
    static_assert(std::is_pod<CpuContext>::value          && sizeof(CpuContext) == 0x250,          "CpuContext definition!");

    namespace srv {

        struct ThrowContext {
            Result result;
            ncm::ProgramId program_id;
            char proc_name[0xD];
            bool is_creport;
            CpuContext cpu_ctx;
            bool generate_error_report;
            Event erpt_event;
            Event battery_event;
            size_t stack_dump_size;
            u64 stack_dump_base;
            u8 stack_dump[0x100];
            u64 tls_address;
            u8 tls_dump[0x100];

            void ClearState() {
                this->result = ResultSuccess();
                this->program_id = ncm::ProgramId::Invalid;
                std::memset(this->proc_name, 0, sizeof(this->proc_name));
                this->is_creport = false;
                std::memset(&this->cpu_ctx, 0, sizeof(this->cpu_ctx));
                this->generate_error_report = false;
                std::memset(&this->erpt_event, 0, sizeof(this->erpt_event));
                std::memset(&this->battery_event, 0, sizeof(this->battery_event));
                this->stack_dump_size = 0;
                this->stack_dump_base = 0;
                std::memset(this->stack_dump, 0, sizeof(this->stack_dump));
                this->tls_address = 0;
                std::memset(this->tls_dump, 0, sizeof(this->tls_dump));
            }
        };

    }

}
