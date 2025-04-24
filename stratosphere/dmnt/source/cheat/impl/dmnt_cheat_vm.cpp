/*
 * Copyright (c) Atmosphère-NX
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
#include <stratosphere.hpp>
#include "dmnt_cheat_vm.hpp"
#include "dmnt_cheat_api.hpp"

namespace ams::dmnt::cheat::impl {

    void CheatVirtualMachine::DebugLog(u32 log_id, u64 value) {
        /* Just unconditionally try to create the log folder. */
        fs::EnsureDirectory("sdmc:/atmosphere/cheat_vm_logs");

        fs::FileHandle log_file;
        {
            char log_path[fs::EntryNameLengthMax + 1];
            util::SNPrintf(log_path, sizeof(log_path), "sdmc:/atmosphere/cheat_vm_logs/%08x.log", log_id);
            if (R_FAILED(fs::OpenFile(std::addressof(log_file), log_path, fs::OpenMode_Write | fs::OpenMode_AllowAppend))) {
                return;
            }
        }
        ON_SCOPE_EXIT { fs::CloseFile(log_file); };

        s64 log_offset;
        if (R_FAILED(fs::GetFileSize(std::addressof(log_offset), log_file))) {
            return;
        }

        char log_value[18];
        util::SNPrintf(log_value, sizeof(log_value), "%016lx\n", value);
        fs::WriteFile(log_file, log_offset, log_value, std::strlen(log_value), fs::WriteOption::Flush);
    }

    void CheatVirtualMachine::OpenDebugLogFile() {
        #ifdef DMNT_CHEAT_VM_DEBUG_LOG
        CloseDebugLogFile();
        fs::EnsureDirectory("sdmc:/atmosphere/cheat_vm_logs");
        fs::CreateFile("sdmc:/atmosphere/cheat_vm_logs/debug_log.txt", 0);
        R_ABORT_UNLESS(fs::OpenFile(std::addressof(m_debug_log_file), "sdmc:/atmosphere/cheat_vm_logs/debug_log.txt", fs::OpenMode_Write | fs::OpenMode_AllowAppend));
        m_debug_log_file_offset = 0;
        m_has_debug_log_file = true;
        #endif
    }

    void CheatVirtualMachine::CloseDebugLogFile() {
        #ifdef DMNT_CHEAT_VM_DEBUG_LOG
        if (m_has_debug_log_file) {
            fs::CloseFile(m_debug_log_file);
        }
        m_has_debug_log_file = false;
        #endif
    }

    void CheatVirtualMachine::LogToDebugFile(const char *format, ...) {
        #ifdef DMNT_CHEAT_VM_DEBUG_LOG
        if (!m_has_debug_log_file) {
            return;
        }

        {
            std::va_list vl;
            va_start(vl, format);
            util::VSNPrintf(m_debug_log_format_buf, sizeof(m_debug_log_format_buf) - 1, format, vl);
            va_end(vl);
        }

        size_t fmt_len = std::strlen(m_debug_log_format_buf);
        if (m_debug_log_format_buf[fmt_len - 1] != '\n') {
            m_debug_log_format_buf[fmt_len + 0] = '\n';
            m_debug_log_format_buf[fmt_len + 1] = '\x00';
            fmt_len += 1;
        }

        fs::WriteFile(m_debug_log_file, m_debug_log_file_offset, m_debug_log_format_buf, fmt_len, fs::WriteOption::Flush);
        m_debug_log_file_offset += fmt_len;
        #else
        AMS_UNUSED(format);
        #endif
    }

    void CheatVirtualMachine::LogOpcode(const CheatVmOpcode *opcode) {
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
                this->LogToDebugFile("Inc Ofs reg:   %d\n", opcode->begin_cond.include_ofs_reg);
                this->LogToDebugFile("Ofs Reg Idx: %x\n", opcode->begin_cond.ofs_reg_index);
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
            case CheatVmOpcodeType_BeginExtendedKeypressConditionalBlock:
                this->LogToDebugFile("Opcode: Begin Extended Keypress Conditional\n");
                this->LogToDebugFile("Key Mask:  %x\n", opcode->begin_ext_keypress_cond.key_mask);
                this->LogToDebugFile("Auto Repeat:  %d\n", opcode->begin_ext_keypress_cond.auto_repeat);
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
                this->LogToDebugFile("Op Type:   %d\n", opcode->save_restore_reg.op_type);
                break;
            case CheatVmOpcodeType_SaveRestoreRegisterMask:
                this->LogToDebugFile("Opcode: Save or Restore Register Mask\n");
                this->LogToDebugFile("Op Type:   %d\n", opcode->save_restore_regmask.op_type);
                for (size_t i = 0; i < NumRegisters; i++) {
                    this->LogToDebugFile("Act[%02x]:   %d\n", i, opcode->save_restore_regmask.should_operate[i]);
                }
                break;
            case CheatVmOpcodeType_ReadWriteStaticRegister:
                this->LogToDebugFile("Opcode: Read/Write Static Register\n");
                if (opcode->rw_static_reg.static_idx < NumReadableStaticRegisters) {
                    this->LogToDebugFile("Op Type: ReadStaticRegister\n");
                } else {
                    this->LogToDebugFile("Op Type: WriteStaticRegister\n");
                }
                this->LogToDebugFile("Reg Idx:   %x\n", opcode->rw_static_reg.idx);
                this->LogToDebugFile("Stc Idx:   %x\n", opcode->rw_static_reg.static_idx);
                break;
            case CheatVmOpcodeType_PauseProcess:
                this->LogToDebugFile("Opcode: Pause Cheat Process\n");
                break;
            case CheatVmOpcodeType_ResumeProcess:
                this->LogToDebugFile("Opcode: Resume Cheat Process\n");
                break;
            case CheatVmOpcodeType_DebugLog:
                this->LogToDebugFile("Opcode: Debug Log\n");
                this->LogToDebugFile("Bit Width: %x\n", opcode->debug_log.bit_width);
                this->LogToDebugFile("Log ID:    %x\n", opcode->debug_log.log_id);
                this->LogToDebugFile("Val Type:  %x\n", opcode->debug_log.val_type);
                switch (opcode->debug_log.val_type) {
                    case DebugLogValueType_RegisterValue:
                        this->LogToDebugFile("Val Type:  Register Value\n");
                        this->LogToDebugFile("X Reg Idx: %x\n", opcode->debug_log.val_reg_index);
                        break;
                    case DebugLogValueType_MemoryRelAddr:
                        this->LogToDebugFile("Val Type:  Memory Relative Address\n");
                        this->LogToDebugFile("Mem Type:  %x\n", opcode->debug_log.mem_type);
                        this->LogToDebugFile("Rel Addr:  %lx\n", opcode->debug_log.rel_address);
                        break;
                    case DebugLogValueType_MemoryOfsReg:
                        this->LogToDebugFile("Val Type:  Memory Offset Register\n");
                        this->LogToDebugFile("Mem Type:  %x\n", opcode->debug_log.mem_type);
                        this->LogToDebugFile("O Reg Idx: %x\n", opcode->debug_log.ofs_reg_index);
                        break;
                    case DebugLogValueType_RegisterRelAddr:
                        this->LogToDebugFile("Val Type:  Register Relative Address\n");
                        this->LogToDebugFile("A Reg Idx: %x\n", opcode->debug_log.addr_reg_index);
                        this->LogToDebugFile("Rel Addr:  %lx\n", opcode->debug_log.rel_address);
                        break;
                    case DebugLogValueType_RegisterOfsReg:
                        this->LogToDebugFile("Val Type:  Register Offset Register\n");
                        this->LogToDebugFile("A Reg Idx: %x\n", opcode->debug_log.addr_reg_index);
                        this->LogToDebugFile("O Reg Idx: %x\n", opcode->debug_log.ofs_reg_index);
                        break;
                }
                break;
            default:
                this->LogToDebugFile("Unknown opcode: %x\n", opcode->opcode);
                break;
        }
    }

    bool CheatVirtualMachine::DecodeNextOpcode(CheatVmOpcode *out) {
        /* If we've ever seen a decode failure, return false. */
        bool valid = m_decode_success;
        CheatVmOpcode opcode = {};
        ON_SCOPE_EXIT {
            m_decode_success &= valid;
            if (valid) {
                *out = opcode;
            }
        };

        /* Helper function for getting instruction dwords. */
        auto GetNextDword = [&]() {
            if (m_instruction_ptr >= m_num_opcodes) {
                valid = false;
                return static_cast<u32>(0);
            }
            return m_program[m_instruction_ptr++];
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
        if (opcode.opcode >= CheatVmOpcodeType_DoubleExtendedWidth) {
            opcode.opcode = (CheatVmOpcodeType)((((u32)opcode.opcode) << 4) | ((first_dword >> 20) & 0xF));
        }

        /* detect condition start. */
        switch (opcode.opcode) {
            case CheatVmOpcodeType_BeginConditionalBlock:
            case CheatVmOpcodeType_BeginKeypressConditionalBlock:
            case CheatVmOpcodeType_BeginExtendedKeypressConditionalBlock:
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
                    opcode.begin_cond.include_ofs_reg = ((first_dword >> 12) & 0xF) != 0;
                    opcode.begin_cond.ofs_reg_index = ((first_dword >> 8) & 0xF);
                    opcode.begin_cond.rel_address = ((u64)(first_dword & 0xFF) << 32ul) | ((u64)second_dword);
                    opcode.begin_cond.value = GetNextVmInt(opcode.begin_cond.bit_width);
                }
                break;
            case CheatVmOpcodeType_EndConditionalBlock:
                {
                    /* 2X000000 */
                    opcode.end_cond.is_else = ((first_dword >> 24) & 0xF) == 1;
                }
                break;
            case CheatVmOpcodeType_ControlLoop:
                {
                    /* 300R0000 VVVVVVVV */
                    /* 310R0000 */
                    /* Parse register, whether loop start or loop end. */
                    opcode.ctrl_loop.start_loop = ((first_dword >> 24) & 0xF) == 0;
                    opcode.ctrl_loop.reg_index = ((first_dword >> 16) & 0xF);

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
                    opcode.ldr_memory.load_from_reg = ((first_dword >> 12) & 0xF);
                    opcode.ldr_memory.offset_register = ((first_dword >> 8) & 0xF);
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
            case CheatVmOpcodeType_BeginExtendedKeypressConditionalBlock:
                {
                    /* C4r00000 kkkkkkkk kkkkkkkk */
                    /* Read additional words. */
                    opcode.begin_ext_keypress_cond.key_mask = (u64)GetNextDword() << 32ul | (u64)GetNextDword();
                    opcode.begin_ext_keypress_cond.auto_repeat = ((first_dword >> 20) & 0xF) != 0;
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
                    /* x = 3 if clearing reg, 2 if clearing saved value, 1 if saving a register, 0 if restoring a register. */
                    /* NOTE: If we add more save slots later, current encoding is backwards compatible. */
                    opcode.save_restore_reg.dst_index = (first_dword >> 16) & 0xF;
                    opcode.save_restore_reg.src_index = (first_dword >> 8) & 0xF;
                    opcode.save_restore_reg.op_type = (SaveRestoreRegisterOpType)((first_dword >> 4) & 0xF);
                }
                break;
            case CheatVmOpcodeType_SaveRestoreRegisterMask:
                {
                    /* C2x0XXXX */
                    /* C2 = opcode 0xC2 */
                    /* x = 3 if clearing reg, 2 if clearing saved value, 1 if saving, 0 if restoring. */
                    /* X = 16-bit bitmask, bit i --> save or restore register i. */
                    opcode.save_restore_regmask.op_type = (SaveRestoreRegisterOpType)((first_dword >> 20) & 0xF);
                    for (size_t i = 0; i < NumRegisters; i++) {
                        opcode.save_restore_regmask.should_operate[i] = (first_dword & (1u << i)) != 0;
                    }
                }
                break;
            case CheatVmOpcodeType_ReadWriteStaticRegister:
                {
                    /* C3000XXx */
                    /* C3 = opcode 0xC3. */
                    /* XX = static register index. */
                    /* x  = register index. */
                    opcode.rw_static_reg.static_idx = ((first_dword >> 4) & 0xFF);
                    opcode.rw_static_reg.idx        = (first_dword & 0xF);
                }
                break;
            case CheatVmOpcodeType_PauseProcess:
                {
                    /* FF0????? */
                    /* FF0 = opcode 0xFF0 */
                    /* Pauses the current process. */
                }
                break;
            case CheatVmOpcodeType_ResumeProcess:
                {
                    /* FF1????? */
                    /* FF1 = opcode 0xFF1 */
                    /* Resumes the current process. */
                }
                break;
            case CheatVmOpcodeType_DebugLog:
                {
                    /* FFFTIX## */
                    /* FFFTI0Ma aaaaaaaa */
                    /* FFFTI1Mr */
                    /* FFFTI2Ra aaaaaaaa */
                    /* FFFTI3Rr */
                    /* FFFTI4X0 */
                    /* FFF = opcode 0xFFF */
                    /* T = bit width. */
                    /* I = log id. */
                    /* X = value operand type, 0 = main/heap with relative offset, 1 = main/heap with offset register, */
                    /*     2 = register with relative offset, 3 = register with offset register, 4 = register value. */
                    /* M = memory type. */
                    /* R = address register. */
                    /* a = relative address. */
                    /* r = offset register. */
                    /* X = value register. */
                    opcode.debug_log.bit_width = (first_dword >> 16) & 0xF;
                    opcode.debug_log.log_id  = ((first_dword >> 12) & 0xF);
                    opcode.debug_log.val_type = (DebugLogValueType)((first_dword >> 8) & 0xF);

                    switch (opcode.debug_log.val_type) {
                        case DebugLogValueType_RegisterValue:
                            opcode.debug_log.val_reg_index = ((first_dword >> 4) & 0xF);
                            break;
                        case DebugLogValueType_MemoryRelAddr:
                            opcode.debug_log.mem_type = (MemoryAccessType)((first_dword >> 4) & 0xF);
                            opcode.debug_log.rel_address = (((u64)(first_dword & 0xF) << 32ul) | ((u64)GetNextDword()));
                            break;
                        case DebugLogValueType_MemoryOfsReg:
                            opcode.debug_log.mem_type = (MemoryAccessType)((first_dword >> 4) & 0xF);
                            opcode.debug_log.ofs_reg_index = (first_dword & 0xF);
                            break;
                        case DebugLogValueType_RegisterRelAddr:
                            opcode.debug_log.addr_reg_index = ((first_dword >> 4) & 0xF);
                            opcode.debug_log.rel_address = (((u64)(first_dword & 0xF) << 32ul) | ((u64)GetNextDword()));
                            break;
                        case DebugLogValueType_RegisterOfsReg:
                            opcode.debug_log.addr_reg_index = ((first_dword >> 4) & 0xF);
                            opcode.debug_log.ofs_reg_index = (first_dword & 0xF);
                            break;
                    }
                }
                break;
            case CheatVmOpcodeType_ExtendedWidth:
            case CheatVmOpcodeType_DoubleExtendedWidth:
            default:
                /* Unrecognized instruction cannot be decoded. */
                valid = false;
                break;
        }

        /* End decoding. */
        return valid;
    }

    void CheatVirtualMachine::SkipConditionalBlock(bool is_if) {
        if (m_condition_depth > 0) {
            /* We want to continue until we're out of the current block. */
            const size_t desired_depth = m_condition_depth - 1;

            CheatVmOpcode skip_opcode;
            while (m_condition_depth > desired_depth && this->DecodeNextOpcode(std::addressof(skip_opcode))) {
                /* Decode instructions until we see end of the current conditional block. */
                /* NOTE: This is broken in gateway's implementation. */
                /* Gateway currently checks for "0x2" instead of "0x20000000" */
                /* In addition, they do a linear scan instead of correctly decoding opcodes. */
                /* This causes issues if "0x2" appears as an immediate in the conditional block... */

                /* We also support nesting of conditional blocks, and Gateway does not. */
                if (skip_opcode.begin_conditional_block) {
                    m_condition_depth++;
                } else if (skip_opcode.opcode == CheatVmOpcodeType_EndConditionalBlock) {
                    if (!skip_opcode.end_cond.is_else) {
                        m_condition_depth--;
                    } else if (is_if && m_condition_depth - 1 == desired_depth) {
                        /* An if will continue to an else at the same depth. */
                        break;
                    }
                }
            }
        } else {
            /* Skipping, but m_condition_depth = 0. */
            /* This is an error condition. */
            /* This could occur with a mismatched "else" opcode, for example. */
            R_ABORT_UNLESS(ResultVirtualMachineInvalidConditionDepth());
        }
    }

    u64 CheatVirtualMachine::GetVmInt(VmInt value, u32 bit_width) {
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

    u64 CheatVirtualMachine::GetCheatProcessAddress(const CheatProcessMetadata* metadata, MemoryAccessType mem_type, u64 rel_address) {
        switch (mem_type) {
            case MemoryAccessType_MainNso:
            default:
                return metadata->main_nso_extents.base + rel_address;
            case MemoryAccessType_Heap:
                return metadata->heap_extents.base + rel_address;
            case MemoryAccessType_Alias:
                return metadata->alias_extents.base + rel_address;
            case MemoryAccessType_Aslr:
                return metadata->aslr_extents.base + rel_address;
            case MemoryAccessType_NonRelative:
                return rel_address;
        }
    }

    void CheatVirtualMachine::ResetState() {
        for (size_t i = 0; i < CheatVirtualMachine::NumRegisters; i++) {
            m_registers[i] = 0;
            m_saved_values[i] = 0;
            m_loop_tops[i] = 0;
        }
        m_instruction_ptr = 0;
        m_condition_depth = 0;
        m_decode_success = true;
    }

    bool CheatVirtualMachine::LoadProgram(const CheatEntry *cheats, size_t num_cheats) {
        /* Reset opcode count. */
        m_num_opcodes = 0;

        for (size_t i = 0; i < num_cheats; i++) {
            if (cheats[i].enabled) {
                /* Bounds check. */
                if (cheats[i].definition.num_opcodes + m_num_opcodes > MaximumProgramOpcodeCount) {
                    m_num_opcodes = 0;
                    return false;
                }

                for (size_t n = 0; n < cheats[i].definition.num_opcodes; n++) {
                    m_program[m_num_opcodes++] = cheats[i].definition.opcodes[n];
                }
            }
        }

        return true;
    }

    static u64 s_keyold = 0;
    void CheatVirtualMachine::Execute(const CheatProcessMetadata *metadata) {
        CheatVmOpcode cur_opcode;
        u64 kHeld = 0;

        /* Get Keys held. */
        hid::GetKeysHeld(std::addressof(kHeld));

        this->OpenDebugLogFile();
        ON_SCOPE_EXIT { this->CloseDebugLogFile(); };

        this->LogToDebugFile("Started VM execution.\n");
        this->LogToDebugFile("Main NSO:  %012lx\n", metadata->main_nso_extents.base);
        this->LogToDebugFile("Heap:      %012lx\n", metadata->main_nso_extents.base);
        this->LogToDebugFile("Keys Held: %08x\n", (u32)(kHeld & 0x0FFFFFFF));

        /* Clear VM state. */
        this->ResetState();

        /* Loop until program finishes. */
        while (this->DecodeNextOpcode(std::addressof(cur_opcode))) {
            this->LogToDebugFile("Instruction Ptr: %04x\n", (u32)m_instruction_ptr);

            for (size_t i = 0; i < NumRegisters; i++) {
                this->LogToDebugFile("Registers[%02x]: %016lx\n", i, m_registers[i]);
            }

            for (size_t i = 0; i < NumRegisters; i++) {
                this->LogToDebugFile("SavedRegs[%02x]: %016lx\n", i, m_saved_values[i]);
            }
            this->LogOpcode(std::addressof(cur_opcode));

            /* Increment conditional depth, if relevant. */
            if (cur_opcode.begin_conditional_block) {
                m_condition_depth++;
            }

            switch (cur_opcode.opcode) {
                case CheatVmOpcodeType_StoreStatic:
                    {
                        /* Calculate address, write value to memory. */
                        u64 dst_address = GetCheatProcessAddress(metadata, cur_opcode.store_static.mem_type, cur_opcode.store_static.rel_address + m_registers[cur_opcode.store_static.offset_register]);
                        u64 dst_value = GetVmInt(cur_opcode.store_static.value, cur_opcode.store_static.bit_width);
                        switch (cur_opcode.store_static.bit_width) {
                            case 1:
                            case 2:
                            case 4:
                            case 8:
                                dmnt::cheat::impl::WriteCheatProcessMemoryUnsafe(dst_address, std::addressof(dst_value), cur_opcode.store_static.bit_width);
                                break;
                        }
                    }
                    break;
                case CheatVmOpcodeType_BeginConditionalBlock:
                    {
                        /* Read value from memory. */
                        u64 src_address = GetCheatProcessAddress(metadata, cur_opcode.begin_cond.mem_type, (cur_opcode.begin_cond.include_ofs_reg) ? m_registers[cur_opcode.begin_cond.ofs_reg_index] + cur_opcode.begin_cond.rel_address : cur_opcode.begin_cond.rel_address);
                        u64 src_value = 0;
                        switch (cur_opcode.store_static.bit_width) {
                            case 1:
                            case 2:
                            case 4:
                            case 8:
                                dmnt::cheat::impl::ReadCheatProcessMemoryUnsafe(src_address, std::addressof(src_value), cur_opcode.begin_cond.bit_width);
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
                            this->SkipConditionalBlock(true);
                        }
                    }
                    break;
                case CheatVmOpcodeType_EndConditionalBlock:
                    if (cur_opcode.end_cond.is_else) {
                        /* Skip to the end of the conditional block. */
                        this->SkipConditionalBlock(false);
                    } else {
                        /* Decrement the condition depth. */
                        /* We will assume, graciously, that mismatched conditional block ends are a nop. */
                        if (m_condition_depth > 0) {
                            m_condition_depth--;
                        }
                    }
                    break;
                case CheatVmOpcodeType_ControlLoop:
                    if (cur_opcode.ctrl_loop.start_loop) {
                        /* Start a loop. */
                        m_registers[cur_opcode.ctrl_loop.reg_index] = cur_opcode.ctrl_loop.num_iters;
                        m_loop_tops[cur_opcode.ctrl_loop.reg_index] = m_instruction_ptr;
                    } else {
                        /* End a loop. */
                        m_registers[cur_opcode.ctrl_loop.reg_index]--;
                        if (m_registers[cur_opcode.ctrl_loop.reg_index] != 0) {
                            m_instruction_ptr = m_loop_tops[cur_opcode.ctrl_loop.reg_index];
                        }
                    }
                    break;
                case CheatVmOpcodeType_LoadRegisterStatic:
                    /* Set a register to a static value. */
                    m_registers[cur_opcode.ldr_static.reg_index] = cur_opcode.ldr_static.value;
                    break;
                case CheatVmOpcodeType_LoadRegisterMemory:
                    {
                        /* Choose source address. */
                        u64 src_address;
                        if (cur_opcode.ldr_memory.load_from_reg == 1) {
                            src_address = m_registers[cur_opcode.ldr_memory.reg_index] + cur_opcode.ldr_memory.rel_address;
                        } else if (cur_opcode.ldr_memory.load_from_reg == 2) {
                            src_address = m_registers[cur_opcode.ldr_memory.offset_register] + cur_opcode.ldr_memory.rel_address;
                        } else if (cur_opcode.ldr_memory.load_from_reg == 3) {
                            src_address = GetCheatProcessAddress(metadata, cur_opcode.ldr_memory.mem_type, m_registers[cur_opcode.ldr_memory.offset_register] + cur_opcode.ldr_memory.rel_address);
                        } else {
                            src_address = GetCheatProcessAddress(metadata, cur_opcode.ldr_memory.mem_type, cur_opcode.ldr_memory.rel_address);
                        }
                        /* Read into register. Gateway only reads on valid bitwidth. */
                        switch (cur_opcode.ldr_memory.bit_width) {
                            case 1:
                            case 2:
                            case 4:
                            case 8:
                                dmnt::cheat::impl::ReadCheatProcessMemoryUnsafe(src_address, std::addressof(m_registers[cur_opcode.ldr_memory.reg_index]), cur_opcode.ldr_memory.bit_width);
                                break;
                        }
                    }
                    break;
                case CheatVmOpcodeType_StoreStaticToAddress:
                    {
                        /* Calculate address. */
                        u64 dst_address = m_registers[cur_opcode.str_static.reg_index];
                        u64 dst_value = cur_opcode.str_static.value;
                        if (cur_opcode.str_static.add_offset_reg) {
                            dst_address += m_registers[cur_opcode.str_static.offset_reg_index];
                        }
                        /* Write value to memory. Gateway only writes on valid bitwidth. */
                        switch (cur_opcode.str_static.bit_width) {
                            case 1:
                            case 2:
                            case 4:
                            case 8:
                                dmnt::cheat::impl::WriteCheatProcessMemoryUnsafe(dst_address, std::addressof(dst_value), cur_opcode.str_static.bit_width);
                                break;
                        }
                        /* Increment register if relevant. */
                        if (cur_opcode.str_static.increment_reg) {
                            m_registers[cur_opcode.str_static.reg_index] += cur_opcode.str_static.bit_width;
                        }
                    }
                    break;
                case CheatVmOpcodeType_PerformArithmeticStatic:
                    {
                        /* Do requested math. */
                        switch (cur_opcode.perform_math_static.math_type) {
                            case RegisterArithmeticType_Addition:
                                m_registers[cur_opcode.perform_math_static.reg_index] +=  (u64)cur_opcode.perform_math_static.value;
                                break;
                            case RegisterArithmeticType_Subtraction:
                                m_registers[cur_opcode.perform_math_static.reg_index] -=  (u64)cur_opcode.perform_math_static.value;
                                break;
                            case RegisterArithmeticType_Multiplication:
                                m_registers[cur_opcode.perform_math_static.reg_index] *=  (u64)cur_opcode.perform_math_static.value;
                                break;
                            case RegisterArithmeticType_LeftShift:
                                m_registers[cur_opcode.perform_math_static.reg_index] <<= (u64)cur_opcode.perform_math_static.value;
                                break;
                            case RegisterArithmeticType_RightShift:
                                m_registers[cur_opcode.perform_math_static.reg_index] >>= (u64)cur_opcode.perform_math_static.value;
                                break;
                            default:
                                /* Do not handle extensions here. */
                                break;
                        }
                        /* Apply bit width. */
                        switch (cur_opcode.perform_math_static.bit_width) {
                            case 1:
                                m_registers[cur_opcode.perform_math_static.reg_index] = static_cast<u8>(m_registers[cur_opcode.perform_math_static.reg_index]);
                                break;
                            case 2:
                                m_registers[cur_opcode.perform_math_static.reg_index] = static_cast<u16>(m_registers[cur_opcode.perform_math_static.reg_index]);
                                break;
                            case 4:
                                m_registers[cur_opcode.perform_math_static.reg_index] = static_cast<u32>(m_registers[cur_opcode.perform_math_static.reg_index]);
                                break;
                            case 8:
                                m_registers[cur_opcode.perform_math_static.reg_index] = static_cast<u64>(m_registers[cur_opcode.perform_math_static.reg_index]);
                                break;
                        }
                    }
                    break;
                case CheatVmOpcodeType_BeginKeypressConditionalBlock:
                    /* Check for keypress. */
                    if ((cur_opcode.begin_keypress_cond.key_mask & kHeld) != cur_opcode.begin_keypress_cond.key_mask) {
                        /* Keys not pressed. Skip conditional block. */
                        this->SkipConditionalBlock(true);
                    }
                    break;
                case CheatVmOpcodeType_BeginExtendedKeypressConditionalBlock:
                    /* Check for keypress. */
                    if (!cur_opcode.begin_ext_keypress_cond.auto_repeat) {
                        if ((cur_opcode.begin_ext_keypress_cond.key_mask & kHeld) != (cur_opcode.begin_ext_keypress_cond.key_mask) || (cur_opcode.begin_ext_keypress_cond.key_mask & s_keyold) == (cur_opcode.begin_ext_keypress_cond.key_mask)) {
                            /* Keys not pressed. Skip conditional block. */
                            this->SkipConditionalBlock(true);
                        }
                    } else if ((cur_opcode.begin_ext_keypress_cond.key_mask & kHeld) != cur_opcode.begin_ext_keypress_cond.key_mask) {
                        /* Keys not pressed. Skip conditional block. */
                        this->SkipConditionalBlock(true);
                    }
                    break;
                case CheatVmOpcodeType_PerformArithmeticRegister:
                    {
                        const u64 operand_1_value = m_registers[cur_opcode.perform_math_reg.src_reg_1_index];
                        const u64 operand_2_value = cur_opcode.perform_math_reg.has_immediate ?
                                                    GetVmInt(cur_opcode.perform_math_reg.value, cur_opcode.perform_math_reg.bit_width) :
                                                    m_registers[cur_opcode.perform_math_reg.src_reg_2_index];

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
                            case RegisterArithmeticType_FloatAddition:
                                if (cur_opcode.perform_math_reg.bit_width == 4) {
                                    res_val = std::bit_cast<std::uint32_t>(std::bit_cast<float>(static_cast<uint32_t>(operand_1_value)) + std::bit_cast<float>(static_cast<uint32_t>(operand_2_value)));
                                } else if (cur_opcode.perform_math_reg.bit_width == 8) {
                                    res_val = std::bit_cast<std::uint64_t>(std::bit_cast<double>(operand_1_value) + std::bit_cast<double>(operand_2_value));
                                }
                                break;
                            case RegisterArithmeticType_FloatSubtraction:
                                if (cur_opcode.perform_math_reg.bit_width == 4) {
                                    res_val = std::bit_cast<std::uint32_t>(std::bit_cast<float>(static_cast<uint32_t>(operand_1_value)) - std::bit_cast<float>(static_cast<uint32_t>(operand_2_value)));
                                } else if (cur_opcode.perform_math_reg.bit_width == 8) {
                                    res_val = std::bit_cast<std::uint64_t>(std::bit_cast<double>(operand_1_value) - std::bit_cast<double>(operand_2_value));
                                }
                                break;
                            case RegisterArithmeticType_FloatMultiplication:
                                if (cur_opcode.perform_math_reg.bit_width == 4) {
                                    res_val = std::bit_cast<std::uint32_t>(std::bit_cast<float>(static_cast<uint32_t>(operand_1_value)) * std::bit_cast<float>(static_cast<uint32_t>(operand_2_value)));
                                } else if (cur_opcode.perform_math_reg.bit_width == 8) {
                                    res_val = std::bit_cast<std::uint64_t>(std::bit_cast<double>(operand_1_value) * std::bit_cast<double>(operand_2_value));
                                }
                                break;
                            case RegisterArithmeticType_FloatDivision:
                                if (cur_opcode.perform_math_reg.bit_width == 4) {
                                    res_val = std::bit_cast<std::uint32_t>(std::bit_cast<float>(static_cast<uint32_t>(operand_1_value)) / std::bit_cast<float>(static_cast<uint32_t>(operand_2_value)));
                                } else if (cur_opcode.perform_math_reg.bit_width == 8) {
                                    res_val = std::bit_cast<std::uint64_t>(std::bit_cast<double>(operand_1_value) / std::bit_cast<double>(operand_2_value));
                                }
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
                        m_registers[cur_opcode.perform_math_reg.dst_reg_index] = res_val;
                    }
                    break;
                case CheatVmOpcodeType_StoreRegisterToAddress:
                    {
                        /* Calculate address. */
                        u64 dst_value   = m_registers[cur_opcode.str_register.str_reg_index];
                        u64 dst_address = m_registers[cur_opcode.str_register.addr_reg_index];
                        switch (cur_opcode.str_register.ofs_type) {
                            case StoreRegisterOffsetType_None:
                                /* Nothing more to do */
                                break;
                            case StoreRegisterOffsetType_Reg:
                                dst_address += m_registers[cur_opcode.str_register.ofs_reg_index];
                                break;
                            case StoreRegisterOffsetType_Imm:
                                dst_address += cur_opcode.str_register.rel_address;
                                break;
                            case StoreRegisterOffsetType_MemReg:
                                dst_address = GetCheatProcessAddress(metadata, cur_opcode.str_register.mem_type, m_registers[cur_opcode.str_register.addr_reg_index]);
                                break;
                            case StoreRegisterOffsetType_MemImm:
                                dst_address = GetCheatProcessAddress(metadata, cur_opcode.str_register.mem_type, cur_opcode.str_register.rel_address);
                                break;
                            case StoreRegisterOffsetType_MemImmReg:
                                dst_address = GetCheatProcessAddress(metadata, cur_opcode.str_register.mem_type, m_registers[cur_opcode.str_register.addr_reg_index] + cur_opcode.str_register.rel_address);
                                break;
                        }

                        /* Write value to memory. Write only on valid bitwidth. */
                        switch (cur_opcode.str_register.bit_width) {
                            case 1:
                            case 2:
                            case 4:
                            case 8:
                                dmnt::cheat::impl::WriteCheatProcessMemoryUnsafe(dst_address, std::addressof(dst_value), cur_opcode.str_register.bit_width);
                                break;
                        }

                        /* Increment register if relevant. */
                        if (cur_opcode.str_register.increment_reg) {
                            m_registers[cur_opcode.str_register.addr_reg_index] += cur_opcode.str_register.bit_width;
                        }
                    }
                    break;
                case CheatVmOpcodeType_BeginRegisterConditionalBlock:
                    {
                        /* Get value from register. */
                        u64 src_value = 0;
                        switch (cur_opcode.begin_reg_cond.bit_width) {
                            case 1:
                                src_value = static_cast<u8>(m_registers[cur_opcode.begin_reg_cond.val_reg_index]  & 0xFFul);
                                break;
                            case 2:
                                src_value = static_cast<u16>(m_registers[cur_opcode.begin_reg_cond.val_reg_index] & 0xFFFFul);
                                break;
                            case 4:
                                src_value = static_cast<u32>(m_registers[cur_opcode.begin_reg_cond.val_reg_index] & 0xFFFFFFFFul);
                                break;
                            case 8:
                                src_value = static_cast<u64>(m_registers[cur_opcode.begin_reg_cond.val_reg_index] & 0xFFFFFFFFFFFFFFFFul);
                                break;
                        }

                        /* Read value from memory. */
                        u64 cond_value = 0;
                        if (cur_opcode.begin_reg_cond.comp_type == CompareRegisterValueType_StaticValue) {
                            cond_value = GetVmInt(cur_opcode.begin_reg_cond.value, cur_opcode.begin_reg_cond.bit_width);
                        } else if (cur_opcode.begin_reg_cond.comp_type == CompareRegisterValueType_OtherRegister) {
                            switch (cur_opcode.begin_reg_cond.bit_width) {
                                case 1:
                                    cond_value = static_cast<u8>(m_registers[cur_opcode.begin_reg_cond.other_reg_index]  & 0xFFul);
                                    break;
                                case 2:
                                    cond_value = static_cast<u16>(m_registers[cur_opcode.begin_reg_cond.other_reg_index] & 0xFFFFul);
                                    break;
                                case 4:
                                    cond_value = static_cast<u32>(m_registers[cur_opcode.begin_reg_cond.other_reg_index] & 0xFFFFFFFFul);
                                    break;
                                case 8:
                                    cond_value = static_cast<u64>(m_registers[cur_opcode.begin_reg_cond.other_reg_index] & 0xFFFFFFFFFFFFFFFFul);
                                    break;
                            }
                        } else {
                            u64 cond_address = 0;
                            switch (cur_opcode.begin_reg_cond.comp_type) {
                                case CompareRegisterValueType_MemoryRelAddr:
                                    cond_address = GetCheatProcessAddress(metadata, cur_opcode.begin_reg_cond.mem_type, cur_opcode.begin_reg_cond.rel_address);
                                    break;
                                case CompareRegisterValueType_MemoryOfsReg:
                                    cond_address = GetCheatProcessAddress(metadata, cur_opcode.begin_reg_cond.mem_type, m_registers[cur_opcode.begin_reg_cond.ofs_reg_index]);
                                    break;
                                case CompareRegisterValueType_RegisterRelAddr:
                                    cond_address = m_registers[cur_opcode.begin_reg_cond.addr_reg_index] + cur_opcode.begin_reg_cond.rel_address;
                                    break;
                                case CompareRegisterValueType_RegisterOfsReg:
                                    cond_address = m_registers[cur_opcode.begin_reg_cond.addr_reg_index] + m_registers[cur_opcode.begin_reg_cond.ofs_reg_index];
                                    break;
                                default:
                                    break;
                            }
                            switch (cur_opcode.begin_reg_cond.bit_width) {
                                case 1:
                                case 2:
                                case 4:
                                case 8:
                                    dmnt::cheat::impl::ReadCheatProcessMemoryUnsafe(cond_address, std::addressof(cond_value), cur_opcode.begin_reg_cond.bit_width);
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
                            this->SkipConditionalBlock(true);
                        }
                    }
                    break;
                case CheatVmOpcodeType_SaveRestoreRegister:
                    /* Save or restore a register. */
                    switch (cur_opcode.save_restore_reg.op_type) {
                        case SaveRestoreRegisterOpType_ClearRegs:
                            m_registers[cur_opcode.save_restore_reg.dst_index] = 0ul;
                            break;
                        case SaveRestoreRegisterOpType_ClearSaved:
                            m_saved_values[cur_opcode.save_restore_reg.dst_index] = 0ul;
                            break;
                        case SaveRestoreRegisterOpType_Save:
                            m_saved_values[cur_opcode.save_restore_reg.dst_index] = m_registers[cur_opcode.save_restore_reg.src_index];
                            break;
                        case SaveRestoreRegisterOpType_Restore:
                        default:
                            m_registers[cur_opcode.save_restore_reg.dst_index] = m_saved_values[cur_opcode.save_restore_reg.src_index];
                            break;
                    }
                    break;
                case CheatVmOpcodeType_SaveRestoreRegisterMask:
                    /* Save or restore register mask. */
                    u64 *src;
                    u64 *dst;
                    switch (cur_opcode.save_restore_regmask.op_type) {
                        case SaveRestoreRegisterOpType_ClearSaved:
                        case SaveRestoreRegisterOpType_Save:
                            src = m_registers;
                            dst = m_saved_values;
                            break;
                        case SaveRestoreRegisterOpType_ClearRegs:
                        case SaveRestoreRegisterOpType_Restore:
                        default:
                            src = m_saved_values;
                            dst = m_registers;
                            break;
                    }
                    for (size_t i = 0; i < NumRegisters; i++) {
                        if (cur_opcode.save_restore_regmask.should_operate[i]) {
                            switch (cur_opcode.save_restore_regmask.op_type) {
                                case SaveRestoreRegisterOpType_ClearSaved:
                                case SaveRestoreRegisterOpType_ClearRegs:
                                    dst[i] = 0ul;
                                    break;
                                case SaveRestoreRegisterOpType_Save:
                                case SaveRestoreRegisterOpType_Restore:
                                default:
                                    dst[i] = src[i];
                                    break;
                            }
                        }
                    }
                    break;
                case CheatVmOpcodeType_ReadWriteStaticRegister:
                    if (cur_opcode.rw_static_reg.static_idx < NumReadableStaticRegisters) {
                        /* Load a register with a static register. */
                        m_registers[cur_opcode.rw_static_reg.idx] = m_static_registers[cur_opcode.rw_static_reg.static_idx];
                    } else {
                        /* Store a register to a static register. */
                        m_static_registers[cur_opcode.rw_static_reg.static_idx] = m_registers[cur_opcode.rw_static_reg.idx];
                    }
                    break;
                case CheatVmOpcodeType_PauseProcess:
                    dmnt::cheat::impl::PauseCheatProcessUnsafe();
                    break;
                case CheatVmOpcodeType_ResumeProcess:
                    dmnt::cheat::impl::ResumeCheatProcessUnsafe();
                    break;
                case CheatVmOpcodeType_DebugLog:
                    {
                        /* Read value from memory. */
                        u64 log_value = 0;
                        if (cur_opcode.debug_log.val_type == DebugLogValueType_RegisterValue) {
                            switch (cur_opcode.debug_log.bit_width) {
                                case 1:
                                    log_value = static_cast<u8>(m_registers[cur_opcode.debug_log.val_reg_index]  & 0xFFul);
                                    break;
                                case 2:
                                    log_value = static_cast<u16>(m_registers[cur_opcode.debug_log.val_reg_index] & 0xFFFFul);
                                    break;
                                case 4:
                                    log_value = static_cast<u32>(m_registers[cur_opcode.debug_log.val_reg_index] & 0xFFFFFFFFul);
                                    break;
                                case 8:
                                    log_value = static_cast<u64>(m_registers[cur_opcode.debug_log.val_reg_index] & 0xFFFFFFFFFFFFFFFFul);
                                    break;
                            }
                        } else {
                            u64 val_address = 0;
                            switch (cur_opcode.debug_log.val_type) {
                                case DebugLogValueType_MemoryRelAddr:
                                    val_address = GetCheatProcessAddress(metadata, cur_opcode.debug_log.mem_type, cur_opcode.debug_log.rel_address);
                                    break;
                                case DebugLogValueType_MemoryOfsReg:
                                    val_address = GetCheatProcessAddress(metadata, cur_opcode.debug_log.mem_type, m_registers[cur_opcode.debug_log.ofs_reg_index]);
                                    break;
                                case DebugLogValueType_RegisterRelAddr:
                                    val_address = m_registers[cur_opcode.debug_log.addr_reg_index] + cur_opcode.debug_log.rel_address;
                                    break;
                                case DebugLogValueType_RegisterOfsReg:
                                    val_address = m_registers[cur_opcode.debug_log.addr_reg_index] + m_registers[cur_opcode.debug_log.ofs_reg_index];
                                    break;
                                default:
                                    break;
                            }
                            switch (cur_opcode.debug_log.bit_width) {
                                case 1:
                                case 2:
                                case 4:
                                case 8:
                                    dmnt::cheat::impl::ReadCheatProcessMemoryUnsafe(val_address, std::addressof(log_value), cur_opcode.debug_log.bit_width);
                                    break;
                            }
                        }

                        /* Log value. */
                        this->DebugLog(cur_opcode.debug_log.log_id, log_value);
                    }
                    break;
                default:
                    /* By default, we do a no-op. */
                    break;
            }
        }
        s_keyold = kHeld;
    }

}
