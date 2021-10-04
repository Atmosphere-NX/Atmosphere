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

#pragma once
#include <vapours.hpp>
#include <stratosphere/fs/fs_file.hpp>

namespace ams::fs::fsa {

    class IFile;

}

namespace ams::util::ini {

    /* Ini handler type. */
    using Handler = int (*)(void *user_ctx, const char *section, const char *name, const char *value);

    /* Utilities for dealing with INI file configuration. */
    int ParseString(const char *ini_str, void *user_ctx, Handler h);
    int ParseFile(fs::FileHandle file, void *user_ctx, Handler h);
    int ParseFile(fs::fsa::IFile *file, void *user_ctx, Handler h);

}