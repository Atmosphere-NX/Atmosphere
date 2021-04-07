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

#define THREAD_STACK_PARAMETERS_SIZE                    0x30
#define THREAD_STACK_PARAMETERS_SVC_PERMISSION          0x00
#define THREAD_STACK_PARAMETERS_CONTEXT                 0x18
#define THREAD_STACK_PARAMETERS_CUR_THREAD              0x20
#define THREAD_STACK_PARAMETERS_DISABLE_COUNT           0x28
#define THREAD_STACK_PARAMETERS_DPC_FLAGS               0x2A
#define THREAD_STACK_PARAMETERS_CURRENT_SVC_ID          0x2B
#define THREAD_STACK_PARAMETERS_IS_CALLING_SVC          0x2C
#define THREAD_STACK_PARAMETERS_IS_IN_EXCEPTION_HANDLER 0x2D
#define THREAD_STACK_PARAMETERS_IS_PINNED               0x2E