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

constexpr u32 InvalidPadName = UINT32_MAX;

constexpr u32 Map[] = {
    InvalidPadName,  /* Invalid */
    0x000000CC,   /* Port Z, Pin 4 */
    0x00000024,   /* Port E, Pin 4 */
    0x0000003C,   /* Port H, Pin 4 */
    0x000000DA,   /* Port BB, Pin 2 */
    0x000000DB,   /* Port BB, Pin 3 */
    0x000000DC,   /* Port BB, Pin 4 */
    0x00000025,   /* Port E, Pin 5 */
    0x00000090,   /* Port S, Pin 0 */
    0x00000091,   /* Port S, Pin 1 */
    0x00000096,   /* Port S, Pin 6 */
    0x00000097,   /* Port S, Pin 7 */
    0x00000026,   /* Port E, Pin 6 */
    0x00000005,   /* Port A, Pin 5 */
    0x00000078,   /* Port P, Pin 0 */
    0x00000093,   /* Port S, Pin 3 */
    0x0000007D,   /* Port P, Pin 5 */
    0x0000007C,   /* Port P, Pin 4 */
    0x0000007B,   /* Port P, Pin 3 */
    0x0000007A,   /* Port P, Pin 2 */
    0x000000BC,   /* Port X, Pin 4 */
    0x000000AE,   /* Port V, Pin 6 */
    0x000000BA,   /* Port X, Pin 2 */
    0x000000B9,   /* Port X, Pin 1 */
    0x000000BD,   /* Port X, Pin 5 */
    0x000000BE,   /* Port X, Pin 6 */
    0x000000BF,   /* Port X, Pin 7 */
    0x000000C0,   /* Port Y, Pin 0 */
    0x000000C1,   /* Port Y, Pin 1 */
    0x000000A9,   /* Port V, Pin 1 */
    0x000000AA,   /* Port V, Pin 2 */
    0x00000055,   /* Port K, Pin 5 */
    0x000000AD,   /* Port V, Pin 5 */
    0x000000C8,   /* Port Z, Pin 0 */
    0x000000CA,   /* Port Z, Pin 2 */
    0x000000CB,   /* Port Z, Pin 3 */
    0x0000004F,   /* Port J, Pin 7 */
    0x00000050,   /* Port K, Pin 0 */
    0x00000051,   /* Port K, Pin 1 */
    0x00000052,   /* Port K, Pin 2 */
    0x00000054,   /* Port K, Pin 4 */
    0x00000056,   /* Port K, Pin 6 */
    0x00000057,   /* Port K, Pin 7 */
    0x00000053,   /* Port K, Pin 3 */
    0x000000E3,   /* Port CC, Pin 3 */
    0x00000038,   /* Port H, Pin 0 */
    0x00000039,   /* Port H, Pin 1 */
    0x0000003B,   /* Port H, Pin 3 */
    0x0000003D,   /* Port H, Pin 5 */
    0x0000003F,   /* Port H, Pin 7 */
    0x00000040,   /* Port I, Pin 0 */
    0x00000041,   /* Port I, Pin 1 */
    0x0000003E,   /* Port H, Pin 6 */
    0x000000E2,   /* Port CC, Pin 2 */
    0x000000E4,   /* Port CC, Pin 4 */
    0x0000003A,   /* Port H, Pin 2 */
    0x000000C9,   /* Port Z, Pin 1 */
    0x0000004D,   /* Port J, Pin 5 */
    0x00000058,   /* Port L, Pin 0 */
    0x0000003E,   /* Port H, Pin 6 */
    0x00000026,   /* Port E, Pin 6 */

    /* Copper only */
    InvalidPadName,  /* Invalid */
    0x00000033,   /* Port G, Pin 3 */
    0x0000001C,   /* Port D, Pin 4 */
    0x000000D9,   /* Port BB, Pin 1 */
    0x0000000C,   /* Port B, Pin 4 */
    0x0000000D,   /* Port B, Pin 5 */
    0x00000021,   /* Port E, Pin 1 */
    0x00000027,   /* Port E, Pin 7 */
    0x00000092,   /* Port S, Pin 2 */
    0x00000095,   /* Port S, Pin 5 */
    0x00000098,   /* Port T, Pin 0 */
    0x00000010,   /* Port C, Pin 0 */
    0x00000011,   /* Port C, Pin 1 */
    0x00000012,   /* Port C, Pin 2 */
    0x00000042,   /* Port I, Pin 2 */
    0x000000E6,   /* Port CC, Pin 6 */

    /* 2.0.0+ Copper only */
    0x000000AC,   /* Port V, Pin 4 */
    0x000000E1,   /* Port CC, Pin 1 */

    /* 5.0.0+ Copper only (unused) */
    0x00000056,   /* Port K, Pin 6 */

    /* 6.0.0+ */
    0x00000020,   /* Port E,  Pin 0 */
    0x00000021,   /* Port E,  Pin 1 */
    0x00000022,   /* Port E,  Pin 2 */
    0x00000023,   /* Port E,  Pin 3 */
    0x0000004C,   /* Port J,  Pin 4 */
    0x00000057,   /* Port K,  Pin 7 */
    0x00000027,   /* Port S,  Pin 4 */
    0x00000098,   /* Port T,  Pin 0 */
    0x00000099,   /* Port T,  Pin 1 */
    0x000000BB,   /* Port X,  Pin 3 */
    0x000000E5,   /* Port CC, Pin 5 */
    0x000000AB,   /* Port V,  Pin 3 */
    0x0000004E,   /* Port J,  Pin 6 */

    /* 7.0.0+ */
    0x00000032,   /* Port G,  Pin 2 */
    0x0000001B,   /* Port D,  Pin 3 */
    0x00000017,   /* Port C,  Pin 7 */
    0x00000018,   /* Port D,  Pin 0 */
    0x00000015,   /* Port C,  Pin 5 */
    0x00000016,   /* Port C,  Pin 6 */

};

static constexpr u32 PadNameMax = util::size(Map);
