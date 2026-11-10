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
/* #1 - Set TotalAvailableMemorySize to 1GB. */
/* #2 - Change the root heap "Reserved" region allocation size from 0x43E000 to 0. */
/* #3 - Patch the initial memory block allocation size from 0xB400000 to 0xB000000. */
/* #4 - Patch the initial memory block heap size from 0xB400000 to 0xB000000. */
/* #5 - Patch the browser engine memory capacity from 0xAA00000 to 0x9000000. */
/* #6 - Patch the browser engine memory allocation size from 0xAA00000 to 0x9000000. */
constexpr inline const EmbeddedPatchEntry SystemWebMemoryPatches_23_0_0[] = {
    { 0x6113A8, "\x09\x00\xA8\xD2", 4 },
    { 0x861E80, "\x02\x00\x80\x52", 4 },
    { 0x861D44, "\x01\x60\xA1\x52", 4 },
    { 0x861D5C, "\x03\x60\xA1\x52", 4 },
    { 0x8623D0, "\x01\x20\xA1\x52", 4 },
    { 0x862498, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry SystemWebMemoryPatches_23_5_0[] = {
    { 0x611468, "\x09\x00\xA8\xD2", 4 },
    { 0x729FE8, "\x02\x00\x80\x52", 4 },
    { 0x729EA8, "\x01\x60\xA1\x52", 4 },
    { 0x729EC0, "\x03\x60\xA1\x52", 4 },
    { 0x72A53C, "\x01\x20\xA1\x52", 4 },
    { 0x72A604, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OpenWebMemoryPatches_23_0_0[] = {
    { 0x611428, "\x09\x00\xA8\xD2", 4 },
    { 0x828E68, "\x02\x00\x80\x52", 4 },
    { 0x828D2C, "\x01\x60\xA1\x52", 4 },
    { 0x828D44, "\x03\x60\xA1\x52", 4 },
    { 0x8293B8, "\x01\x20\xA1\x52", 4 },
    { 0x829480, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OpenWebMemoryPatches_23_5_0[] = {
    { 0x6114E8, "\x09\x00\xA8\xD2", 4 },
    { 0x8E4C70, "\x02\x00\x80\x52", 4 },
    { 0x8E4B30, "\x01\x60\xA1\x52", 4 },
    { 0x8E4B48, "\x03\x60\xA1\x52", 4 },
    { 0x8E51C4, "\x01\x20\xA1\x52", 4 },
    { 0x8E528C, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_23_0_0[] = {
    { 0x6DE0DC, "\x09\x00\xA8\xD2", 4 },
    { 0x130744, "\x02\x00\x80\x52", 4 },
    { 0x130614, "\x01\x60\xA1\x52", 4 },
    { 0x130624, "\x02\x60\xA1\x52", 4 },
    { 0x130C30, "\x01\x20\xA1\x52", 4 },
    { 0x130CD0, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry OfflineWebMemoryPatches_23_5_0[] = {
    { 0x6E28FC, "\x09\x00\xA8\xD2", 4 },
    { 0x168144, "\x02\x00\x80\x52", 4 },
    { 0x168014, "\x01\x60\xA1\x52", 4 },
    { 0x168024, "\x02\x60\xA1\x52", 4 },
    { 0x168630, "\x01\x20\xA1\x52", 4 },
    { 0x1686D0, "\x01\x20\xA1\x52", 4 },
};

constexpr inline const EmbeddedPatch WebMemoryPatches[] = {
    { ParseModuleId("97BF9C2D43EB002FB5B3578E7714F428104ED596"), util::size(SystemWebMemoryPatches_23_0_0),  SystemWebMemoryPatches_23_0_0  },      /* 23.0.0 - 23.0.1 */
    { ParseModuleId("BFC1836C97C456CA4E56DEABF7FBF275E5522363"), util::size(SystemWebMemoryPatches_23_5_0),  SystemWebMemoryPatches_23_5_0  },      /* 23.5.0 */
    
    { ParseModuleId("1470453424A9CDE689BE6DA86457ED1C79F5F403"), util::size(OpenWebMemoryPatches_23_0_0),    OpenWebMemoryPatches_23_0_0    },      /* 23.0.0 - 23.0.1 */
    { ParseModuleId("C01A479EB77D4118D092EEF9F7E6AA0F8D1B0FE1"), util::size(OpenWebMemoryPatches_23_5_0),    OpenWebMemoryPatches_23_5_0    },      /* 23.5.0 */
    
    { ParseModuleId("86DAFFA68DA38660317018ADC7AD415BCCDDC583"), util::size(OfflineWebMemoryPatches_23_0_0), OfflineWebMemoryPatches_23_0_0 },      /* 23.0.0 - 23.0.1 */
    { ParseModuleId("0F20276C46BAEFF61762EBB4765DFA08EB56385B"), util::size(OfflineWebMemoryPatches_23_5_0), OfflineWebMemoryPatches_23_5_0 },      /* 23.5.0 */
};
