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

/* All kernel code should have access to libvapours. */
#include <vapours.hpp>

/* First, pull in core macros (panic, etc). */
#include "mesosphere/kern_panic.hpp"

/* Primitive types. */
#include "mesosphere/kern_k_typed_address.hpp"

/* Core functionality. */
#include "mesosphere/kern_select_k_system_control.hpp"
