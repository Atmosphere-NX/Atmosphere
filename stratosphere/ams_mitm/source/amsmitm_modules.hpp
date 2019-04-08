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

enum MitmModuleId : u32 {
    MitmModuleId_FsMitm = 0,
    MitmModuleId_SetMitm = 1,
    MitmModuleId_BpcMitm = 2,
    MitmModuleId_NsMitm = 3,
    
    /* Always keep this at the end. */
    MitmModuleId_Count,
};

void LaunchAllMitmModules();

void WaitAllMitmModules();