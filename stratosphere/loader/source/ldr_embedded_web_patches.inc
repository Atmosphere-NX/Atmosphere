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

/* Breakdown of each patch in order: */ 
/* #1 - Patch the 0xDEEF000 size check branch to NOP. */
/* #2 - Change the root heap "Reserved" region allocation size from 0x43E000 to 0. */
/* #3 - Change the root heap "Scene" size from 0x4580000 to 0x4040000. */
/* #4 - Change the scene heap "ReservedTbd" region allocation size from 0x540000 to 0. */
/* #5 - Patch the linking of "ReservedTbd" to the parent heap with a NOP. */
/* #6 - Patch the initial memory block allocation size from 0xB400000 to 0xAC00000. */
/* #7 - Patch the initial memory block initialization size from 0xB400000 to 0xAC00000. */
constexpr inline const EmbeddedPatchEntry SystemWebMemoryPatches_23_0_0[] = {
    { 0x861DD4, "\x1F\x20\x03\xD5", 4 },
    { 0x861E80, "\x02\x00\x80\x52", 4 },
    { 0x861E6C, "\x82\x80\xA0\x52", 4 },
    { 0x8C1718, "\x02\x00\x80\x52", 4 },
    { 0x8C172C, "\x1F\x20\x03\xD5", 4 },
    { 0x861D44, "\x01\x58\xA1\x52", 4 },
    { 0x861D5C, "\x03\x58\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OpenWebMemoryPatches_23_0_0[] = {
    { 0x828DBC, "\x1F\x20\x03\xD5", 4 },
    { 0x828E68, "\x02\x00\x80\x52", 4 },
    { 0x828E54, "\x82\x80\xA0\x52", 4 },
    { 0x74C564, "\x02\x00\x80\x52", 4 },
    { 0x74C578, "\x1F\x20\x03\xD5", 4 },
    { 0x828D2C, "\x01\x58\xA1\x52", 4 },
    { 0x828D44, "\x03\x58\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_23_0_0[] = {
    { 0x130698, "\x1F\x20\x03\xD5", 4 },
    { 0x130744, "\x02\x00\x80\x52", 4 },
    { 0x130730, "\x82\x80\xA0\x52", 4 },
    { 0x4D728,  "\x02\x00\x80\x52", 4 },
    { 0x4D738,  "\x1F\x20\x03\xD5", 4 },
    { 0x130614, "\x01\x58\xA1\x52", 4 },
    { 0x130624, "\x02\x58\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatch WebMemoryPatches[] = {
    { ParseModuleId("97BF9C2D43EB002FB5B3578E7714F428104ED596"), util::size(SystemWebMemoryPatches_23_0_0),  SystemWebMemoryPatches_23_0_0  },      /* 23.0.0 */
    
    { ParseModuleId("1470453424A9CDE689BE6DA86457ED1C79F5F403"), util::size(OpenWebMemoryPatches_23_0_0),    OpenWebMemoryPatches_23_0_0    },      /* 23.0.0 */
    
    { ParseModuleId("86DAFFA68DA38660317018ADC7AD415BCCDDC583"), util::size(OfflineWebMemoryPatches_23_0_0), OfflineWebMemoryPatches_23_0_0 },      /* 23.0.0 */
};
