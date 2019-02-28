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

bool DmntCheatVm::DecodeNextOpcode(CheatVmOpcode *out) {
    /* TODO: Parse opcodes */
    return false;
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

void DmntCheatVm::Execute(const CheatProcessMetadata *metadata) {
    CheatVmOpcode cur_opcode;
    u64 kDown = 0;
    
    /* TODO: Get Keys down. */
    
    /* Clear VM state. */
    for (size_t i = 0; i < DmntCheatVm::NumRegisters; i++) {
        this->registers[i] = 0;
        this->loop_tops[i] = 0;
    }
    
    /* Loop until program finishes. */
    while (this->DecodeNextOpcode(&cur_opcode)) {
        switch (cur_opcode.opcode) {
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
        }
    }
}