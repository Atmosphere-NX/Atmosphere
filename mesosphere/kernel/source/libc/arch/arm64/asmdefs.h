/*
 * Macros for asm code.
 *
 * Copyright (c) 2019, Arm Limited.
 * SPDX-License-Identifier: MIT
 */

#ifndef _ASMDEFS_H
#define _ASMDEFS_H

#define ENTRY_ALIGN(name, alignment)    \
  .global name;                         \
  .type name,%function;                 \
  .align alignment;                     \
  name:                                 \
  .cfi_startproc;

#define ENTRY(name)    ENTRY_ALIGN(name, 6)

#define ENTRY_ALIAS(name)   \
  .global name;             \
  .type name,%function;     \
  name:

#define END(name)       \
  .cfi_endproc;         \
  .size name, .-name;

#define L(l) .L ## l

#endif
