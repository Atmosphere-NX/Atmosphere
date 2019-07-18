/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <cstdio>
#include <switch.h>

namespace sts::util::ini {

    /* Ini handler type. */
    using Handler = int (*)(void *user_ctx, const char *section, const char *name, const char *value);

    /* Utilities for dealing with INI file configuration. */
    int ParseString(const char *ini_str, void *user_ctx, Handler h);
    int ParseFile(FILE *f, void *user_ctx, Handler h);
    int ParseFile(FsFile *f, void *user_ctx, Handler h);
    int ParseFile(const char *path, void *user_ctx, Handler h);

}