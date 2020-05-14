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

#define __ACCESS_TABLE_NAME__    PmcAccessTable
#define __ACCESS_TABLE_ADDRESS__ MemoryRegionPhysicalDevicePmc.GetAddress()
#define __ACCESS_TABLE_INC__     "secmon_pmc_access_table_data.inc"

#include "secmon_define_access_table.inc"

#undef __ACCESS_TABLE_INC__
#undef __ACCESS_TABLE_ADDRESS__
#undef __ACCESS_TABLE_NAME__
