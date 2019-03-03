/*
 * Copyright (c) 2018 Atmosphère-NX
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
 
#include <switch.h>
#include "dmnt_cheat_types.hpp"
#include "dmnt_cheat_vm.hpp"
#include "dmnt_cheat_manager.hpp"

bool DmntCheatVm::DecodeNextOpcode(CheatVmOpcode *out) {
    /* If we've ever seen a decode failure, return true. */
    bool valid = this->decode_success;
    CheatVmOpcode opcode = {};
    ON_SCOPE_EXIT {
        this->decode_success &= valid;
        if (valid) {
            *out = opcode;
        }
    };
    
    /* If we have ever seen a decode failure, don't decode any more. */
    if (!valid) {
        return valid;
    }
    
    /* Validate instruction pointer. */
    if (this->instruction_ptr >= this->num_opcodes) {
        valid = false;
        return valid;
    }
    
    /* Read opcode. */
    const u32 first_dword = this->program[this->instruction_ptr++];
    opcode.opcode = (CheatVmOpcodeType)(((first_dword >> 28) & 0xF));
    
    switch (opcode.opcode) {
        case CheatVmOpcodeType_StoreStatic:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_BeginConditionalBlock:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_EndConditionalBlock:
            {
                /* There's actually nothing left to process here! */
            }
            break;
        case CheatVmOpcodeType_ControlLoop:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_LoadRegisterStatic:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_LoadRegisterMemory:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_StoreToRegisterAddress:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_PerformArithmeticStatic:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_BeginKeypressConditionalBlock:
            {
                /* TODO */
            }
            break;
        case CheatVmOpcodeType_PerformArithmeticRegister:
            {
                /* TODO */
            }
            break;
        default:
            /* Unrecognized instruction cannot be decoded. */
            valid = false;
            break;
    }
    
    /* End decoding. */
    return valid;
}

void DmntCheatVm::SkipConditionalBlock() {
    CheatVmOpcode skip_opcode;
    while (this->DecodeNextOpcode(&skip_opcode)) {
        /* Decode instructions until we see end of conditional block. */
        /* NOTE: This is broken in gateway's implementation. */
        /* Gateway currently checks for "0x2" instead of "0x20000000" */
        /* In addition, they do a linear scan instead of correctly decoding opcodes. */
        /* This causes issues if "0x2" appears as an immediate in the conditional block... */
        if (skip_opcode.opcode == CheatVmOpcodeType_EndConditionalBlock) {
            break;
        }
    }
}

u64 DmntCheatVm::GetVmInt(VmInt value, u32 bit_width) {
    switch (bit_width) {
        case 1:
            return value.bit8;
        case 2:
            return value.bit16;
        case 4:
            return value.bit32;
        case 8:
            return value.bit64;
        default:
            /* Invalid bit width -> return 0. */
            return 0;
    }
}

u64 DmntCheatVm::GetCheatProcessAddress(const CheatProcessMetadata* metadata, MemoryAccessType mem_type, u64 rel_address) {
    switch (mem_type) {
        case MemoryAccessType_MainNso:
        default:
            return metadata->main_nso_extents.base + rel_address;
        case MemoryAccessType_Heap:
            return metadata->heap_extents.base + rel_address;
    }
}

void DmntCheatVm::ResetState() {
    for (size_t i = 0; i < DmntCheatVm::NumRegisters; i++) {
        this->registers[i] = 0;
        this->loop_tops[i] = 0;
    }
    this->instruction_ptr = 0;
    this->decode_success = true;
}

