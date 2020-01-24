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
 
#ifndef FUSEE_EXCEPTION_HANDLERS_H
#define FUSEE_EXCEPTION_HANDLERS_H

#include <stdint.h>
#include <stddef.h>

/* Copies up to len bytes, stops and returns the read length on data fault. */
size_t safecpy(void *dst, const void *src, size_t len);

void setup_exception_handlers(void);

#endif
