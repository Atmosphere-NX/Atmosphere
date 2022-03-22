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
#include <stratosphere/diag/impl/diag_impl_build_config.hpp>
#include <stratosphere/diag/impl/diag_impl_log.hpp>

#if defined(AMS_IMPL_ENABLE_LOG) || defined(AMS_IMPL_ENABLE_SDK_LOG)

#define AMS_IMPL_LOG_META_DATA(_MOD_, _SEV_, _VER_)        \
    (::ams::diag::LogMetaData {                            \
        { __LINE__, __FILE__, AMS_CURRENT_FUNCTION_NAME }, \
        _MOD_,                                             \
        _SEV_,                                             \
        _VER_,                                             \
        false,                                             \
        nullptr,                                           \
        0,                                                 \
    })

#define AMS_IMPL_STRUCTURED_LOG_IMPL(_MOD_, _SEV_, _VER_, ...) ::ams::diag::impl::LogImpl(AMS_IMPL_LOG_META_DATA(_MOD_, _SEV_, _VER_), __VA_ARGS__)
#define AMS_IMPL_STRUCTURED_VLOG_IMPL(_MOD_, _SEV_, _VER_, _FMT_, _VL_) ::ams::diag::impl::VLogImpl(AMS_IMPL_LOG_META_DATA(_MOD_, _SEV_, _VER_), _FMT_, _VL_)
#define AMS_IMPL_STRUCTURED_PUT_IMPL(_MOD_, _SEV_, _VER_, _MSG_, _ML_) ::ams::diag::impl::PutImpl(AMS_IMPL_LOG_META_DATA(_MOD_, _SEV_, _VER_), _MSG_, _ML_)

#else

#define AMS_IMPL_STRUCTURED_LOG_IMPL(_MOD_, _SEV_, _VER_, ...)          do { /* ... */ } while (false)
#define AMS_IMPL_STRUCTURED_VLOG_IMPL(_MOD_, _SEV_, _VER_, _FMT_, _VL_) do { /* ... */ } while (false)
#define AMS_IMPL_STRUCTURED_PUT_IMPL(_MOD_, _SEV_, _VER_, _MSG_, _ML_)  do { /* ... */ } while (false)

#endif