void DmntCheatVm::Execute(const CheatProcessMetadata *metadata) {
    CheatVmOpcode cur_opcode;
    u64 kDown = 0;
    
    /* TODO: Get Keys down. */
    
    /* Clear VM state. */
    this->ResetState();
    
    /* Loop until program finishes. */
    while (this->DecodeNextOpcode(&cur_opcode)) {
        switch (cur_opcode.opcode) {
            case CheatVmOpcodeType_StoreStatic:
                {
                    /* Calculate address, write value to memory. */
                    u64 dst_address = GetCheatProcessAddress(metadata, cur_opcode.store_static.mem_type, cur_opcode.store_static.rel_address + this->registers[cur_opcode.store_static.offset_register]);
                    u64 dst_value = GetVmInt(cur_opcode.store_static.value, cur_opcode.store_static.bit_width);
                    switch (cur_opcode.store_static.bit_width) {
                        case 1:
                        case 2:
                        case 4:
                        case 8:
                            DmntCheatManager::WriteCheatProcessMemoryForVm(dst_address, &dst_value, cur_opcode.store_static.bit_width);
                            break;
                    }
                }
                break;
            case CheatVmOpcodeType_BeginConditionalBlock:
                {
                    /* Read value from memory. */
                    u64 src_address = GetCheatProcessAddress(metadata, cur_opcode.begin_cond.mem_type, cur_opcode.begin_cond.rel_address);
                    u64 src_value = 0;
                    switch (cur_opcode.store_static.bit_width) {
                        case 1:
                        case 2:
                        case 4:
                        case 8:
                            DmntCheatManager::ReadCheatProcessMemoryForVm(src_address, &src_value, cur_opcode.begin_cond.bit_width);
                            break;
                    }
                    /* Check against condition. */
                    u64 cond_value = GetVmInt(cur_opcode.begin_cond.value, cur_opcode.begin_cond.bit_width);
                    bool cond_met = false;
                    switch (cur_opcode.begin_cond.cond_type) {
                        case ConditionalComparisonType_GT:
                            cond_met = src_value > cond_value;
                            break;
                        case ConditionalComparisonType_GE:
                            cond_met = src_value >= cond_value;
                            break;
                        case ConditionalComparisonType_LT:
                            cond_met = src_value < cond_value;
                            break;
                        case ConditionalComparisonType_LE:
                            cond_met = src_value <= cond_value;
                            break;
                        case ConditionalComparisonType_EQ:
                            cond_met = src_value == cond_value;
                            break;
                        case ConditionalComparisonType_NE:
                            cond_met = src_value != cond_value;
                            break;
                    }
                    /* Skip conditional block if condition not met. */
                    if (!cond_met) {
                        this->SkipConditionalBlock();
                    }
                }
                break;
            case CheatVmOpcodeType_EndConditionalBlock:
                /* There is nothing to do here. Just move on to the next instruction. */
                break;
            case CheatVmOpcodeType_ControlLoop:
                if (cur_opcode.ctrl_loop.start_loop) {
                    /* Start a loop. */
                    this->registers[cur_opcode.ctrl_loop.reg_index] = cur_opcode.ctrl_loop.num_iters;
                    this->loop_tops[cur_opcode.ctrl_loop.reg_index] = this->instruction_ptr;
                } else {
                    /* End a loop. */
                    this->registers[cur_opcode.ctrl_loop.reg_index]--;
                    if (this->registers[cur_opcode.ctrl_loop.reg_index] != 0) {
                        this->instruction_ptr = this->loop_tops[cur_opcode.ctrl_loop.reg_index];
                    }
                }
                break;
            case CheatVmOpcodeType_LoadRegisterStatic:
                /* Set a register to a static value. */
                this->registers[cur_opcode.ldr_static.reg_index] = cur_opcode.ldr_static.value;
                break;
            case CheatVmOpcodeType_LoadRegisterMemory:
                {
                    /* Choose source address. */
                    u64 src_address;
                    if (cur_opcode.ldr_memory.load_from_reg) {
                        src_address = this->registers[cur_opcode.ldr_memory.reg_index] + cur_opcode.ldr_memory.rel_address;
                    } else {
                        src_address = GetCheatProcessAddress(metadata, cur_opcode.ldr_memory.mem_type, cur_opcode.ldr_memory.rel_address);
                    }
                    /* Read into register. Gateway only reads on valid bitwidth. */
                    switch (cur_opcode.ldr_memory.bit_width) {
                        case 1:
                        case 2:
                        case 4:
                        case 8:
                            DmntCheatManager::ReadCheatProcessMemoryForVm(src_address, &this->registers[cur_opcode.ldr_memory.reg_index], cur_opcode.ldr_memory.bit_width);
                            break;
                    }
                }
                break;
            case CheatVmOpcodeType_StoreToRegisterAddress:
                {
                    /* Calculate address. */
                    u64 dst_address = this->registers[cur_opcode.str_regaddr.reg_index];
                    u64 dst_value = cur_opcode.str_regaddr.value;
                    if (cur_opcode.str_regaddr.add_offset_reg) {
                        dst_address += this->registers[cur_opcode.str_regaddr.offset_reg_index];
                    }
                    /* Write value to memory. Gateway only writes on valid bitwidth. */
                    switch (cur_opcode.str_regaddr.bit_width) {
                        case 1:
                        case 2:
                        case 4:
                        case 8:
                            DmntCheatManager::WriteCheatProcessMemoryForVm(dst_address, &dst_value, cur_opcode.str_regaddr.bit_width);
                            break;
                    }
                    /* Increment register if relevant. */
                    if (cur_opcode.str_regaddr.increment_reg) {
                        this->registers[cur_opcode.str_regaddr.reg_index] += cur_opcode.str_regaddr.bit_width;
                    }
                }
                break;
            case CheatVmOpcodeType_PerformArithmeticStatic:
                {
                    /* Do requested math. */
                    switch (cur_opcode.perform_math_static.math_type) {
                        case RegisterArithmeticType_Addition:
                            this->registers[cur_opcode.perform_math_static.reg_index] +=  (u64)cur_opcode.perform_math_static.value;
                            break;
                        case RegisterArithmeticType_Subtraction:
                            this->registers[cur_opcode.perform_math_static.reg_index] -=  (u64)cur_opcode.perform_math_static.value;
                            break;
                        case RegisterArithmeticType_Multiplication:
                            this->registers[cur_opcode.perform_math_static.reg_index] *=  (u64)cur_opcode.perform_math_static.value;
                            break;
                        case RegisterArithmeticType_LeftShift:
                            this->registers[cur_opcode.perform_math_static.reg_index] <<= (u64)cur_opcode.perform_math_static.value;
                            break;
                        case RegisterArithmeticType_RightShift:
                            this->registers[cur_opcode.perform_math_static.reg_index] >>= (u64)cur_opcode.perform_math_static.value;
                            break;
                        default:
                            /* Do not handle extensions here. */
                            break;
                    }
                    /* Apply bit width. */
                    switch (cur_opcode.perform_math_static.bit_width) {
                        case 1:
                            this->registers[cur_opcode.perform_math_static.reg_index] = static_cast<u8>(this->registers[cur_opcode.perform_math_static.reg_index]);
                            break;
                        case 2:
                            this->registers[cur_opcode.perform_math_static.reg_index] = static_cast<u16>(this->registers[cur_opcode.perform_math_static.reg_index]);
                            break;
                        case 4:
                            this->registers[cur_opcode.perform_math_static.reg_index] = static_cast<u32>(this->registers[cur_opcode.perform_math_static.reg_index]);
                            break;
                        case 8:
                            this->registers[cur_opcode.perform_math_static.reg_index] = static_cast<u64>(this->registers[cur_opcode.perform_math_static.reg_index]);
                            break;
                    }
                }
                break;
            case CheatVmOpcodeType_BeginKeypressConditionalBlock:
                /* Check for keypress. */
                if ((cur_opcode.begin_keypress_cond.key_mask & kDown) != cur_opcode.begin_keypress_cond.key_mask) {
                    /* Keys not pressed. Skip conditional block. */
                    this->SkipConditionalBlock();
                }
                break;
            case CheatVmOpcodeType_PerformArithmeticRegister:
                {
                    const u64 operand_1_value = this->registers[cur_opcode.perform_math_reg.src_reg_1_index];
                    const u64 operand_2_value = cur_opcode.perform_math_reg.has_immediate ? 
                                                GetVmInt(cur_opcode.perform_math_reg.value, cur_opcode.perform_math_reg.bit_width) :
                                                this->registers[cur_opcode.perform_math_reg.src_reg_2_index];
                    
                    u64 res_val = 0;
                    /* Do requested math. */
                    switch (cur_opcode.perform_math_reg.math_type) {
                        case RegisterArithmeticType_Addition:
                            res_val = operand_1_value + operand_2_value;
                            break;
                        case RegisterArithmeticType_Subtraction:
                            res_val = operand_1_value - operand_2_value;
                            break;
                        case RegisterArithmeticType_Multiplication:
                            res_val = operand_1_value * operand_2_value;
                            break;
                        case RegisterArithmeticType_LeftShift:
                            res_val = operand_1_value << operand_2_value;
                            break;
                        case RegisterArithmeticType_RightShift:
                            res_val = operand_1_value >> operand_2_value;
                            break;
                        case RegisterArithmeticType_LogicalAnd:
                            res_val = operand_1_value & operand_2_value;
                            break;
                        case RegisterArithmeticType_LogicalOr:
                            res_val = operand_1_value | operand_2_value;
                            break;
                        case RegisterArithmeticType_LogicalNot:
                            res_val = ~operand_1_value;
                            break;
                        case RegisterArithmeticType_LogicalXor:
                            res_val = operand_1_value ^ operand_2_value;
                            break;
                        case RegisterArithmeticType_None:
                            res_val = operand_1_value;
                            break;
                    }
                    
                    
                    /* Apply bit width. */
                    switch (cur_opcode.perform_math_reg.bit_width) {
                        case 1:
                            res_val = static_cast<u8>(res_val);
                            break;
                        case 2:
                            res_val = static_cast<u16>(res_val);
                            break;
                        case 4:
                            res_val = static_cast<u32>(res_val);
                            break;
                        case 8:
                            res_val = static_cast<u64>(res_val);
                            break;
                    }
                    
                    /* Save to register. */
                    this->registers[cur_opcode.perform_math_reg.dst_reg_index] = res_val;
                }
                break;
        }
    }
}