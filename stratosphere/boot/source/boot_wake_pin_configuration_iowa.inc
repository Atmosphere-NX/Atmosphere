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

static constexpr WakePinConfig WakePinConfigsIowa[] = {
    {0x00, false, 0x02},
    {0x01, false, 0x02},
    {0x02, false, 0x02},
    {0x03, false, 0x02},
    {0x04, true,  0x02},
    {0x05, false, 0x02},
    {0x06, true,  0x02},
    {0x07, true,  0x02},
    {0x08, true,  0x01},
    {0x0A, true,  0x02},
    {0x0B, false, 0x02},
    {0x0C, false, 0x02},
    {0x0D, false, 0x02},
    {0x0E, true,  0x00},
    {0x0F, false, 0x02},
    {0x11, false, 0x02},
    {0x12, false, 0x02},
    {0x13, false, 0x02},
    {0x14, false, 0x02},
    {0x15, false, 0x02},
    {0x16, false, 0x02},
    {0x17, false, 0x02},
    {0x18, false, 0x02},
    {0x19, false, 0x02},
    {0x1A, false, 0x02},
    {0x1B, true,  0x00},
    {0x1C, false, 0x02},
    {0x21, false, 0x02},
    {0x22, true,  0x00},
    {0x23, true,  0x02},
    {0x24, false, 0x02},
    {0x2D, false, 0x02},
    {0x2E, false, 0x02},
    {0x2F, false, 0x02},
    {0x30, true,  0x02},
    {0x31, false, 0x02},
    {0x32, false, 0x02},
    {0x33, true,  0x00},
    {0x34, true,  0x00},
    {0x35, false, 0x02},
    {0x36, false, 0x02},
    {0x37, false, 0x02},
    {0x38, false, 0x02},
    {0x39, false, 0x02},
    {0x3A, false, 0x02},
    {0x3B, false, 0x02},
    {0x3D, false, 0x02},
    {0x3E, false, 0x02},
    {0x3F, false, 0x02},
};

static constexpr size_t NumWakePinConfigsIowa = util::size(WakePinConfigsIowa);
