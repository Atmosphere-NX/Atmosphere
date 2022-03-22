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
#include <stratosphere/diag/impl/diag_impl_structured_log.hpp>

#if defined(AMS_IMPL_ENABLE_SDK_LOG)

#define AMS_SDK_LOG(...)          AMS_IMPL_STRUCTURED_LOG_IMPL("$", ::ams::diag::LogSeverity_Info, 0, __VA_ARGS__)
#define AMS_SDK_VLOG(_FMT_, _VL_) AMS_IMPL_STRUCTURED_VLOG_IMPL("$", ::ams::diag::LogSeverity_Info, 0, _FMT_, _VL_)
#define AMS_SDK_PUT(_MSG_, _ML_)  AMS_IMPL_STRUCTURED_PUT_IMPL("$", ::ams::diag::LogSeverity_Info, 0, _MSG_, _ML_)

#else

#define AMS_SDK_LOG(...)          do { /* ... */ } while (false)
#define AMS_SDK_VLOG(_FMT_, _VL_) do { /* ... */ } while (false)
#define AMS_SDK_PUT(_MSG_, _ML_)  do { /* ... */ } while (false)

#endif