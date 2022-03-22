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
#include <exosphere.hpp>
#pragma once

namespace ams::nxboot {

    NORETURN void ExceptionHandler();

    NORETURN void ExceptionHandler0();
    NORETURN void ExceptionHandler1();
    NORETURN void ExceptionHandler2();
    NORETURN void ExceptionHandler3();
    NORETURN void ExceptionHandler4();
    NORETURN void ExceptionHandler5();
    NORETURN void ExceptionHandler6();
    NORETURN void ExceptionHandler7();

}