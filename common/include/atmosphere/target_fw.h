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
 
#ifndef ATMOSPHERE_TARGET_FIRMWARE_H
#define ATMOSPHERE_TARGET_FIRMWARE_H

#define ATMOSPHERE_TARGET_FIRMWARE_100 1
#define ATMOSPHERE_TARGET_FIRMWARE_200 2
#define ATMOSPHERE_TARGET_FIRMWARE_300 3
#define ATMOSPHERE_TARGET_FIRMWARE_400 4
#define ATMOSPHERE_TARGET_FIRMWARE_500 5
#define ATMOSPHERE_TARGET_FIRMWARE_600 6
#define ATMOSPHERE_TARGET_FIRMWARE_620 7
#define ATMOSPHERE_TARGET_FIRMWARE_700 8
#define ATMOSPHERE_TARGET_FIRMWARE_800 9

#define ATMOSPHERE_TARGET_FIRMWARE_CURRENT ATMOSPHERE_TARGET_FIRMWARE_800

#define ATMOSPHERE_TARGET_FIRMWARE_MIN ATMOSPHERE_TARGET_FIRMWARE_100
#define ATMOSPHERE_TARGET_FIRMWARE_MAX ATMOSPHERE_TARGET_FIRMWARE_800

/* TODO: What should this be, for release? */
#define ATMOSPHERE_TARGET_FIRMWARE_DEFAULT_FOR_DEBUG ATMOSPHERE_TARGET_FIRMWARE_CURRENT

#endif