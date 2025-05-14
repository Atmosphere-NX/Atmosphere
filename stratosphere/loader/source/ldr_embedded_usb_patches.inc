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

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_14_0_0[] = {
    { 0x7414, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x74A4, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_15_0_0[] = {
    { 0x7230, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x72AC, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_16_0_0[] = {
    { 0x718C, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x7208, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_17_0_0[] = {
    { 0x7170, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x71EC, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_18_0_0[] = {
    { 0x6DCC, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x6E48, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_19_0_0[] = {
    { 0x6D90, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x6E10, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatchEntry Usb30ForceEnablePatches_20_0_0[] = {
    { 0x7010, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
    { 0x7090, "\x20\x00\x80\x52\xC0\x03\x5F\xD6", 8 },
};

constexpr inline const EmbeddedPatch Usb30ForceEnablePatches[] = {
    { ParseModuleId("C0D3F4E87E8B0FE9BBE9F1968A20767F3DC08E03"), util::size(Usb30ForceEnablePatches_9_0_0),  Usb30ForceEnablePatches_9_0_0 },
    { ParseModuleId("B9C700CA8335F8BAA0D2041D8D09F772890BA988"), util::size(Usb30ForceEnablePatches_10_0_0), Usb30ForceEnablePatches_10_0_0 },
    { ParseModuleId("95BAF06A69650C215A5DD50CF8BD2A603E7AD3C2"), util::size(Usb30ForceEnablePatches_11_0_0), Usb30ForceEnablePatches_11_0_0 },
    { ParseModuleId("F3F88D90EF6B3B6358EDEBAF87D56FA88D3A081C"), util::size(Usb30ForceEnablePatches_12_0_0), Usb30ForceEnablePatches_12_0_0 }, /* 12.0.0 - 12.0.3 */
    { ParseModuleId("A109046278AA99353C20EC38B57C495B74A06D91"), util::size(Usb30ForceEnablePatches_12_0_0), Usb30ForceEnablePatches_12_0_0 }, /* 12.1.0 */
    { ParseModuleId("69384BF4A543C62C4342C94E5E2CED3C5A675251"), util::size(Usb30ForceEnablePatches_13_0_0), Usb30ForceEnablePatches_13_0_0 }, /* 13.0.0 - 13.1.0 */
    { ParseModuleId("1C97AFF30D48AFFEB74B28A530D30ABA0ABB9FFF"), util::size(Usb30ForceEnablePatches_14_0_0), Usb30ForceEnablePatches_14_0_0 }, /* 14.0.0 */
    { ParseModuleId("30B15A83E94D91750E7470795414AD1AE9C6A8DB"), util::size(Usb30ForceEnablePatches_15_0_0), Usb30ForceEnablePatches_15_0_0 }, /* 15.0.0 */
    { ParseModuleId("225865A442B4B66E8BD14B3E9450B901BDF29A40"), util::size(Usb30ForceEnablePatches_16_0_0), Usb30ForceEnablePatches_16_0_0 }, /* 16.0.0 */
    { ParseModuleId("70D4C2ABCD049F16B301186924367F813DA70248"), util::size(Usb30ForceEnablePatches_17_0_0), Usb30ForceEnablePatches_17_0_0 }, /* 17.0.0 */
    { ParseModuleId("4F21AE15E814FA46515C0401BB23D4F7ADCBF3F4"), util::size(Usb30ForceEnablePatches_18_0_0), Usb30ForceEnablePatches_18_0_0 }, /* 18.0.0 */
    { ParseModuleId("54BB9BB32C958E02752DC5E4AE8D016BFE1F5418"), util::size(Usb30ForceEnablePatches_19_0_0), Usb30ForceEnablePatches_19_0_0 }, /* 19.0.0 */
    { ParseModuleId("40E80E7442C0DFC985315E6F9E8C77229818AC0F"), util::size(Usb30ForceEnablePatches_20_0_0), Usb30ForceEnablePatches_20_0_0 }, /* 20.0.0 */
};
