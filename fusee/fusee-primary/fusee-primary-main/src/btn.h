/*
 * Copyright (c) 2018 naehrwert
 * Copyright (c) 2018 CTCaer
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
 
#ifndef FUSEE_BTN_H_
#define FUSEE_BTN_H_

#define BTN_POWER 0x1
#define BTN_VOL_DOWN 0x2
#define BTN_VOL_UP 0x4

uint32_t btn_read();
uint32_t btn_wait();
uint32_t btn_wait_timeout(uint32_t time_ms, uint32_t mask);

#endif