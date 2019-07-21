/*
 * Copyright (c) 2019 Atmosphère-NX
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
#include "utils.h"

typedef struct ExceptionStackFrame {
    u64 x[31]; // x0 .. x30
    u64 sp_el1;
    union {
        u64 sp_el2;
        u64 sp_el0;
    };
    u64 elr_el2;
    u64 spsr_el2;
} ExceptionStackFrame;