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
#include "stage2.h"
#include "utils.h"

void __libc_init_array(void);
void __libc_fini_array(void);

extern uint8_t __bss_start__[], __bss_end__[];
extern uint8_t __heap_start__[], __heap_end__[];

extern char *fake_heap_start;
extern char *fake_heap_end;

int __program_argc;
void **__program_argv;

void __program_exit(int rc);
static void __program_parse_argc_argv(int argc, char *argdata);
static void __program_cleanup_argv(void);

static void __program_init_heap(void) {
    fake_heap_start = (char*)__heap_start__;
    fake_heap_end   = (char*)__heap_end__;
}

static void __program_init_newlib_hooks(void) {
    __syscalls.exit = __program_exit; /* For exit, etc. */
}

void __program_init(int argc, char *argdata) {
    /* Zero-fill the .bss section */
    memset(__bss_start__, 0, __bss_end__ - __bss_start__);
    
    __program_init_heap();
    __program_init_newlib_hooks();
    __program_parse_argc_argv(argc, argdata);
    __libc_init_array();
}

void __program_exit(int rc) {
    __libc_fini_array();
    __program_cleanup_argv();
}

static void __program_parse_argc_argv(int argc, char *argdata) {
    __program_argc = argc;

    __program_argv = malloc(argc * sizeof(void **));
    if (__program_argv == NULL) {
        generic_panic();
    }
    
    __program_argv[0] = malloc(sizeof(stage2_mtc_args_t));
    if (__program_argv[0] == NULL) {
        generic_panic();
    }
    memcpy(__program_argv[0], argdata, sizeof(stage2_mtc_args_t));
}

static void __program_cleanup_argv(void) {
    for (int i = 0; i < __program_argc; i++) {
        free(__program_argv[i]);
        __program_argv[i] = NULL;
    }
    free(__program_argv);
}
