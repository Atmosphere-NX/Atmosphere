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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <malloc.h>
#include <sys/iosupport.h>
#include "utils.h"

void __libc_init_array(void);
void __libc_fini_array(void);

extern uint8_t __bss_start__[], __bss_end__[];
extern uint8_t __heap_start__[], __heap_end__[];

extern char *fake_heap_start;
extern char *fake_heap_end;

int __program_argc;
void **__program_argv;

void __attribute__((noreturn)) __program_exit(int rc);
void __attribute__((noreturn)) (*__program_exit_callback)(int rc) = NULL;

static void __program_parse_argc_argv(int argc, char *argdata);
static void __program_cleanup_argv(void);

static void __program_init_heap(void) {
    fake_heap_start = (char*)__heap_start__;
    fake_heap_end   = (char*)__heap_end__;
}

static void __program_init_newlib_hooks(void) {
    __syscalls.exit = __program_exit; /* For exit, etc. */
}

static void __program_move_additional_sections(void) {
#if defined(FUSEE_STAGE1_SRC) || defined(FUSEE_STAGE2_SRC)
    extern uint8_t __chainloader_lma__[], __chainloader_start__[], __chainloader_bss_start__[], __chainloader_end__[];
    extern uint8_t __nxboot_lma__[], __nxboot_start__[], __nxboot_bss_start__[], __nxboot_end__[];
    memcpy(__chainloader_start__, __chainloader_lma__, __chainloader_bss_start__ - __chainloader_start__);
    memset(__chainloader_bss_start__, 0, __chainloader_end__ - __chainloader_bss_start__);
    memcpy(__nxboot_start__, __nxboot_lma__, __nxboot_bss_start__ - __nxboot_start__);
    memset(__nxboot_bss_start__, 0, __nxboot_end__ - __nxboot_bss_start__);
#endif
}

void __program_init(int argc, char *argdata) {
    /* Zero-fill the .bss section */
    memset(__bss_start__, 0, __bss_end__ - __bss_start__);

    __program_init_heap();
    __program_init_newlib_hooks();
    __program_parse_argc_argv(argc, argdata);

    /* Once argv is parsed, we can discard the low IRAM region */
    __program_move_additional_sections();
    __libc_init_array();
}

void __program_exit(int rc) {
    __libc_fini_array();
    __program_cleanup_argv();
    if (__program_exit_callback == NULL) {
        /* Default callback */
        generic_panic();
    } else {
        __program_exit_callback(rc);
    }
    for (;;);
}

#ifdef FUSEE_STAGE1_SRC
static void __program_parse_argc_argv(int argc, char *argdata) {
    __program_argc = 0;
    __program_argv = NULL;
}
#elif defined(FUSEE_STAGE2_SRC)
#include "stage2.h"
static void __program_parse_argc_argv(int argc, char *argdata) {
    size_t pos = 0, len;

    __program_argc = argc;

    __program_argv = malloc(argc * sizeof(void **));
    if (__program_argv == NULL) {
        generic_panic();
    }

    len = strlen(argdata);
    __program_argv[0] = malloc(len + 1);
    if (__program_argv[0] == NULL) {
        generic_panic();
    }
    strcpy((char *)__program_argv[0], argdata);
    pos += len + 1;

    __program_argv[1] = malloc(sizeof(stage2_args_t));
    if (__program_argv[1] == NULL) {
        generic_panic();
    }
    memcpy(__program_argv[1], argdata + pos, sizeof(stage2_args_t));
}
#else
static void __program_parse_argc_argv(int argc, char *argdata) {
    size_t pos = 0, len;

    __program_argc = argc;

    __program_argv = malloc(argc * sizeof(void **));
    if (__program_argv == NULL) {
        generic_panic();
    }

    for (int i = 0; i < argc; i++) {
        len = strlen(argdata + pos);
        __program_argv[i] = malloc(len + 1);
        if (__program_argv[i] == NULL) {
            generic_panic();
        }
        strcpy((char *)__program_argv[i], argdata + pos);
        pos += len + 1;
    }
}
#endif

static void __program_cleanup_argv(void) {
#ifndef FUSEE_STAGE1_SRC
    for (int i = 0; i < __program_argc; i++) {
        free(__program_argv[i]);
        __program_argv[i] = NULL;
    }
    free(__program_argv);
#endif
}
