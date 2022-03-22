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

extern "C" int __wrap_sprintf (char *str, const char *fmt, ...) {
    AMS_UNUSED(fmt);

    str[0] = '\x00';
    return 0;
}

extern "C" int __wrap_fprintf(void *stream, const char *fmt, ...) {
    AMS_UNUSED(stream, fmt);
    return 0;
}

extern "C" void *__wrap_getenv(const char *) {
    return nullptr;
}

extern "C" void __wrap_sscanf(const char *str, const char *format, ...) {
    AMS_UNUSED(str, format);
    AMS_ABORT("sscanf was called\n");
}
