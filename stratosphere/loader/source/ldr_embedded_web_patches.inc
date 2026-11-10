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

/* The following patches reduce memory usage by the browser applets: */
/*   OfflineWeb (LibAppletOff): 20.0.0+ */
/*   SystemWeb, OpenWeb: 22.0.0+ (these applets were added in 22.0.0) */
/*   LibAppletWeb, LibAppletShop, LibAppletLns, LibAppletAuth: 20.0.0 - 21.2.0 (stubs from 22.0.0+) */
/* Breakdown of each patch in order: */
/* #1 - Set TotalAvailableMemorySize to 1GB. */
/* #2 - Change the root heap "Reserved" region allocation size to 0 (from 0xF7E000 on 20.x, 0x47E000 on 21.0.0-22.0.0, 0x43E000 on 22.5.0+). */
/* #3 - Patch the initial memory block allocation size (from 0xAE00000 to 0xAA00000 on 20.x, from 0xB400000 to 0xB000000 on 21.0.0+). */
/* #4 - Patch the initial memory block heap size from 0xB400000 to 0xB000000 (21.0.0+) */
/* #5 - Patch the browser engine memory capacity from 0xAA00000 to 0x9000000. */
/* #6 - Patch the browser engine memory allocation size from 0xAA00000 to 0x9000000. */

