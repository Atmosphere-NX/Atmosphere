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
#include <vapours/literals.hpp>
#include <vapours/util.hpp>
#include <vapours/results.hpp>
#include <vapours/reg.hpp>

#define EVP_CPU_RESET_VECTOR          (0x100)

#define EVP_COP_RESET_VECTOR          (0x200)
#define EVP_COP_UNDEF_VECTOR          (0x204)
#define EVP_COP_SWI_VECTOR            (0x208)
#define EVP_COP_PREFETCH_ABORT_VECTOR (0x20C)
#define EVP_COP_DATA_ABORT_VECTOR     (0x210)
#define EVP_COP_RSVD_VECTOR           (0x214)
#define EVP_COP_IRQ_VECTOR            (0x218)
#define EVP_COP_FIQ_VECTOR            (0x21C)
