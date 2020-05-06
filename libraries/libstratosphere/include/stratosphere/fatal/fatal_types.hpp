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
                "X0",
                "X1",
                "X2",
                "X3",
                "X4",
                "X5",
                "X6",
                "X7",
                "X8",
                "X9",
                "X10",
                "X11",
                "X12",
                "X13",
                "X14",
                "X15",
                "X16",
                "X17",
                "X18",
                "X19",
                "X20",
                "X21",
                "X22",
                "X23",
                "X24",
                "X25",
                "X26",
                "X27",
                "X28",
                "FP",
                "LR",
                "SP",
                "PC",
                "PState",
                "Afsr0",
                "Afsr1",
                "Esr",
                "Far",
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
                "R0",
                "R1",
                "R2",
                "R3",
                "R4",
                "R5",
                "R6",
                "R7",
                "R8",
                "R9",
                "R10",
                "FP",
                "IP",
                "LR",
                "SP",
                "PC",
                "PState",
                "Afsr0",
                "Afsr1",
                "Esr",
                "Far",
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

    static_assert(util::is_pod<aarch64::CpuContext>::value && sizeof(aarch64::CpuContext) == 0x248, "aarch64::CpuContext definition!");
    static_assert(util::is_pod<aarch32::CpuContext>::value && sizeof(aarch32::CpuContext) == 0xE0,  "aarch32::CpuContext definition!");
    static_assert(util::is_pod<CpuContext>::value          && sizeof(CpuContext) == 0x250,          "CpuContext definition!");

    namespace srv {

        struct ThrowContext {
            Result result;
            ncm::ProgramId program_id;
            char proc_name[0xD];
            bool is_creport;
            CpuContext cpu_ctx;
            bool generate_error_report;
            os::Event *erpt_event;
            os::Event *battery_event;
            size_t stack_dump_size;
            u64 stack_dump_base;
            u8 stack_dump[0x100];
            u64 tls_address;
            u8 tls_dump[0x100];

            ThrowContext(os::Event *erpt, os::Event *bat)
                : result(ResultSuccess()), program_id(), proc_name(), is_creport(), cpu_ctx(), generate_error_report(),
                  erpt_event(erpt), battery_event(bat),
                  stack_dump_size(), stack_dump_base(), stack_dump(), tls_address(), tls_dump()
            {
                /* ... */
            }
        };

    }

}