constexpr inline const EmbeddedPatchEntry SystemWebMemoryPatches_22_0_0[] = {
    { 0x69CC48, "\x09\x00\xA8\xD2", 4 },
    { 0x82B6BC, "\x02\x00\x80\x52", 4 },
    { 0x82B580, "\x01\x60\xA1\x52", 4 },
    { 0x82B598, "\x03\x60\xA1\x52", 4 },
    { 0x82BC0C, "\x01\x20\xA1\x52", 4 },
    { 0x82BCD4, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry SystemWebMemoryPatches_22_5_0[] = {
    { 0x69CB68, "\x09\x00\xA8\xD2", 4 },
    { 0x823770, "\x02\x00\x80\x52", 4 },
    { 0x823634, "\x01\x60\xA1\x52", 4 },
    { 0x82364C, "\x03\x60\xA1\x52", 4 },
    { 0x823CC0, "\x01\x20\xA1\x52", 4 },
    { 0x823D88, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry SystemWebMemoryPatches_23_0_0[] = {
    { 0x6113A8, "\x09\x00\xA8\xD2", 4 },
    { 0x861E80, "\x02\x00\x80\x52", 4 },
    { 0x861D44, "\x01\x60\xA1\x52", 4 },
    { 0x861D5C, "\x03\x60\xA1\x52", 4 },
    { 0x8623D0, "\x01\x20\xA1\x52", 4 },
    { 0x862498, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OpenWebMemoryPatches_22_0_0[] = {
    { 0x69CCC8, "\x09\x00\xA8\xD2", 4 },
    { 0x7A0E6C, "\x02\x00\x80\x52", 4 },
    { 0x7A0D30, "\x01\x60\xA1\x52", 4 },
    { 0x7A0D48, "\x03\x60\xA1\x52", 4 },
    { 0x7A13BC, "\x01\x20\xA1\x52", 4 },
    { 0x7A1484, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OpenWebMemoryPatches_22_5_0[] = {
    { 0x69CBE8, "\x09\x00\xA8\xD2", 4 },
    { 0x87BA38, "\x02\x00\x80\x52", 4 },
    { 0x87B8FC, "\x01\x60\xA1\x52", 4 },
    { 0x87B914, "\x03\x60\xA1\x52", 4 },
    { 0x87BF88, "\x01\x20\xA1\x52", 4 },
    { 0x87C050, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OpenWebMemoryPatches_23_0_0[] = {
    { 0x611428, "\x09\x00\xA8\xD2", 4 },
    { 0x828E68, "\x02\x00\x80\x52", 4 },
    { 0x828D2C, "\x01\x60\xA1\x52", 4 },
    { 0x828D44, "\x03\x60\xA1\x52", 4 },
    { 0x8293B8, "\x01\x20\xA1\x52", 4 },
    { 0x829480, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_20_0_0[] = {
    { 0x6C3DB4, "\x09\x00\xA8\xD2", 4 },
    { 0x098C64, "\x02\x00\x80\x52", 4 },
    { 0x098B78, "\x19\x54\xA1\x52", 4 },
    { 0x098B98, "\x01\x20\xA1\x52", 4 },
    { 0x0997B8, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_20_1_0[] = {
    { 0x6C4504, "\x09\x00\xA8\xD2", 4 },
    { 0x0F93F0, "\x02\x00\x80\x52", 4 },
    { 0x0F9304, "\x19\x54\xA1\x52", 4 },
    { 0x0F9324, "\x01\x20\xA1\x52", 4 },
    { 0x01A92C, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_20_2_0[] = {
    { 0x6C4784, "\x09\x00\xA8\xD2", 4 },
    { 0x0E47BC, "\x02\x00\x80\x52", 4 },
    { 0x0E46D0, "\x19\x54\xA1\x52", 4 },
    { 0x0E46F0, "\x01\x20\xA1\x52", 4 },
    { 0x154EE4, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_20_3_0[] = {
    { 0x6C4664, "\x09\x00\xA8\xD2", 4 },
    { 0x025E2C, "\x02\x00\x80\x52", 4 },
    { 0x025D40, "\x19\x54\xA1\x52", 4 },
    { 0x025D60, "\x01\x20\xA1\x52", 4 },
    { 0x03DF80, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_20_4_0[] = {
    { 0x6C46E4, "\x09\x00\xA8\xD2", 4 },
    { 0x044364, "\x02\x00\x80\x52", 4 },
    { 0x044278, "\x19\x54\xA1\x52", 4 },
    { 0x044298, "\x01\x20\xA1\x52", 4 },
    { 0x130020, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_21_0_0[] = {
    { 0x71416C, "\x09\x00\xA8\xD2", 4 },
    { 0x188E48, "\x02\x00\x80\x52", 4 },
    { 0x188D60, "\x01\x60\xA1\x52", 4 },
    { 0x188D70, "\x02\x60\xA1\x52", 4 },
    { 0x1891C0, "\x01\x20\xA1\x52", 4 },
    { 0x189260, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_21_1_0[] = {
    { 0x7141EC, "\x09\x00\xA8\xD2", 4 },
    { 0x14F768, "\x02\x00\x80\x52", 4 },
    { 0x14F680, "\x01\x60\xA1\x52", 4 },
    { 0x14F690, "\x02\x60\xA1\x52", 4 },
    { 0x14FAE0, "\x01\x20\xA1\x52", 4 },
    { 0x14FB80, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_21_2_0[] = {
    { 0x7141EC, "\x09\x00\xA8\xD2", 4 },
    { 0x1054F8, "\x02\x00\x80\x52", 4 },
    { 0x105410, "\x01\x60\xA1\x52", 4 },
    { 0x105420, "\x02\x60\xA1\x52", 4 },
    { 0x105870, "\x01\x20\xA1\x52", 4 },
    { 0x105910, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_22_0_0[] = {
    { 0x738C9C, "\x09\x00\xA8\xD2", 4 },
    { 0x198704, "\x02\x00\x80\x52", 4 },
    { 0x1985D4, "\x01\x60\xA1\x52", 4 },
    { 0x1985E4, "\x02\x60\xA1\x52", 4 },
    { 0x198BF0, "\x01\x20\xA1\x52", 4 },
    { 0x198C90, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_22_5_0[] = {
    { 0x73DBBC, "\x09\x00\xA8\xD2", 4 },
    { 0x199CD4, "\x02\x00\x80\x52", 4 },
    { 0x199BA4, "\x01\x60\xA1\x52", 4 },
    { 0x199BB4, "\x02\x60\xA1\x52", 4 },
    { 0x19A1C0, "\x01\x20\xA1\x52", 4 },
    { 0x19A260, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_23_0_0[] = {
    { 0x6DE0DC, "\x09\x00\xA8\xD2", 4 },
    { 0x130744, "\x02\x00\x80\x52", 4 },
    { 0x130614, "\x01\x60\xA1\x52", 4 },
    { 0x130624, "\x02\x60\xA1\x52", 4 },
    { 0x130C30, "\x01\x20\xA1\x52", 4 },
    { 0x130CD0, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_20_0_0[] = {
    { 0x602C18, "\x09\x00\xA8\xD2", 4 },
    { 0x89F484, "\x02\x00\x80\x52", 4 },
    { 0x89F3B0, "\x17\x54\xA1\x52", 4 },
    { 0x89F3D0, "\x01\x20\xA1\x52", 4 },
    { 0x7E6D54, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_20_1_0[] = {
    { 0x603038, "\x09\x00\xA8\xD2", 4 },
    { 0x7825D8, "\x02\x00\x80\x52", 4 },
    { 0x782504, "\x17\x54\xA1\x52", 4 },
    { 0x782524, "\x01\x20\xA1\x52", 4 },
    { 0x7CF260, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_20_2_0[] = {
    { 0x603058, "\x09\x00\xA8\xD2", 4 },
    { 0x8E7724, "\x02\x00\x80\x52", 4 },
    { 0x8E7650, "\x17\x54\xA1\x52", 4 },
    { 0x8E7670, "\x01\x20\xA1\x52", 4 },
    { 0x8B6640, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_20_3_0[] = {
    { 0x603038, "\x09\x00\xA8\xD2", 4 },
    { 0x8454B0, "\x02\x00\x80\x52", 4 },
    { 0x8453DC, "\x17\x54\xA1\x52", 4 },
    { 0x8453FC, "\x01\x20\xA1\x52", 4 },
    { 0x861194, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_20_4_0[] = {
    { 0x603038, "\x09\x00\xA8\xD2", 4 },
    { 0x851254, "\x02\x00\x80\x52", 4 },
    { 0x851180, "\x17\x54\xA1\x52", 4 },
    { 0x8511A0, "\x01\x20\xA1\x52", 4 },
    { 0x765A94, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_21_0_0[] = {
    { 0x645778, "\x09\x00\xA8\xD2", 4 },
    { 0x8D3A5C, "\x02\x00\x80\x52", 4 },
    { 0x8D3960, "\x01\x60\xA1\x52", 4 },
    { 0x8D3978, "\x03\x60\xA1\x52", 4 },
    { 0x8D3E8C, "\x01\x20\xA1\x52", 4 },
    { 0x8D3F54, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_21_1_0[] = {
    { 0x645778, "\x09\x00\xA8\xD2", 4 },
    { 0x7CB370, "\x02\x00\x80\x52", 4 },
    { 0x7CB274, "\x01\x60\xA1\x52", 4 },
    { 0x7CB28C, "\x03\x60\xA1\x52", 4 },
    { 0x7CB7A0, "\x01\x20\xA1\x52", 4 },
    { 0x7CB868, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletWebMemoryPatches_21_2_0[] = {
    { 0x645778, "\x09\x00\xA8\xD2", 4 },
    { 0x7B3730, "\x02\x00\x80\x52", 4 },
    { 0x7B3634, "\x01\x60\xA1\x52", 4 },
    { 0x7B364C, "\x03\x60\xA1\x52", 4 },
    { 0x7B3B60, "\x01\x20\xA1\x52", 4 },
    { 0x7B3C28, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_20_0_0[] = {
    { 0x6026F8, "\x09\x00\xA8\xD2", 4 },
    { 0x86C250, "\x02\x00\x80\x52", 4 },
    { 0x86C17C, "\x17\x54\xA1\x52", 4 },
    { 0x86C19C, "\x01\x20\xA1\x52", 4 },
    { 0x833D50, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_20_1_0[] = {
    { 0x602B18, "\x09\x00\xA8\xD2", 4 },
    { 0x8E1AF8, "\x02\x00\x80\x52", 4 },
    { 0x8E1A24, "\x17\x54\xA1\x52", 4 },
    { 0x8E1A44, "\x01\x20\xA1\x52", 4 },
    { 0x7FDEB8, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_20_2_0[] = {
    { 0x602B18, "\x09\x00\xA8\xD2", 4 },
    { 0x7B217C, "\x02\x00\x80\x52", 4 },
    { 0x7B20A8, "\x17\x54\xA1\x52", 4 },
    { 0x7B20C8, "\x01\x20\xA1\x52", 4 },
    { 0x893104, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_20_3_0[] = {
    { 0x602B18, "\x09\x00\xA8\xD2", 4 },
    { 0x8372AC, "\x02\x00\x80\x52", 4 },
    { 0x8371D8, "\x17\x54\xA1\x52", 4 },
    { 0x8371F8, "\x01\x20\xA1\x52", 4 },
    { 0x898E20, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_20_4_0[] = {
    { 0x602B18, "\x09\x00\xA8\xD2", 4 },
    { 0x860744, "\x02\x00\x80\x52", 4 },
    { 0x860670, "\x17\x54\xA1\x52", 4 },
    { 0x860690, "\x01\x20\xA1\x52", 4 },
    { 0x7E4B64, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_21_0_0[] = {
    { 0x6451B8, "\x09\x00\xA8\xD2", 4 },
    { 0x82EA30, "\x02\x00\x80\x52", 4 },
    { 0x82E934, "\x01\x60\xA1\x52", 4 },
    { 0x82E94C, "\x03\x60\xA1\x52", 4 },
    { 0x82EE60, "\x01\x20\xA1\x52", 4 },
    { 0x82EF28, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_21_1_0[] = {
    { 0x6451B8, "\x09\x00\xA8\xD2", 4 },
    { 0x76C37C, "\x02\x00\x80\x52", 4 },
    { 0x76C280, "\x01\x60\xA1\x52", 4 },
    { 0x76C298, "\x03\x60\xA1\x52", 4 },
    { 0x76C7AC, "\x01\x20\xA1\x52", 4 },
    { 0x76C874, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletShopMemoryPatches_21_2_0[] = {
    { 0x6451B8, "\x09\x00\xA8\xD2", 4 },
    { 0x811200, "\x02\x00\x80\x52", 4 },
    { 0x811104, "\x01\x60\xA1\x52", 4 },
    { 0x81111C, "\x03\x60\xA1\x52", 4 },
    { 0x811630, "\x01\x20\xA1\x52", 4 },
    { 0x8116F8, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_20_0_0[] = {
    { 0x6034D8, "\x09\x00\xA8\xD2", 4 },
    { 0x8B12F8, "\x02\x00\x80\x52", 4 },
    { 0x8B1224, "\x17\x54\xA1\x52", 4 },
    { 0x8B1244, "\x01\x20\xA1\x52", 4 },
    { 0x72F204, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_20_1_0[] = {
    { 0x6038F8, "\x09\x00\xA8\xD2", 4 },
    { 0x78C334, "\x02\x00\x80\x52", 4 },
    { 0x78C260, "\x17\x54\xA1\x52", 4 },
    { 0x78C280, "\x01\x20\xA1\x52", 4 },
    { 0x7F7924, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_20_2_0[] = {
    { 0x6038F8, "\x09\x00\xA8\xD2", 4 },
    { 0x878C14, "\x02\x00\x80\x52", 4 },
    { 0x878B40, "\x17\x54\xA1\x52", 4 },
    { 0x878B60, "\x01\x20\xA1\x52", 4 },
    { 0x84CA10, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_20_3_0[] = {
    { 0x6038F8, "\x09\x00\xA8\xD2", 4 },
    { 0x7C7078, "\x02\x00\x80\x52", 4 },
    { 0x7C6FA4, "\x17\x54\xA1\x52", 4 },
    { 0x7C6FC4, "\x01\x20\xA1\x52", 4 },
    { 0x8AFA50, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_20_4_0[] = {
    { 0x6038F8, "\x09\x00\xA8\xD2", 4 },
    { 0x899930, "\x02\x00\x80\x52", 4 },
    { 0x89985C, "\x17\x54\xA1\x52", 4 },
    { 0x89987C, "\x01\x20\xA1\x52", 4 },
    { 0x7C7104, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_21_0_0[] = {
    { 0x6460D8, "\x09\x00\xA8\xD2", 4 },
    { 0x8FB570, "\x02\x00\x80\x52", 4 },
    { 0x8FB474, "\x01\x60\xA1\x52", 4 },
    { 0x8FB48C, "\x03\x60\xA1\x52", 4 },
    { 0x8FB9A0, "\x01\x20\xA1\x52", 4 },
    { 0x8FBA68, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_21_1_0[] = {
    { 0x6460D8, "\x09\x00\xA8\xD2", 4 },
    { 0x89C750, "\x02\x00\x80\x52", 4 },
    { 0x89C654, "\x01\x60\xA1\x52", 4 },
    { 0x89C66C, "\x03\x60\xA1\x52", 4 },
    { 0x89CB80, "\x01\x20\xA1\x52", 4 },
    { 0x89CC48, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletLnsMemoryPatches_21_2_0[] = {
    { 0x6460D8, "\x09\x00\xA8\xD2", 4 },
    { 0x7EC258, "\x02\x00\x80\x52", 4 },
    { 0x7EC15C, "\x01\x60\xA1\x52", 4 },
    { 0x7EC174, "\x03\x60\xA1\x52", 4 },
    { 0x7EC688, "\x01\x20\xA1\x52", 4 },
    { 0x7EC750, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_20_0_0[] = {
    { 0x6027F8, "\x09\x00\xA8\xD2", 4 },
    { 0x876B10, "\x02\x00\x80\x52", 4 },
    { 0x876A3C, "\x17\x54\xA1\x52", 4 },
    { 0x876A5C, "\x01\x20\xA1\x52", 4 },
    { 0x7E42E4, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_20_1_0[] = {
    { 0x602C18, "\x09\x00\xA8\xD2", 4 },
    { 0x87F410, "\x02\x00\x80\x52", 4 },
    { 0x87F33C, "\x17\x54\xA1\x52", 4 },
    { 0x87F35C, "\x01\x20\xA1\x52", 4 },
    { 0x85C7A0, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_20_2_0[] = {
    { 0x602C18, "\x09\x00\xA8\xD2", 4 },
    { 0x779EEC, "\x02\x00\x80\x52", 4 },
    { 0x779E18, "\x17\x54\xA1\x52", 4 },
    { 0x779E38, "\x01\x20\xA1\x52", 4 },
    { 0x84CE04, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_20_3_0[] = {
    { 0x602C18, "\x09\x00\xA8\xD2", 4 },
    { 0x7D45B8, "\x02\x00\x80\x52", 4 },
    { 0x7D44E4, "\x17\x54\xA1\x52", 4 },
    { 0x7D4504, "\x01\x20\xA1\x52", 4 },
    { 0x77DC6C, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_20_4_0[] = {
    { 0x602C18, "\x09\x00\xA8\xD2", 4 },
    { 0x86E804, "\x02\x00\x80\x52", 4 },
    { 0x86E730, "\x17\x54\xA1\x52", 4 },
    { 0x86E750, "\x01\x20\xA1\x52", 4 },
    { 0x809BD4, "\x14\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_21_0_0[] = {
    { 0x6452D8, "\x09\x00\xA8\xD2", 4 },
    { 0x8CB208, "\x02\x00\x80\x52", 4 },
    { 0x8CB10C, "\x01\x60\xA1\x52", 4 },
    { 0x8CB124, "\x03\x60\xA1\x52", 4 },
    { 0x8CB638, "\x01\x20\xA1\x52", 4 },
    { 0x8CB700, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_21_1_0[] = {
    { 0x6452D8, "\x09\x00\xA8\xD2", 4 },
    { 0x8285F8, "\x02\x00\x80\x52", 4 },
    { 0x8284FC, "\x01\x60\xA1\x52", 4 },
    { 0x828514, "\x03\x60\xA1\x52", 4 },
    { 0x828A28, "\x01\x20\xA1\x52", 4 },
    { 0x828AF0, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry LibAppletAuthMemoryPatches_21_2_0[] = {
    { 0x6452D8, "\x09\x00\xA8\xD2", 4 },
    { 0x85CE28, "\x02\x00\x80\x52", 4 },
    { 0x85CD2C, "\x01\x60\xA1\x52", 4 },
    { 0x85CD44, "\x03\x60\xA1\x52", 4 },
    { 0x85D258, "\x01\x20\xA1\x52", 4 },
    { 0x85D320, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatch WebMemoryPatches[] = {
    { ParseModuleId("2829EE946B0F07189C6D33837885245EB365203C"), util::size(SystemWebMemoryPatches_22_0_0),     SystemWebMemoryPatches_22_0_0     }, /* 22.0.0 */
    { ParseModuleId("6207C77F752ECAB177528F202B278B0E0185EAAC"), util::size(SystemWebMemoryPatches_22_5_0),     SystemWebMemoryPatches_22_5_0     }, /* 22.5.0 */
    { ParseModuleId("97BF9C2D43EB002FB5B3578E7714F428104ED596"), util::size(SystemWebMemoryPatches_23_0_0),     SystemWebMemoryPatches_23_0_0     }, /* 23.0.0, 23.0.1 */

    { ParseModuleId("8F9A450A2BAE2FD782D2F3012D187E9C69887D78"), util::size(OpenWebMemoryPatches_22_0_0),       OpenWebMemoryPatches_22_0_0       }, /* 22.0.0 */
    { ParseModuleId("B81358863C0A926F4ED1E8C359E989FB94657B4A"), util::size(OpenWebMemoryPatches_22_5_0),       OpenWebMemoryPatches_22_5_0       }, /* 22.5.0 */
    { ParseModuleId("1470453424A9CDE689BE6DA86457ED1C79F5F403"), util::size(OpenWebMemoryPatches_23_0_0),       OpenWebMemoryPatches_23_0_0       }, /* 23.0.0, 23.0.1 */

    { ParseModuleId("622D8F42C6065188BAB1CB8104B2D7EE41CCA13E"), util::size(OfflineWebMemoryPatches_20_0_0),    OfflineWebMemoryPatches_20_0_0    }, /* 20.0.0 */
    { ParseModuleId("AFD8A3629A1ED66FC4FFE7AF3DDA4FA8672DFCB5"), util::size(OfflineWebMemoryPatches_20_1_0),    OfflineWebMemoryPatches_20_1_0    }, /* 20.1.0 */
    { ParseModuleId("FB6E674A097EDC81EDE09EAC95CFD35086A4DB14"), util::size(OfflineWebMemoryPatches_20_2_0),    OfflineWebMemoryPatches_20_2_0    }, /* 20.2.0 */
    { ParseModuleId("320701EA1073B8D51D27F2C32A6CD6F52B34CA31"), util::size(OfflineWebMemoryPatches_20_3_0),    OfflineWebMemoryPatches_20_3_0    }, /* 20.3.0 */
    { ParseModuleId("9AE4D57AC926833F272A016D7A7300D9B126D36B"), util::size(OfflineWebMemoryPatches_20_4_0),    OfflineWebMemoryPatches_20_4_0    }, /* 20.4.0 */
    { ParseModuleId("1133C9FFE21B5CB6ACFCCE0C108C5C6811271B4F"), util::size(OfflineWebMemoryPatches_21_0_0),    OfflineWebMemoryPatches_21_0_0    }, /* 21.0.0 */
    { ParseModuleId("6279CBD9C99264D9A925853F06AFF36870310304"), util::size(OfflineWebMemoryPatches_21_1_0),    OfflineWebMemoryPatches_21_1_0    }, /* 21.1.0 */
    { ParseModuleId("CA99A5C236D55827BAE16A5BA7CE25E6447BC56E"), util::size(OfflineWebMemoryPatches_21_2_0),    OfflineWebMemoryPatches_21_2_0    }, /* 21.2.0 */
    { ParseModuleId("A29FC27457B635F050580D6E0CD5C8101E6BD8E8"), util::size(OfflineWebMemoryPatches_22_0_0),    OfflineWebMemoryPatches_22_0_0    }, /* 22.0.0 */
    { ParseModuleId("B3C2F688FF22C78507A46F58F20DE4BD56C738C3"), util::size(OfflineWebMemoryPatches_22_5_0),    OfflineWebMemoryPatches_22_5_0    }, /* 22.5.0 */
    { ParseModuleId("86DAFFA68DA38660317018ADC7AD415BCCDDC583"), util::size(OfflineWebMemoryPatches_23_0_0),    OfflineWebMemoryPatches_23_0_0    }, /* 23.0.0, 23.0.1 */

    { ParseModuleId("1D1D9178B291BBBA8610B26B1BCA6D71969C6C86"), util::size(LibAppletWebMemoryPatches_20_0_0),  LibAppletWebMemoryPatches_20_0_0  }, /* 20.0.0 */
    { ParseModuleId("CD26EB3829F5A5C20D1CA6B88E63DA01996C31C0"), util::size(LibAppletWebMemoryPatches_20_1_0),  LibAppletWebMemoryPatches_20_1_0  }, /* 20.1.0 */
    { ParseModuleId("A5ED0215087480331DD84030F3FD621D07879450"), util::size(LibAppletWebMemoryPatches_20_2_0),  LibAppletWebMemoryPatches_20_2_0  }, /* 20.2.0 */
    { ParseModuleId("6704B10C23E865CB5E9952ECD8C75DB062C9C071"), util::size(LibAppletWebMemoryPatches_20_3_0),  LibAppletWebMemoryPatches_20_3_0  }, /* 20.3.0 */
    { ParseModuleId("83A92BF44FF000049CF4ABB2451B749B02D3F71F"), util::size(LibAppletWebMemoryPatches_20_4_0),  LibAppletWebMemoryPatches_20_4_0  }, /* 20.4.0 */
    { ParseModuleId("C9B985DBEE3D18CCAC7A1A93E0215C427E8E36FB"), util::size(LibAppletWebMemoryPatches_21_0_0),  LibAppletWebMemoryPatches_21_0_0  }, /* 21.0.0 */
    { ParseModuleId("C51BAB9F438AB44C9B20376DA0741B290A4C0A39"), util::size(LibAppletWebMemoryPatches_21_1_0),  LibAppletWebMemoryPatches_21_1_0  }, /* 21.1.0 */
    { ParseModuleId("A6F252F143978EE8C849EF51FBBA0FFC64143285"), util::size(LibAppletWebMemoryPatches_21_2_0),  LibAppletWebMemoryPatches_21_2_0  }, /* 21.2.0 */

    { ParseModuleId("AE7E730B756118A0BB127942D0300FF2A07DCA1B"), util::size(LibAppletShopMemoryPatches_20_0_0), LibAppletShopMemoryPatches_20_0_0 }, /* 20.0.0 */
    { ParseModuleId("F0CE12877D5CDBB5B223716E2AAF99A1FC083DF7"), util::size(LibAppletShopMemoryPatches_20_1_0), LibAppletShopMemoryPatches_20_1_0 }, /* 20.1.0 */
    { ParseModuleId("4D1ABBED0F81C71C9D2BDB38B0978B8C388F4ADC"), util::size(LibAppletShopMemoryPatches_20_2_0), LibAppletShopMemoryPatches_20_2_0 }, /* 20.2.0 */
    { ParseModuleId("85814B6AB9B75B7F5DE9D76404C8830D786560D5"), util::size(LibAppletShopMemoryPatches_20_3_0), LibAppletShopMemoryPatches_20_3_0 }, /* 20.3.0 */
    { ParseModuleId("C458650FCA6C09C9B794F7453975193F14F5C711"), util::size(LibAppletShopMemoryPatches_20_4_0), LibAppletShopMemoryPatches_20_4_0 }, /* 20.4.0 */
    { ParseModuleId("BF55651BB9BA0AFA136719ABF8F1D40CED8FEC88"), util::size(LibAppletShopMemoryPatches_21_0_0), LibAppletShopMemoryPatches_21_0_0 }, /* 21.0.0 */
    { ParseModuleId("54D20BDBA14E469E63EA4FC6259767F7247B955C"), util::size(LibAppletShopMemoryPatches_21_1_0), LibAppletShopMemoryPatches_21_1_0 }, /* 21.1.0 */
    { ParseModuleId("41FFD38B2B6DE8CA8C22B2D31E95CB3FCFAC3A19"), util::size(LibAppletShopMemoryPatches_21_2_0), LibAppletShopMemoryPatches_21_2_0 }, /* 21.2.0 */

    { ParseModuleId("731B130A7E21D5715FF1BBA67861443BBDAFCEA3"), util::size(LibAppletLnsMemoryPatches_20_0_0),  LibAppletLnsMemoryPatches_20_0_0  }, /* 20.0.0 */
    { ParseModuleId("8BEE6B14BFF85295BB11EA1040008AB6F128422A"), util::size(LibAppletLnsMemoryPatches_20_1_0),  LibAppletLnsMemoryPatches_20_1_0  }, /* 20.1.0 */
    { ParseModuleId("4817F5F6CFB7B3C8927239A1E4614255BE733CF3"), util::size(LibAppletLnsMemoryPatches_20_2_0),  LibAppletLnsMemoryPatches_20_2_0  }, /* 20.2.0 */
    { ParseModuleId("ED993919FAA323EFCD9052C2EC6CFBCA40E476C3"), util::size(LibAppletLnsMemoryPatches_20_3_0),  LibAppletLnsMemoryPatches_20_3_0  }, /* 20.3.0 */
    { ParseModuleId("140A29C7AA57AD7E39A5B61ED9608412E805BEC7"), util::size(LibAppletLnsMemoryPatches_20_4_0),  LibAppletLnsMemoryPatches_20_4_0  }, /* 20.4.0 */
    { ParseModuleId("D96A3FAD7FA28250297AD2D4BE5D5FA550D590FB"), util::size(LibAppletLnsMemoryPatches_21_0_0),  LibAppletLnsMemoryPatches_21_0_0  }, /* 21.0.0 */
    { ParseModuleId("352C36008CCEF8E501705851647EE1C108F3181B"), util::size(LibAppletLnsMemoryPatches_21_1_0),  LibAppletLnsMemoryPatches_21_1_0  }, /* 21.1.0 */
    { ParseModuleId("BC6D50E47F3189136590B073C33B38C8AB860EBD"), util::size(LibAppletLnsMemoryPatches_21_2_0),  LibAppletLnsMemoryPatches_21_2_0  }, /* 21.2.0 */

    { ParseModuleId("AF9BCD9FBFE04B7203C1548CEBA620B88730CB90"), util::size(LibAppletAuthMemoryPatches_20_0_0), LibAppletAuthMemoryPatches_20_0_0 }, /* 20.0.0 */
    { ParseModuleId("E3C0E5084B4BB50ACDEC30E750681ECEB23DCDC8"), util::size(LibAppletAuthMemoryPatches_20_1_0), LibAppletAuthMemoryPatches_20_1_0 }, /* 20.1.0 */
    { ParseModuleId("2F37AD8DF750E669497C302D373C0A668D285FC1"), util::size(LibAppletAuthMemoryPatches_20_2_0), LibAppletAuthMemoryPatches_20_2_0 }, /* 20.2.0 */
    { ParseModuleId("ADC8F197A17504ACA92FB2A35BC4DBCA7A49F36E"), util::size(LibAppletAuthMemoryPatches_20_3_0), LibAppletAuthMemoryPatches_20_3_0 }, /* 20.3.0 */
    { ParseModuleId("E1B505FF1D2D9F63EDE90C17B833D6950C1DAB14"), util::size(LibAppletAuthMemoryPatches_20_4_0), LibAppletAuthMemoryPatches_20_4_0 }, /* 20.4.0 */
    { ParseModuleId("8F8E95E884CB33151867A4637F29D44115BB209E"), util::size(LibAppletAuthMemoryPatches_21_0_0), LibAppletAuthMemoryPatches_21_0_0 }, /* 21.0.0 */
    { ParseModuleId("82D6C108DB9461A1DA552FDBFE3AA44C124F182D"), util::size(LibAppletAuthMemoryPatches_21_1_0), LibAppletAuthMemoryPatches_21_1_0 }, /* 21.1.0 */
    { ParseModuleId("5E458A9D988611B09777BCD712007F4E0FF1846D"), util::size(LibAppletAuthMemoryPatches_21_2_0), LibAppletAuthMemoryPatches_21_2_0 }, /* 21.2.0 */
};
