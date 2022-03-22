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
#include <stratosphere/diag/diag_log_types.hpp>

namespace ams::diag::impl {

    void LogImpl(const LogMetaData &meta, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
    void VLogImpl(const LogMetaData &meta, const char *fmt, std::va_list vl);
    void PutImpl(const LogMetaData &meta, const char *msg, size_t msg_size);

}
