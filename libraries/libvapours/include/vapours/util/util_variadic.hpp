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

#pragma once
#include <vapours/common.hpp>
#include <vapours/assert.hpp>

#define AMS_UTIL_VARIADIC_INVOKE_MACRO(__HANDLER__) \
    __HANDLER__(_01_)                               \
    __HANDLER__(_02_)                               \
    __HANDLER__(_03_)                               \
    __HANDLER__(_04_)                               \
    __HANDLER__(_05_)                               \
    __HANDLER__(_06_)                               \
    __HANDLER__(_07_)                               \
    __HANDLER__(_08_)                               \
    __HANDLER__(_09_)                               \
    __HANDLER__(_0A_)                               \
    __HANDLER__(_0B_)                               \
    __HANDLER__(_0C_)                               \
    __HANDLER__(_0D_)                               \
    __HANDLER__(_0E_)                               \
    __HANDLER__(_0F_)

#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_01_(_T_)                                                 typename _T_##_01_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_02_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_01_(_T_), typename _T_##_02_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_03_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_02_(_T_), typename _T_##_03_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_04_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_03_(_T_), typename _T_##_04_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_05_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_04_(_T_), typename _T_##_05_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_06_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_05_(_T_), typename _T_##_06_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_07_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_06_(_T_), typename _T_##_07_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_08_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_07_(_T_), typename _T_##_08_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_09_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_08_(_T_), typename _T_##_09_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0A_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_09_(_T_), typename _T_##_0A_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0B_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0A_(_T_), typename _T_##_0B_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0C_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0B_(_T_), typename _T_##_0C_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0D_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0C_(_T_), typename _T_##_0D_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0E_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0D_(_T_), typename _T_##_0E_
#define AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0F_(_T_) AMS_UTIL_VARIADIC_TEMPLATE_PARAMETERS_0E_(_T_), typename _T_##_0F_

#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_01_(_T_, _N_)                                                     _T_##_01_ &&_N_##_01_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_02_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_01_(_T_, _N_), _T_##_02_ &&_N_##_02_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_03_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_02_(_T_, _N_), _T_##_03_ &&_N_##_03_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_04_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_03_(_T_, _N_), _T_##_04_ &&_N_##_04_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_05_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_04_(_T_, _N_), _T_##_05_ &&_N_##_05_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_06_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_05_(_T_, _N_), _T_##_06_ &&_N_##_06_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_07_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_06_(_T_, _N_), _T_##_07_ &&_N_##_07_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_08_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_07_(_T_, _N_), _T_##_08_ &&_N_##_08_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_09_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_08_(_T_, _N_), _T_##_09_ &&_N_##_09_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0A_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_09_(_T_, _N_), _T_##_0A_ &&_N_##_0A_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0B_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0A_(_T_, _N_), _T_##_0B_ &&_N_##_0B_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0C_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0B_(_T_, _N_), _T_##_0C_ &&_N_##_0C_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0D_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0C_(_T_, _N_), _T_##_0D_ &&_N_##_0D_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0E_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0D_(_T_, _N_), _T_##_0E_ &&_N_##_0E_
#define AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0F_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_ARGUMENTS_0E_(_T_, _N_), _T_##_0F_ &&_N_##_0F_

#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_01_(_T_, _N_)                                                    ::std::forward<_T_##_01_>(_N_##_01_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_02_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_01_(_T_, _N_), ::std::forward<_T_##_02_>(_N_##_02_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_03_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_02_(_T_, _N_), ::std::forward<_T_##_03_>(_N_##_03_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_04_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_03_(_T_, _N_), ::std::forward<_T_##_04_>(_N_##_04_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_05_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_04_(_T_, _N_), ::std::forward<_T_##_05_>(_N_##_05_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_06_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_05_(_T_, _N_), ::std::forward<_T_##_06_>(_N_##_06_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_07_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_06_(_T_, _N_), ::std::forward<_T_##_07_>(_N_##_07_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_08_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_07_(_T_, _N_), ::std::forward<_T_##_08_>(_N_##_08_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_09_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_08_(_T_, _N_), ::std::forward<_T_##_09_>(_N_##_09_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0A_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_09_(_T_, _N_), ::std::forward<_T_##_0A_>(_N_##_0A_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0B_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0A_(_T_, _N_), ::std::forward<_T_##_0B_>(_N_##_0B_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0C_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0B_(_T_, _N_), ::std::forward<_T_##_0C_>(_N_##_0C_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0D_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0C_(_T_, _N_), ::std::forward<_T_##_0D_>(_N_##_0D_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0E_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0D_(_T_, _N_), ::std::forward<_T_##_0E_>(_N_##_0E_)
#define AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0F_(_T_, _N_) AMS_UTIL_VARIADIC_TEMPLATE_FORWARDS_0E_(_T_, _N_), ::std::forward<_T_##_0F_>(_N_##_0F_)
