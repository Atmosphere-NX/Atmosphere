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
#include "dmnt_hid.hpp"


void DmntCheatVm::OpenDebugLogFile() {
    #ifdef DMNT_CHEAT_VM_DEBUG_LOG
    CloseDebugLogFile();
    this->debug_log_file = fopen("cheat_vm_log.txt", "ab");
    #endif
}

void DmntCheatVm::CloseDebugLogFile() {
    #ifdef DMNT_CHEAT_VM_DEBUG_LOG
    if (this->debug_log_file != NULL) {
        fclose(this->debug_log_file);
        this->debug_log_file = NULL;
    }
    #endif
}

void DmntCheatVm::LogToDebugFile(const char *format, ...) {
    #ifdef DMNT_CHEAT_VM_DEBUG_LOG
    if (this->debug_log_file != NULL) {
        va_list arglist;
        va_start(arglist, format);
        vfprintf(this->debug_log_file, format, arglist);
        va_end(arglist);
    }
    #endif
}

void DmntCheatVm::LogOpcode(const CheatVmOpcode *opcode) {
    #ifndef DMNT_CHEAT_VM_DEBUG_LOG
        return;
    #endif
    switch (opcode->opcode) {
        case CheatVmOpcodeType_StoreStatic:
            this->LogToDebugFile("Opcode: Store Static\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->store_static.bit_width);
            this->LogToDebugFile("Mem Type:  %x\n", opcode->store_static.mem_type);
            this->LogToDebugFile("Reg Idx:   %x\n", opcode->store_static.offset_register);
            this->LogToDebugFile("Rel Addr:  %lx\n", opcode->store_static.rel_address);
            this->LogToDebugFile("Value:     %lx\n", opcode->store_static.value.bit64);
            break;
        case CheatVmOpcodeType_BeginConditionalBlock:
            this->LogToDebugFile("Opcode: Begin Conditional\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->begin_cond.bit_width);
            this->LogToDebugFile("Mem Type:  %x\n", opcode->begin_cond.mem_type);
            this->LogToDebugFile("Cond Type: %x\n", opcode->begin_cond.cond_type);
            this->LogToDebugFile("Rel Addr:  %lx\n", opcode->begin_cond.rel_address);
            this->LogToDebugFile("Value:     %lx\n", opcode->begin_cond.value.bit64);
            break;
        case CheatVmOpcodeType_EndConditionalBlock:
            this->LogToDebugFile("Opcode: End Conditional\n");
            break;
        case CheatVmOpcodeType_ControlLoop:
            if (opcode->ctrl_loop.start_loop) {
                this->LogToDebugFile("Opcode: Start Loop\n");
                this->LogToDebugFile("Reg Idx:   %x\n", opcode->ctrl_loop.reg_index);
                this->LogToDebugFile("Num Iters: %x\n", opcode->ctrl_loop.num_iters);
            } else {
                this->LogToDebugFile("Opcode: End Loop\n");
                this->LogToDebugFile("Reg Idx:   %x\n", opcode->ctrl_loop.reg_index);
            }
            break;
        case CheatVmOpcodeType_LoadRegisterStatic:
            this->LogToDebugFile("Opcode: Load Register Static\n");
            this->LogToDebugFile("Reg Idx:   %x\n", opcode->ldr_static.reg_index);
            this->LogToDebugFile("Value:     %lx\n", opcode->ldr_static.value);
            break;
        case CheatVmOpcodeType_LoadRegisterMemory:
            this->LogToDebugFile("Opcode: Load Register Memory\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->ldr_memory.bit_width);
            this->LogToDebugFile("Reg Idx:   %x\n", opcode->ldr_memory.reg_index);
            this->LogToDebugFile("Mem Type:  %x\n", opcode->ldr_memory.mem_type);
            this->LogToDebugFile("From Reg:  %d\n", opcode->ldr_memory.load_from_reg);
            this->LogToDebugFile("Rel Addr:  %lx\n", opcode->ldr_memory.rel_address);
            break;
        case CheatVmOpcodeType_StoreStaticToAddress:
            this->LogToDebugFile("Opcode: Store Static to Address\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->str_static.bit_width);
            this->LogToDebugFile("Reg Idx:   %x\n", opcode->str_static.reg_index);
            if (opcode->str_static.add_offset_reg) {
                this->LogToDebugFile("O Reg Idx: %x\n", opcode->str_static.offset_reg_index);
            }
            this->LogToDebugFile("Incr Reg:  %d\n", opcode->str_static.increment_reg);
            this->LogToDebugFile("Value:     %lx\n", opcode->str_static.value);
            break;
        case CheatVmOpcodeType_PerformArithmeticStatic:
            this->LogToDebugFile("Opcode: Perform Static Arithmetic\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->perform_math_static.bit_width);
            this->LogToDebugFile("Reg Idx:   %x\n", opcode->perform_math_static.reg_index);
            this->LogToDebugFile("Math Type: %x\n", opcode->perform_math_static.math_type);
            this->LogToDebugFile("Value:     %lx\n", opcode->perform_math_static.value);
            break;
        case CheatVmOpcodeType_BeginKeypressConditionalBlock:
            this->LogToDebugFile("Opcode: Begin Keypress Conditional\n");
            this->LogToDebugFile("Key Mask:  %x\n", opcode->begin_keypress_cond.key_mask);
            break;
        case CheatVmOpcodeType_PerformArithmeticRegister:
            this->LogToDebugFile("Opcode: Perform Register Arithmetic\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->perform_math_reg.bit_width);
            this->LogToDebugFile("Dst Idx:   %x\n", opcode->perform_math_reg.dst_reg_index);
            this->LogToDebugFile("Src1 Idx:  %x\n", opcode->perform_math_reg.src_reg_1_index);
            if (opcode->perform_math_reg.has_immediate) {
                this->LogToDebugFile("Value:     %lx\n", opcode->perform_math_reg.value.bit64);
            } else {
                this->LogToDebugFile("Src2 Idx:  %x\n", opcode->perform_math_reg.src_reg_2_index);
            }
            break;
        case CheatVmOpcodeType_StoreRegisterToAddress:
            this->LogToDebugFile("Opcode: Store Register to Address\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->str_register.bit_width);
            this->LogToDebugFile("S Reg Idx: %x\n", opcode->str_register.str_reg_index);
            this->LogToDebugFile("A Reg Idx: %x\n", opcode->str_register.addr_reg_index);
            this->LogToDebugFile("Incr Reg:  %d\n", opcode->str_register.increment_reg);
            switch (opcode->str_register.ofs_type) {
                case StoreRegisterOffsetType_None:
                    break;
                case StoreRegisterOffsetType_Reg:
                    this->LogToDebugFile("O Reg Idx: %x\n", opcode->str_register.ofs_reg_index);
                    break;
                case StoreRegisterOffsetType_Imm:
                    this->LogToDebugFile("Rel Addr:  %lx\n", opcode->str_register.rel_address);
                    break;
                case StoreRegisterOffsetType_MemReg:
                    this->LogToDebugFile("Mem Type:  %x\n", opcode->str_register.mem_type);
                    break;
                case StoreRegisterOffsetType_MemImm:
                case StoreRegisterOffsetType_MemImmReg:
                    this->LogToDebugFile("Mem Type:  %x\n", opcode->str_register.mem_type);
                    this->LogToDebugFile("Rel Addr:  %lx\n", opcode->str_register.rel_address);
                    break;
            }
            break;
        case CheatVmOpcodeType_BeginRegisterConditionalBlock: 
            this->LogToDebugFile("Opcode: Begin Register Conditional\n");
            this->LogToDebugFile("Bit Width: %x\n", opcode->begin_reg_cond.bit_width);
            this->LogToDebugFile("Cond Type: %x\n", opcode->begin_reg_cond.cond_type);
            this->LogToDebugFile("V Reg Idx: %x\n", opcode->begin_reg_cond.val_reg_index);
                switch (opcode->begin_reg_cond.comp_type) {
                    case CompareRegisterValueType_StaticValue:
                        this->LogToDebugFile("Comp Type: Static Value\n");
                        this->LogToDebugFile("Value:     %lx\n", opcode->begin_reg_cond.value.bit64);
                        break;
                    case CompareRegisterValueType_OtherRegister:
                        this->LogToDebugFile("Comp Type: Other Register\n");
                        this->LogToDebugFile("X Reg Idx: %x\n", opcode->begin_reg_cond.other_reg_index);
                        break;
                    case CompareRegisterValueType_MemoryRelAddr:
                        this->LogToDebugFile("Comp Type: Memory Relative Address\n");
                        this->LogToDebugFile("Mem Type:  %x\n", opcode->begin_reg_cond.mem_type);
                        this->LogToDebugFile("Rel Addr:  %lx\n", opcode->begin_reg_cond.rel_address);
                        break;
                    case CompareRegisterValueType_MemoryOfsReg:
                        this->LogToDebugFile("Comp Type: Memory Offset Register\n");
                        this->LogToDebugFile("Mem Type:  %x\n", opcode->begin_reg_cond.mem_type);
                        this->LogToDebugFile("O Reg Idx: %x\n", opcode->begin_reg_cond.ofs_reg_index);
                        break;
                    case CompareRegisterValueType_RegisterRelAddr:
                        this->LogToDebugFile("Comp Type: Register Relative Address\n");
                        this->LogToDebugFile("A Reg Idx: %x\n", opcode->begin_reg_cond.addr_reg_index);
                        this->LogToDebugFile("Rel Addr:  %lx\n", opcode->begin_reg_cond.rel_address);
                        break;
                    case CompareRegisterValueType_RegisterOfsReg:
                        this->LogToDebugFile("Comp Type: Register Offset Register\n");
                        this->LogToDebugFile("A Reg Idx: %x\n", opcode->begin_reg_cond.addr_reg_index);
                        this->LogToDebugFile("O Reg Idx: %x\n", opcode->begin_reg_cond.ofs_reg_index);
                        break;
                }
            break;
        case CheatVmOpcodeType_SaveRestoreRegister: 
            this->LogToDebugFile("Opcode: Save or Restore Register\n");
            this->LogToDebugFile("Dst Idx:   %x\n", opcode->save_restore_reg.dst_index);
            this->LogToDebugFile("Src Idx:   %x\n", opcode->save_restore_reg.src_index);
            this->LogToDebugFile("Is Save:   %d\n", opcode->save_restore_reg.is_save);
            break;
        case CheatVmOpcodeType_SaveRestoreRegisterMask: 
            this->LogToDebugFile("Opcode: Save or Restore Register Mask\n");
            this->LogToDebugFile("Is Save:   %d\n", opcode->save_restore_regmask.is_save);
            for (size_t i = 0; i < NumRegisters; i++) {
                this->LogToDebugFile("Act[%02x]:   %d\n", i, opcode->save_restore_regmask.should_save_or_restore[i]);
            }
            break;
        default:
            this->LogToDebugFile("Unknown opcode: %x\n", opcode->opcode);
            break;
    }
}

bool DmntCheatVm::DecodeNextOpcode(CheatVmOpcode *out) {
    /* If we've ever seen a decode failure, return false. */
    bool valid = this->decode_success;
    CheatVmOpcode opcode = {};
    ON_SCOPE_EXIT {
        this->decode_success &= valid;
        if (valid) {
            *out = opcode;
        }
    };
    
    /* Helper function for getting instruction dwords. */
    auto GetNextDword = [&]() {
        if (this->instruction_ptr >= this->num_opcodes) {
            valid = false;
            return static_cast<u32>(0);
        }
        return this->program[this->instruction_ptr++];
    };
    
    /* Helper function for parsing a VmInt. */
    auto GetNextVmInt = [&](const u32 bit_width) {
        VmInt val = {0};
        
        const u32 first_dword = GetNextDword();
        switch (bit_width) {
            case 1:
                val.bit8 = (u8)first_dword;
                break;
            case 2:
                val.bit16 = (u16)first_dword;
                break;
            case 4:
                val.bit32 = first_dword;
                break;
            case 8:
                val.bit64 = (((u64)first_dword) << 32ul) | ((u64)GetNextDword());
                break;
        }
        
        return val;
    };
    
    /* Read opcode. */
    const u32 first_dword = GetNextDword();
    if (!valid) {
        return valid;
    }
    
    opcode.opcode = (CheatVmOpcodeType)(((first_dword >> 28) & 0xF));
    if (opcode.opcode >= CheatVmOpcodeType_ExtendedWidth) {
        opcode.opcode = (CheatVmOpcodeType)((((u32)opcode.opcode) << 4) | ((first_dword >> 24) & 0xF));
    }
    
    /* detect condition start. */
    switch (opcode.opcode) {
        case CheatVmOpcodeType_BeginConditionalBlock:
        case CheatVmOpcodeType_BeginKeypressConditionalBlock:
        case CheatVmOpcodeType_BeginRegisterConditionalBlock:
            opcode.begin_conditional_block = true;
            break;
        default:
            opcode.begin_conditional_block = false;
            break;
    }
    
    switch (opcode.opcode) {
        case CheatVmOpcodeType_StoreStatic:
            {
                /* 0TMR00AA AAAAAAAA YYYYYYYY (YYYYYYYY) */
                /* Read additional words. */
                const u32 second_dword = GetNextDword();
                opcode.store_static.bit_width = (first_dword >> 24) & 0xF;
                opcode.store_static.mem_type = (MemoryAccessType)((first_dword >> 20) & 0xF);
                opcode.store_static.offset_register = ((first_dword >> 16) & 0xF);
                opcode.store_static.rel_address = ((u64)(first_dword & 0xFF) << 32ul) | ((u64)second_dword);
                opcode.store_static.value = GetNextVmInt(opcode.store_static.bit_width);
            }
            break;
        case CheatVmOpcodeType_BeginConditionalBlock:
            {
                /* 1TMC00AA AAAAAAAA YYYYYYYY (YYYYYYYY) */
                /* Read additional words. */
                const u32 second_dword = GetNextDword();
                opcode.begin_cond.bit_width = (first_dword >> 24) & 0xF;
                opcode.begin_cond.mem_type = (MemoryAccessType)((first_dword >> 20) & 0xF);
                opcode.begin_cond.cond_type = (ConditionalComparisonType)((first_dword >> 16) & 0xF);
                opcode.begin_cond.rel_address = ((u64)(first_dword & 0xFF) << 32ul) | ((u64)second_dword);
                opcode.begin_cond.value = GetNextVmInt(opcode.store_static.bit_width);
            }
            break;
        case CheatVmOpcodeType_EndConditionalBlock:
            {
                /* 20000000 */
                /* There's actually nothing left to process here! */
            }
            break;
        case CheatVmOpcodeType_ControlLoop:
            {
                /* 300R0000 VVVVVVVV */
                /* 310R0000 */
                /* Parse register, whether loop start or loop end. */
                opcode.ctrl_loop.start_loop = ((first_dword >> 24) & 0xF) == 0;
                opcode.ctrl_loop.reg_index = ((first_dword >> 20) & 0xF);
                
                /* Read number of iters if loop start. */
                if (opcode.ctrl_loop.start_loop) {
                    opcode.ctrl_loop.num_iters = GetNextDword();
                }
            }
            break;
        case CheatVmOpcodeType_LoadRegisterStatic:
            {
                /* 400R0000 VVVVVVVV VVVVVVVV */
                /* Read additional words. */
                opcode.ldr_static.reg_index = ((first_dword >> 16) & 0xF);
                opcode.ldr_static.value = (((u64)GetNextDword()) << 32ul) | ((u64)GetNextDword());
            }
            break;
        case CheatVmOpcodeType_LoadRegisterMemory:
            {
                /* 5TMRI0AA AAAAAAAA */
                /* Read additional words. */
                const u32 second_dword = GetNextDword();
                opcode.ldr_memory.bit_width = (first_dword >> 24) & 0xF;
                opcode.ldr_memory.mem_type = (MemoryAccessType)((first_dword >> 20) & 0xF);
                opcode.ldr_memory.reg_index = ((first_dword >> 16) & 0xF);
                opcode.ldr_memory.load_from_reg = ((first_dword >> 12) & 0xF) != 0;
                opcode.ldr_memory.rel_address = ((u64)(first_dword & 0xFF) << 32ul) | ((u64)second_dword);
            }
            break;
        case CheatVmOpcodeType_StoreStaticToAddress:
            {
                /* 6T0RIor0 VVVVVVVV VVVVVVVV */
                /* Read additional words. */
                opcode.str_static.bit_width = (first_dword >> 24) & 0xF;
                opcode.str_static.reg_index = ((first_dword >> 16) & 0xF);
                opcode.str_static.increment_reg = ((first_dword >> 12) & 0xF) != 0;
                opcode.str_static.add_offset_reg = ((first_dword >> 8) & 0xF) != 0;
                opcode.str_static.offset_reg_index = ((first_dword >> 4) & 0xF);
                opcode.str_static.value = (((u64)GetNextDword()) << 32ul) | ((u64)GetNextDword());
            }
            break;
        case CheatVmOpcodeType_PerformArithmeticStatic:
            {
                /* 7T0RC000 VVVVVVVV */
                /* Read additional words. */
                opcode.perform_math_static.bit_width = (first_dword >> 24) & 0xF;
                opcode.perform_math_static.reg_index = ((first_dword >> 16) & 0xF);
                opcode.perform_math_static.math_type = (RegisterArithmeticType)((first_dword >> 12) & 0xF);
                opcode.perform_math_static.value = GetNextDword();
            }
            break;
        case CheatVmOpcodeType_BeginKeypressConditionalBlock:
            {
                /* 8kkkkkkk */
                /* Just parse the mask. */
                opcode.begin_keypress_cond.key_mask = first_dword & 0x0FFFFFFF;
            }
            break;
        case CheatVmOpcodeType_PerformArithmeticRegister:
            {
                /* 9TCRSIs0 (VVVVVVVV (VVVVVVVV)) */
                opcode.perform_math_reg.bit_width = (first_dword >> 24) & 0xF;
                opcode.perform_math_reg.math_type = (RegisterArithmeticType)((first_dword >> 20) & 0xF);
                opcode.perform_math_reg.dst_reg_index = ((first_dword >> 16) & 0xF);
                opcode.perform_math_reg.src_reg_1_index = ((first_dword >> 12) & 0xF);
                opcode.perform_math_reg.has_immediate = ((first_dword >> 8) & 0xF) != 0;
                if (opcode.perform_math_reg.has_immediate) {
                    opcode.perform_math_reg.src_reg_2_index = 0;
                    opcode.perform_math_reg.value = GetNextVmInt(opcode.perform_math_reg.bit_width);
                } else {
                    opcode.perform_math_reg.src_reg_2_index = ((first_dword >> 4) & 0xF);
                }
            }
            break;
        case CheatVmOpcodeType_StoreRegisterToAddress:
            {
                /* ATSRIOxa (aaaaaaaa) */
                /* A = opcode 10 */
                /* T = bit width */
                /* S = src register index */
                /* R = address register index */
                /* I = 1 if increment address register, 0 if not increment address register */
                /* O = offset type, 0 = None, 1 = Register, 2 = Immediate, 3 = Memory Region,
                        4 = Memory Region + Relative Address (ignore address register), 5 = Memory Region + Relative Address */
                /* x = offset register (for offset type 1), memory type (for offset type 3) */
                /* a = relative address (for offset type 2+3) */
                opcode.str_register.bit_width = (first_dword >> 24) & 0xF;
                opcode.str_register.str_reg_index  = ((first_dword >> 20) & 0xF);
                opcode.str_register.addr_reg_index = ((first_dword >> 16) & 0xF);
                opcode.str_register.increment_reg  = ((first_dword >> 12) & 0xF) != 0;
                opcode.str_register.ofs_type = (StoreRegisterOffsetType)(((first_dword >> 8) & 0xF));
                opcode.str_register.ofs_reg_index = ((first_dword >> 4) & 0xF);
                switch (opcode.str_register.ofs_type) {
                    case StoreRegisterOffsetType_None:
                    case StoreRegisterOffsetType_Reg:
                        /* Nothing more to do */
                        break;
                    case StoreRegisterOffsetType_Imm:
                        opcode.str_register.rel_address = (((u64)(first_dword & 0xF) << 32ul) | ((u64)GetNextDword()));
                        break;
                    case StoreRegisterOffsetType_MemReg:
                        opcode.str_register.mem_type = (MemoryAccessType)((first_dword >> 4) & 0xF);
                        break;
                    case StoreRegisterOffsetType_MemImm:
                    case StoreRegisterOffsetType_MemImmReg:
                        opcode.str_register.mem_type = (MemoryAccessType)((first_dword >> 4) & 0xF);
                        opcode.str_register.rel_address = (((u64)(first_dword & 0xF) << 32ul) | ((u64)GetNextDword()));
                        break;
                    default:
                        opcode.str_register.ofs_type = StoreRegisterOffsetType_None;
                        break;
                }
            }
            break;
        case CheatVmOpcodeType_BeginRegisterConditionalBlock:
            {
                /* C0TcSX## */
                /* C0TcS0Ma aaaaaaaa */
                /* C0TcS1Mr */
                /* C0TcS2Ra aaaaaaaa */
                /* C0TcS3Rr */
                /* C0TcS400 VVVVVVVV (VVVVVVVV) */
                /* C0TcS5X0 */
                /* C0 = opcode 0xC0 */
                /* T = bit width */
                /* c = condition type. */
                /* S = source register. */
                /* X = value operand type, 0 = main/heap with relative offset, 1 = main/heap with offset register, */
                /*     2 = register with relative offset, 3 = register with offset register, 4 = static value, 5 = other register. */
                /* M = memory type. */
                /* R = address register. */
                /* a = relative address. */
                /* r = offset register. */
                /* X = other register. */
                /* V = value. */
                opcode.begin_reg_cond.bit_width = (first_dword >> 20) & 0xF;
                opcode.begin_reg_cond.cond_type = (ConditionalComparisonType)((first_dword >> 16) & 0xF);
                opcode.begin_reg_cond.val_reg_index  = ((first_dword >> 12) & 0xF);
                opcode.begin_reg_cond.comp_type = (CompareRegisterValueType)((first_dword >> 8) & 0xF);
                
                switch (opcode.begin_reg_cond.comp_type) {
                    case CompareRegisterValueType_StaticValue:
                        opcode.begin_reg_cond.value = GetNextVmInt(opcode.begin_reg_cond.bit_width);
                        break;
                    case CompareRegisterValueType_OtherRegister:
                        opcode.begin_reg_cond.other_reg_index = ((first_dword >> 4) & 0xF);
                        break;
                    case CompareRegisterValueType_MemoryRelAddr:
                        opcode.begin_reg_cond.mem_type = (MemoryAccessType)((first_dword >> 4) & 0xF);
                        opcode.begin_reg_cond.rel_address = (((u64)(first_dword & 0xF) << 32ul) | ((u64)GetNextDword()));
                        break;
                    case CompareRegisterValueType_MemoryOfsReg:
                        opcode.begin_reg_cond.mem_type = (MemoryAccessType)((first_dword >> 4) & 0xF);
                        opcode.begin_reg_cond.ofs_reg_index = (first_dword & 0xF);
                        break;
                    case CompareRegisterValueType_RegisterRelAddr:
                        opcode.begin_reg_cond.addr_reg_index = ((first_dword >> 4) & 0xF);
                        opcode.begin_reg_cond.rel_address = (((u64)(first_dword & 0xF) << 32ul) | ((u64)GetNextDword()));
                        break;
                    case CompareRegisterValueType_RegisterOfsReg:
                        opcode.begin_reg_cond.addr_reg_index = ((first_dword >> 4) & 0xF);
                        opcode.begin_reg_cond.ofs_reg_index = (first_dword & 0xF);
                        break;
                }
            }
            break;
        case CheatVmOpcodeType_SaveRestoreRegister:
            {
                /* C10D0Sx0 */
                /* C1 = opcode 0xC1 */
                /* D = destination index. */
                /* S = source index. */
                /* x = 1 if saving a register, 0 if restoring a register. */
                /* NOTE: If we add more save slots later, current encoding is backwards compatible. */
                opcode.save_restore_reg.dst_index = (first_dword >> 16) & 0xF;
                opcode.save_restore_reg.src_index = (first_dword >> 8) & 0xF;
                opcode.save_restore_reg.is_save = ((first_dword >> 4) & 0xF) != 0;
            }
            break;
        case CheatVmOpcodeType_SaveRestoreRegisterMask:
            {
                /* C2x0XXXX */
                /* C2 = opcode 0xC2 */
                /* x = 1 if saving, 0 if restoring. */
                /* X = 16-bit bitmask, bit i --> save or restore register i. */
                opcode.save_restore_regmask.is_save = ((first_dword >> 20) & 0xF) != 0;
                for (size_t i = 0; i < NumRegisters; i++) {
                    opcode.save_restore_regmask.should_save_or_restore[i] = (first_dword & (1u << i)) != 0;
                }
            }
            break;
        case CheatVmOpcodeType_ExtendedWidth:
        default:
            /* Unrecognized instruction cannot be decoded. */
            valid = false;
            break;
    }
    
    /* End decoding. */
    return valid;
}

void DmntCheatVm::SkipConditionalBlock() {
    if (this->condition_depth > 0) {
        /* We want to continue until we're out of the current block. */
        const size_t desired_depth = this->condition_depth - 1;
        
        CheatVmOpcode skip_opcode;
        while (this->condition_depth > desired_depth && this->DecodeNextOpcode(&skip_opcode)) {
            /* Decode instructions until we see end of the current conditional block. */
            /* NOTE: This is broken in gateway's implementation. */
            /* Gateway currently checks for "0x2" instead of "0x20000000" */
            /* In addition, they do a linear scan instead of correctly decoding opcodes. */
            /* This causes issues if "0x2" appears as an immediate in the conditional block... */
            
            /* We also support nesting of conditional blocks, and Gateway does not. */
            if (skip_opcode.begin_conditional_block) {
                this->condition_depth++;
            } else if (skip_opcode.opcode == CheatVmOpcodeType_EndConditionalBlock) {
                this->condition_depth--;
            }
        }
    } else {
        /* Skipping, but this->condition_depth = 0. */
        /* This is an error condition. */
        /* However, I don't actually believe it is possible for this to happen. */
        /* I guess we'll throw a fatal error here, so as to encourage me to fix the VM */
        /* in the event that someone triggers it? I don't know how you'd do that. */
        fatalSimple(ResultDmntCheatVmInvalidCondDepth);
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
        this->saved_values[i] = 0;
        this->loop_tops[i] = 0;
    }
    this->instruction_ptr = 0;
    this->condition_depth = 0;
    this->decode_success = true;
}

bool DmntCheatVm::LoadProgram(const CheatEntry *cheats, size_t num_cheats) {
    /* Reset opcode count. */
    this->num_opcodes = 0;
    
    for (size_t i = 0; i < num_cheats; i++) {
        if (cheats[i].enabled) {
            /* Bounds check. */
            if (cheats[i].definition.num_opcodes + this->num_opcodes > MaximumProgramOpcodeCount) {
                this->num_opcodes = 0;
                return false;
            }
            
            for (size_t n = 0; n < cheats[i].definition.num_opcodes; n++) {
                this->program[this->num_opcodes++] = cheats[i].definition.opcodes[n];
            }
        }
    }
    
    return true;
}

void DmntCheatVm::Execute(const CheatProcessMetadata *metadata) {
    CheatVmOpcode cur_opcode;
    u64 kDown = 0;
    
    /* Get Keys down. */
    HidManagement::GetKeysDown(&kDown);
    
    this->OpenDebugLogFile();
    ON_SCOPE_EXIT { this->CloseDebugLogFile(); };
    
    this->LogToDebugFile("Started VM execution.\n");
    this->LogToDebugFile("Main NSO:  %012lx\n", metadata->main_nso_extents.base);
    this->LogToDebugFile("Heap:      %012lx\n", metadata->main_nso_extents.base);
    this->LogToDebugFile("Keys Down: %08x\n", (u32)(kDown & 0x0FFFFFFF));
        
    /* Clear VM state. */
    this->ResetState();
    
    /* Loop until program finishes. */
    while (this->DecodeNextOpcode(&cur_opcode)) {
        this->LogToDebugFile("Instruction Ptr: %04x\n", (u32)this->instruction_ptr);

        for (size_t i = 0; i < NumRegisters; i++) {
            this->LogToDebugFile("Registers[%02x]: %016lx\n", i, this->registers[i]);
        }

        for (size_t i = 0; i < NumRegisters; i++) {
            this->LogToDebugFile("SavedRegs[%02x]: %016lx\n", i, this->saved_values[i]);
        }
        this->LogOpcode(&cur_opcode);
        
        /* Increment conditional depth, if relevant. */
        if (cur_opcode.begin_conditional_block) {
            this->condition_depth++;
        }
        
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
                /* Decrement the condition depth. */
                /* We will assume, graciously, that mismatched conditional block ends are a nop. */
                if (this->condition_depth > 0) {
                    this->condition_depth--;
                }
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
            case CheatVmOpcodeType_StoreStaticToAddress:
                {
                    /* Calculate address. */
                    u64 dst_address = this->registers[cur_opcode.str_static.reg_index];
                    u64 dst_value = cur_opcode.str_static.value;
                    if (cur_opcode.str_static.add_offset_reg) {
                        dst_address += this->registers[cur_opcode.str_static.offset_reg_index];
                    }
                    /* Write value to memory. Gateway only writes on valid bitwidth. */
                    switch (cur_opcode.str_static.bit_width) {
                        case 1:
                        case 2:
                        case 4:
                        case 8:
                            DmntCheatManager::WriteCheatProcessMemoryForVm(dst_address, &dst_value, cur_opcode.str_static.bit_width);
                            break;
                    }
                    /* Increment register if relevant. */
                    if (cur_opcode.str_static.increment_reg) {
                        this->registers[cur_opcode.str_static.reg_index] += cur_opcode.str_static.bit_width;
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
            case CheatVmOpcodeType_StoreRegisterToAddress:
                {
                    /* Calculate address. */
                    u64 dst_value   = this->registers[cur_opcode.str_register.str_reg_index];
                    u64 dst_address = this->registers[cur_opcode.str_register.addr_reg_index];
                    switch (cur_opcode.str_register.ofs_type) {
                        case StoreRegisterOffsetType_None:
                            /* Nothing more to do */
                            break;
                        case StoreRegisterOffsetType_Reg:
                            dst_address += this->registers[cur_opcode.str_register.ofs_reg_index];
                            break;
                        case StoreRegisterOffsetType_Imm:
                            dst_address += cur_opcode.str_register.rel_address;
                            break;
                        case StoreRegisterOffsetType_MemReg:
                            dst_address = GetCheatProcessAddress(metadata, cur_opcode.str_register.mem_type, this->registers[cur_opcode.str_register.addr_reg_index]);
                            break;
                        case StoreRegisterOffsetType_MemImm:
                            dst_address = GetCheatProcessAddress(metadata, cur_opcode.str_register.mem_type, cur_opcode.str_register.rel_address);
                            break;
                        case StoreRegisterOffsetType_MemImmReg:
                            dst_address = GetCheatProcessAddress(metadata, cur_opcode.str_register.mem_type, this->registers[cur_opcode.str_register.addr_reg_index] + cur_opcode.str_register.rel_address);
                            break;
                    }
                    
                    /* Write value to memory. Write only on valid bitwidth. */
                    switch (cur_opcode.str_register.bit_width) {
                        case 1:
                        case 2:
                        case 4:
                        case 8:
                            DmntCheatManager::WriteCheatProcessMemoryForVm(dst_address, &dst_value, cur_opcode.str_register.bit_width);
                            break;
                    }
                    
                    /* Increment register if relevant. */
                    if (cur_opcode.str_register.increment_reg) {
                        this->registers[cur_opcode.str_register.addr_reg_index] += cur_opcode.str_register.bit_width;
                    }
                }
                break;
            case CheatVmOpcodeType_BeginRegisterConditionalBlock:
                {
                    /* Get value from register. */
                    u64 src_value = 0;
                    switch (cur_opcode.begin_reg_cond.bit_width) {
                        case 1:
                            src_value = static_cast<u8>(this->registers[cur_opcode.begin_reg_cond.val_reg_index]  & 0xFFul);
                            break;
                        case 2:
                            src_value = static_cast<u16>(this->registers[cur_opcode.begin_reg_cond.val_reg_index] & 0xFFFFul);
                            break;
                        case 4:
                            src_value = static_cast<u32>(this->registers[cur_opcode.begin_reg_cond.val_reg_index] & 0xFFFFFFFFul);
                            break;
                        case 8:
                            src_value = static_cast<u64>(this->registers[cur_opcode.begin_reg_cond.val_reg_index] & 0xFFFFFFFFFFFFFFFFul);
                            break;
                    }
                    
                    /* Read value from memory. */
                    u64 cond_value = 0;
                    if (cur_opcode.begin_reg_cond.comp_type == CompareRegisterValueType_StaticValue) {
                        cond_value = GetVmInt(cur_opcode.begin_reg_cond.value, cur_opcode.begin_reg_cond.bit_width);
                    } else if (cur_opcode.begin_reg_cond.comp_type == CompareRegisterValueType_OtherRegister) {
                        switch (cur_opcode.begin_reg_cond.bit_width) {
                            case 1:
                                cond_value = static_cast<u8>(this->registers[cur_opcode.begin_reg_cond.other_reg_index]  & 0xFFul);
                                break;
                            case 2:
                                cond_value = static_cast<u16>(this->registers[cur_opcode.begin_reg_cond.other_reg_index] & 0xFFFFul);
                                break;
                            case 4:
                                cond_value = static_cast<u32>(this->registers[cur_opcode.begin_reg_cond.other_reg_index] & 0xFFFFFFFFul);
                                break;
                            case 8:
                                cond_value = static_cast<u64>(this->registers[cur_opcode.begin_reg_cond.other_reg_index] & 0xFFFFFFFFFFFFFFFFul);
                                break;
                        }
                    } else {
                        u64 cond_address = 0;
                        switch (cur_opcode.begin_reg_cond.comp_type) {
                            case CompareRegisterValueType_MemoryRelAddr:
                                cond_address = GetCheatProcessAddress(metadata, cur_opcode.begin_reg_cond.mem_type, cur_opcode.begin_reg_cond.rel_address);
                                break;
                            case CompareRegisterValueType_MemoryOfsReg:
                                cond_address = GetCheatProcessAddress(metadata, cur_opcode.begin_reg_cond.mem_type, this->registers[cur_opcode.begin_reg_cond.ofs_reg_index]);
                                break;
                            case CompareRegisterValueType_RegisterRelAddr:
                                cond_address = this->registers[cur_opcode.begin_reg_cond.addr_reg_index] + cur_opcode.begin_reg_cond.rel_address;
                                break;
                            case CompareRegisterValueType_RegisterOfsReg:
                                cond_address = this->registers[cur_opcode.begin_reg_cond.addr_reg_index] + this->registers[cur_opcode.begin_reg_cond.ofs_reg_index];
                                break;
                            default:
                                break;
                        }
                        switch (cur_opcode.begin_reg_cond.bit_width) {
                            case 1:
                            case 2:
                            case 4:
                            case 8:
                                DmntCheatManager::ReadCheatProcessMemoryForVm(cond_address, &cond_value, cur_opcode.begin_reg_cond.bit_width);
                                break;
                        }
                    }
                    
                    /* Check against condition. */
                    bool cond_met = false;
                    switch (cur_opcode.begin_reg_cond.cond_type) {
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
            case CheatVmOpcodeType_SaveRestoreRegister:
                /* Save or restore a register. */
                if (cur_opcode.save_restore_reg.is_save) {
                    this->saved_values[cur_opcode.save_restore_reg.dst_index] = this->registers[cur_opcode.save_restore_reg.src_index];
                } else {
                    this->registers[cur_opcode.save_restore_reg.dst_index] = this->saved_values[cur_opcode.save_restore_reg.src_index];
                }
                break;
            case CheatVmOpcodeType_SaveRestoreRegisterMask:
                /* Save or restore register mask. */
                u64 *src;
                u64 *dst;
                if (cur_opcode.save_restore_regmask.is_save) {
                    src = this->registers;
                    dst = this->saved_values;
                } else {
                    src = this->registers;
                    dst = this->saved_values;
                }
                for (size_t i = 0; i < NumRegisters; i++) {
                    if (cur_opcode.save_restore_regmask.should_save_or_restore[i]) {
                        dst[i] = src[i];
                    }
                }
                break;
            default:
                /* By default, we do a no-op. */
                break;
        }
    }
}