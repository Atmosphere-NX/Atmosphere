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
constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_1_0_0[] = {
    { 0x304C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_2_0_0[] = {
    { 0x33C4, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_2_1_0[] = {
    { 0x3300, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_3_0_0[] = {
    { 0x36B4, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_3_0_1[] = {
    { 0x36B4, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_4_0_0[] = {
    { 0x3698, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_5_0_0[] = {
    { 0x2075C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_5_1_0[] = {
    { 0x207EC, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_6_0_0[] = {
    { 0x74B2C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_6_1_0[] = {
    { 0x74B6C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_7_0_0[] = {
    { 0x76C4C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_8_0_0[] = {
    { 0x7854C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_9_0_0[] = {
    { 0xE1EC, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_9_1_0[] = {
    { 0xE1EC, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_10_0_0[] = {
    { 0xF67C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_11_0_0[] = {
    { 0xF32C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_12_0_0[] = {
    { 0x106C8, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_12_1_0[] = {
    { 0x106C8, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_13_0_0[] = {
    { 0x10740, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_13_1_0[] = {
    { 0x10740, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_14_0_0[] = {
    { 0x44238, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_15_0_0[] = {
    { 0x79E60, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_16_0_0[] = {
    { 0x7A80C, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_17_0_0[] = {
    { 0x809E0, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_18_0_0[] = {
    { 0x83560, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_18_0_1[] = {
    { 0x83560, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_19_0_0[] = {
    { 0x86420, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_20_0_0[] = {
    { 0x879C0, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatchEntry ForceCommunicationEnabledPatches_20_1_0[] = {
    { 0x87AC0, "\x00\x30\x9A\xD2\x00\x1E\xA1\xF2\x61\x01\x00\xD4\xE0\x03\x1F\xAA\xC0\x03\x5F\xD6", 20 },
};

constexpr inline const EmbeddedPatch ForceCommunicationEnabledPatches[] = {
    { ParseModuleId("D47D1506009B340829CD545B2A3F3AA7881FBADA"), util::size(ForceCommunicationEnabledPatches_1_0_0), ForceCommunicationEnabledPatches_1_0_0 }, /* 1.0.0 */
    { ParseModuleId("E6BFDADD5C69E17D43B7C67E2B2EE8B2E50C8E1F"), util::size(ForceCommunicationEnabledPatches_2_0_0), ForceCommunicationEnabledPatches_2_0_0 }, /* 2.0.0 */
    { ParseModuleId("AD32FB6D8F36668C586E538E32576A8D6A3931C0"), util::size(ForceCommunicationEnabledPatches_2_1_0), ForceCommunicationEnabledPatches_2_1_0 }, /* 2.1.0-2.3.0 */
    { ParseModuleId("881206E6B5078EFC4E2C30D7B33E33AD266538C6"), util::size(ForceCommunicationEnabledPatches_3_0_0), ForceCommunicationEnabledPatches_3_0_0 }, /* 3.0.0 */
    { ParseModuleId("5835E2DADB4DD570DD811ABF521FA91AC3C7B717"), util::size(ForceCommunicationEnabledPatches_3_0_1), ForceCommunicationEnabledPatches_3_0_1 }, /* 3.0.1-3.0.2 */
    { ParseModuleId("38774C42DFCB8B9D7AA61550D6AF7D335472556C"), util::size(ForceCommunicationEnabledPatches_4_0_0), ForceCommunicationEnabledPatches_4_0_0 }, /* 4.0.0-4.1.0 */
    { ParseModuleId("B6966381A806655B718F1BF11DB5FF836E3085F7"), util::size(ForceCommunicationEnabledPatches_5_0_0), ForceCommunicationEnabledPatches_5_0_0 }, /* 5.0.0-5.0.2 */
    { ParseModuleId("D82361E0D66DC01AFA3B5116532E5E1ED569C578"), util::size(ForceCommunicationEnabledPatches_5_1_0), ForceCommunicationEnabledPatches_5_1_0 }, /* 5.1.0 */
    { ParseModuleId("3AED90979B380C6415F975F5B784BEA2B4730E8C"), util::size(ForceCommunicationEnabledPatches_6_0_0), ForceCommunicationEnabledPatches_6_0_0 }, /* 6.0.0-6.0.1 */
    { ParseModuleId("43F10952AE80CFADC39A0BF59EA4E552EF4A4528"), util::size(ForceCommunicationEnabledPatches_6_1_0), ForceCommunicationEnabledPatches_6_1_0 }, /* 6.1.0-6.2.0 */
    { ParseModuleId("929014BFCFE462FD76B2BB3454FB304F63C73AC2"), util::size(ForceCommunicationEnabledPatches_7_0_0), ForceCommunicationEnabledPatches_7_0_0 }, /* 7.0.0-7.0.1 */
    { ParseModuleId("FD53CB863709DFFEC19C0889F61D4C424AFFD4ED"), util::size(ForceCommunicationEnabledPatches_8_0_0), ForceCommunicationEnabledPatches_8_0_0 }, /* 8.0.0-8.1.0 */
    { ParseModuleId("7DF07326B6B50CA37F19C1C44F9458406C536B30"), util::size(ForceCommunicationEnabledPatches_9_0_0), ForceCommunicationEnabledPatches_9_0_0 }, /* 9.0.0-9.0.1 */
    { ParseModuleId("9C72C47F0310F7C9F487047D8EB42DBB96882088"), util::size(ForceCommunicationEnabledPatches_9_1_0), ForceCommunicationEnabledPatches_9_1_0 }, /* 9.1.0-9.2.0 */
    { ParseModuleId("5DA461C7B6CAE6B88EDF4F914F7CBCF0943B10BB"), util::size(ForceCommunicationEnabledPatches_10_0_0), ForceCommunicationEnabledPatches_10_0_0 }, /* 10.0.0-10.2.0 */
    { ParseModuleId("7A43F840337C28D453718843608EEFF78AFD460B"), util::size(ForceCommunicationEnabledPatches_11_0_0), ForceCommunicationEnabledPatches_11_0_0 }, /* 11.0.0-11.0.1 */
    { ParseModuleId("A85F50FBA10E06A3EBA3D3FACB9E075B218C7D6D"), util::size(ForceCommunicationEnabledPatches_12_0_0), ForceCommunicationEnabledPatches_12_0_0 }, /* 12.0.0-12.0.3 */
    { ParseModuleId("69E25CDEEED5C6520AC2AC8E5EAE01CD8FC46E40"), util::size(ForceCommunicationEnabledPatches_12_1_0), ForceCommunicationEnabledPatches_12_1_0 }, /* 12.1.0 */
    { ParseModuleId("8CB532EA199207191F04CE3DDECEC854C7CF07D6"), util::size(ForceCommunicationEnabledPatches_13_0_0), ForceCommunicationEnabledPatches_13_0_0 }, /* 13.0.0 */
    { ParseModuleId("6F02D68B1DCD2AFFBAED14B5933F81F3C327E537"), util::size(ForceCommunicationEnabledPatches_13_1_0), ForceCommunicationEnabledPatches_13_1_0 }, /* 13.1.0-13.2.1 */
    { ParseModuleId("BA91B5A61E423F51FB83B2C9E6C153CC5AE27DCB"), util::size(ForceCommunicationEnabledPatches_14_0_0), ForceCommunicationEnabledPatches_14_0_0 }, /* 14.0.0-14.1.2 */
    { ParseModuleId("A188828ADF447425D97901462EBF732D2E29BC4E"), util::size(ForceCommunicationEnabledPatches_15_0_0), ForceCommunicationEnabledPatches_15_0_0 }, /* 15.0.0-15.0.1 */
    { ParseModuleId("A2C3AEE4E5A954908BEBAEDEA2010095E9E521B2"), util::size(ForceCommunicationEnabledPatches_16_0_0), ForceCommunicationEnabledPatches_16_0_0 }, /* 16.0.0-16.1.0 */
    { ParseModuleId("440F71259BCA1E97EA3663CBD6EC9315951F9E96"), util::size(ForceCommunicationEnabledPatches_17_0_0), ForceCommunicationEnabledPatches_17_0_0 }, /* 17.0.0-17.0.1 */
    { ParseModuleId("184A3F5734F456D0718FA35D15D8410A9BDFC537"), util::size(ForceCommunicationEnabledPatches_18_0_0), ForceCommunicationEnabledPatches_18_0_0 }, /* 18.0.0 */
    { ParseModuleId("0C4C2220D90155B139E254179D2D35244B32C9C1"), util::size(ForceCommunicationEnabledPatches_18_0_1), ForceCommunicationEnabledPatches_18_0_1 }, /* 18.0.1-18.1.0 */
    { ParseModuleId("1BD31ABF1E3E607CA79B29CF7D7B10758FF93FA0"), util::size(ForceCommunicationEnabledPatches_19_0_0), ForceCommunicationEnabledPatches_19_0_0 }, /* 19.0.0-19.0.1 */
    { ParseModuleId("2C3E24F46D131685B361FEE1F0570B985095E034"), util::size(ForceCommunicationEnabledPatches_20_0_0), ForceCommunicationEnabledPatches_20_0_0 }, /* 20.0.0-20.0.1 */
    { ParseModuleId("523D2C6E08D9BE4B512AD20AA5628B7BB06D73C2"), util::size(ForceCommunicationEnabledPatches_20_1_0), ForceCommunicationEnabledPatches_20_1_0 }, /* 20.1.0 */
};