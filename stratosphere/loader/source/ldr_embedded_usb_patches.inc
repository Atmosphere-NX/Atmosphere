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

/* Patch fallback case to mov w0, #1 rather than retrieving settings flag. */
constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_9_0_0[] = {
    { 0x521C, "\x20\x00\x80\x52", 4 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_10_0_0[] = {
    { 0x5494, "\x20\x00\x80\x52", 4 },
};

/* Patch getter functions to return 1. */
constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_11_0_0[] = {
    { 0x85DC, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x866C, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_12_0_0[] = {
    { 0x840C, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x849C, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_13_0_0[] = {
    { 0x8424, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x84B4, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatch Usb30ForceEnablePatches[] = {
    { ParseModuleId("C0D3F4E87E8B0FE9BBE9F1968A20767F3DC08E03"), util::size(Usb30ForceEnablePatches_9_0_0),  Usb30ForceEnablePatches_9_0_0 },
    { ParseModuleId("B9C700CA8335F8BAA0D2041D8D09F772890BA988"), util::size(Usb30ForceEnablePatches_10_0_0), Usb30ForceEnablePatches_10_0_0 },
    { ParseModuleId("95BAF06A69650C215A5DD50CF8BD2A603E7AD3C2"), util::size(Usb30ForceEnablePatches_11_0_0), Usb30ForceEnablePatches_11_0_0 },
    { ParseModuleId("F3F88D90EF6B3B6358EDEBAF87D56FA88D3A081C"), util::size(Usb30ForceEnablePatches_12_0_0), Usb30ForceEnablePatches_12_0_0 }, /* 12.0.0 - 12.0.3 */
    { ParseModuleId("A109046278AA99353C20EC38B57C495B74A06D91"), util::size(Usb30ForceEnablePatches_12_0_0), Usb30ForceEnablePatches_12_0_0 }, /* 12.1.0 */
    { ParseModuleId("69384BF4A543C62C4342C94E5E2CED3C5A675251"), util::size(Usb30ForceEnablePatches_13_0_0), Usb30ForceEnablePatches_13_0_0 }, /* 13.0.0 - 13.1.0 */
};
