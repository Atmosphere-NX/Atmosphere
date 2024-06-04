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
constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_1_0_0[] = {
    { 0x4884, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_2_3_0[] = {
    { 0x4758, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_3_0_0[] = {
    { 0x4710, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_3_0_2[] = {
    { 0x4710, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_4_0_0[] = {
    { 0x553C, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_5_0_0[] = {
    { 0x5E94, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_5_1_0[] = {
    { 0x5E94, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_6_0_0[] = {
    { 0x4BCD4, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_6_1_0[] = {
    { 0x4BCD4, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_7_0_0[] = {
    { 0x4D5F4, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_8_0_1[] = {
    { 0x4EDC4, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_8_1_0[] = {
    { 0x4EDC4, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_9_0_0[] = {
    { 0x2D2D4, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_10_0_0[] = {
    { 0x2DB98, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_10_1_0[] = {
    { 0x2DBAC, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_11_0_0[] = {
    { 0x2CFCC, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_12_0_0[] = {
    { 0x2E47C, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_12_1_0[] = {
    { 0x2E51C, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_13_0_0[] = {
    { 0x31428, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_13_1_0[] = {
    { 0x31928, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_14_0_0[] = {
    { 0x662D8, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_15_0_0[] = {
    { 0x649EC, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_16_0_0[] = {
    { 0x64A6C, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_17_0_0[] = {
    { 0x6720C, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatchEntry DisableTicketVerificationPatches_18_0_0[] = {
    { 0x6964C, "\xE0\x03\x1F\xAA", 4 },
};

constexpr inline const EmbeddedPatch DisableTicketVerificationPatches[] = {
    { ParseModuleId("F7271147BA178C4E9F6451C62B10E668E6F8DECC"), util::size(DisableTicketVerificationPatches_1_0_0), DisableTicketVerificationPatches_1_0_0 }, /* 1.0.0 */
    { ParseModuleId("76935CA6235CA150D790D4DDF09CA3488AEF50D8"), util::size(DisableTicketVerificationPatches_2_3_0), DisableTicketVerificationPatches_2_3_0 }, /* 2.3.0 */
    { ParseModuleId("F92CFF283777465AFBCC6E160C10EFE7AADC4D3C"), util::size(DisableTicketVerificationPatches_3_0_0), DisableTicketVerificationPatches_3_0_0 }, /* 3.0.0 */
    { ParseModuleId("F86D615F9FC6F0D16C3458BE58C64594DBDAB0DF"), util::size(DisableTicketVerificationPatches_3_0_2), DisableTicketVerificationPatches_3_0_2 }, /* 3.0.2 */
    { ParseModuleId("7135C20E752183914606CCE81293F24B738C3AEF"), util::size(DisableTicketVerificationPatches_4_0_0), DisableTicketVerificationPatches_4_0_0 }, /* 4.0.0-4.1.0 */
    { ParseModuleId("A6E7D98BD335371E1B9A96F6241EA3E83B0C36F4"), util::size(DisableTicketVerificationPatches_5_0_0), DisableTicketVerificationPatches_5_0_0 }, /* 5.0.0-5.0.2 */
    { ParseModuleId("C0BFC6518707AC975295A0B656A051DA885A9045"), util::size(DisableTicketVerificationPatches_5_1_0), DisableTicketVerificationPatches_5_1_0 }, /* 5.1.0 */
    { ParseModuleId("CBF2D40A2210C4EE1137F80098DB776EAC7DDCC1"), util::size(DisableTicketVerificationPatches_6_0_0), DisableTicketVerificationPatches_6_0_0 }, /* 6.0.0-6.0.1 */
    { ParseModuleId("9C1B205852731D054B070529D320AD9649E7796C"), util::size(DisableTicketVerificationPatches_6_1_0), DisableTicketVerificationPatches_6_1_0 }, /* 6.1.0-6.2.0 */
    { ParseModuleId("D2D2430244D162C9FAABE8C89A58C6E3962160F1"), util::size(DisableTicketVerificationPatches_7_0_0), DisableTicketVerificationPatches_7_0_0 }, /* 7.0.0-7.0.1 */
    { ParseModuleId("39E7884F7DDEA9BE4FF9F7187898FACC466A080A"), util::size(DisableTicketVerificationPatches_8_0_1), DisableTicketVerificationPatches_8_0_1 }, /* 8.0.1 */
    { ParseModuleId("CF8FB4F82D3E2D56720E1EDB14AB06543343E042"), util::size(DisableTicketVerificationPatches_8_1_0), DisableTicketVerificationPatches_8_1_0 }, /* 8.1.0-8.1.1 */
    { ParseModuleId("34E95FF2929B5B767DCFD41D108AD927D2D7F5DC"), util::size(DisableTicketVerificationPatches_9_0_0), DisableTicketVerificationPatches_9_0_0 }, /* 9.0.0-9.2.0 */
    { ParseModuleId("03E4EB5556B98B327D1353E8AA2C7ADF2C544470"), util::size(DisableTicketVerificationPatches_10_0_0), DisableTicketVerificationPatches_10_0_0 }, /* 10.0.0-10.0.4 */
    { ParseModuleId("5AA09E1AF740A91D0F73ADFAE81A63E8AC0610D2"), util::size(DisableTicketVerificationPatches_10_1_0), DisableTicketVerificationPatches_10_1_0 }, /* 10.1.0-10.2.0 */
    { ParseModuleId("3B8BF56DBEC7225D2EE666B009C42C0DC4552010"), util::size(DisableTicketVerificationPatches_11_0_0), DisableTicketVerificationPatches_11_0_0 }, /* 11.0.0-11.0.1 */
    { ParseModuleId("F48ABC3EF41A0555800127D172C9B8D2C653243A"), util::size(DisableTicketVerificationPatches_12_0_0), DisableTicketVerificationPatches_12_0_0 }, /* 12.0.0-12.0.3 */
    { ParseModuleId("1114E9102F1EBCD1B0EAF19C927362CFCB8B5D2C"), util::size(DisableTicketVerificationPatches_12_1_0), DisableTicketVerificationPatches_12_1_0 }, /* 12.1.0 */
    { ParseModuleId("D0ECC9483E636AE19FE3E314DB41CB18019977D7"), util::size(DisableTicketVerificationPatches_13_0_0), DisableTicketVerificationPatches_13_0_0 }, /* 13.0.0 */
    { ParseModuleId("98BB737D9780E9FE3117981A77A09FF6756DBC2E"), util::size(DisableTicketVerificationPatches_13_1_0), DisableTicketVerificationPatches_13_1_0 }, /* 13.1.0-13.2.1 */
    { ParseModuleId("621351EB04199CF0B7A899896C451F2FD1B7A1CE"), util::size(DisableTicketVerificationPatches_14_0_0), DisableTicketVerificationPatches_14_0_0 }, /* 14.0.0-14.1.2 */
    { ParseModuleId("4FB9AFAED8A9093A3D88F6D61158B1C165F9444A"), util::size(DisableTicketVerificationPatches_15_0_0), DisableTicketVerificationPatches_15_0_0 }, /* 15.0.0-15.0.1 */
    { ParseModuleId("127904316C924326084288641825FE5DC259AF6B"), util::size(DisableTicketVerificationPatches_16_0_0), DisableTicketVerificationPatches_16_0_0 }, /* 16.0.0-16.1.0 */
    { ParseModuleId("02DCD930D0A7462451E9973AB2022D697C77614B"), util::size(DisableTicketVerificationPatches_17_0_0), DisableTicketVerificationPatches_17_0_0 }, /* 17.0.0-17.0.1 */
    { ParseModuleId("897C2CF2777B3A8845640C096933ACE61DD12272"), util::size(DisableTicketVerificationPatches_18_0_0), DisableTicketVerificationPatches_18_0_0 }, /* 18.0.0 */
};
