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
 
#include "log_device_null.h"

static void initialize(debug_log_device_null_t *this) {
    (void)this;
    /* Do nothing */
}

static void write_string(debug_log_device_null_t *this, const char *str, size_t len) {
    (void)this;
    (void)str;
    (void)len;
    /* Do nothing */
}

static void finalize(debug_log_device_null_t *this) {
    (void)this;
    /* Do nothing */
}

debug_log_device_null_t g_debug_log_device_null = {
    .super = {
        .initialize     = (void (*)(debug_log_device_t *, ...))initialize,
        .write_string   = (void (*)(debug_log_device_t *, const char *, size_t))write_string,
        .finalize       = (void (*)(debug_log_device_t *))finalize,
    },
};
