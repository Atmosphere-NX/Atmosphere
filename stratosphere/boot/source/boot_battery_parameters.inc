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

constexpr u8 Max17050Status           = 0x00;
constexpr u8 Max17050VAlrtThreshold   = 0x01;
constexpr u8 Max17050TAlrtThreshold   = 0x02;
constexpr u8 Max17050SocAlrtThreshold = 0x03;
constexpr u8 Max17050AtRate           = 0x04;
constexpr u8 Max17050RemCapRep        = 0x05;
constexpr u8 Max17050SocRep           = 0x06;
constexpr u8 Max17050Age              = 0x07;
constexpr u8 Max17050Temperature      = 0x08;
constexpr u8 Max17050VCell            = 0x09;
constexpr u8 Max17050Current          = 0x0A;
constexpr u8 Max17050AverageCurrent   = 0x0B;

constexpr u8 Max17050SocMix           = 0x0D;
constexpr u8 Max17050SocAv            = 0x0E;
constexpr u8 Max17050RemCapMix        = 0x0F;
constexpr u8 Max17050FullCap          = 0x10;
constexpr u8 Max17050Tte              = 0x11;
constexpr u8 Max17050QResidual00      = 0x12;
constexpr u8 Max17050FullSocThr       = 0x13;


constexpr u8 Max17050AverageTemp      = 0x16;
constexpr u8 Max17050Cycles           = 0x17;
constexpr u8 Max17050DesignCap        = 0x18;
constexpr u8 Max17050AverageVCell     = 0x19;
constexpr u8 Max17050MaxMinTemp       = 0x1A;
constexpr u8 Max17050MaxMinVoltage    = 0x1B;
constexpr u8 Max17050MaxMinCurrent    = 0x1C;
constexpr u8 Max17050Config           = 0x1D;
constexpr u8 Max17050IChgTerm         = 0x1E;
constexpr u8 Max17050RemCapAv         = 0x1F;

constexpr u8 Max17050Version          = 0x21;
constexpr u8 Max17050QResidual10      = 0x22;
constexpr u8 Max17050FullCapNom       = 0x23;
constexpr u8 Max17050TempNom          = 0x24;
constexpr u8 Max17050TempLim          = 0x25;

constexpr u8 Max17050Ain              = 0x27;
constexpr u8 Max17050LearnCfg         = 0x28;
constexpr u8 Max17050FilterCfg        = 0x29;
constexpr u8 Max17050RelaxCfg         = 0x2A;
constexpr u8 Max17050MiscCfg          = 0x2B;
constexpr u8 Max17050TGain            = 0x2C;
constexpr u8 Max17050TOff             = 0x2D;
constexpr u8 Max17050CGain            = 0x2E;
constexpr u8 Max17050COff             = 0x2F;


constexpr u8 Max17050QResidual20      = 0x32;



constexpr u8 Max17050IAvgEmpty        = 0x36;
constexpr u8 Max17050FCtc             = 0x37;
constexpr u8 Max17050RComp0           = 0x38;
constexpr u8 Max17050TempCo           = 0x39;
constexpr u8 Max17050VEmpty           = 0x3A;


constexpr u8 Max17050FStat            = 0x3D;
constexpr u8 Max17050Timer            = 0x3E;
constexpr u8 Max17050ShdnTimer        = 0x3F;


constexpr u8 Max17050QResidual30      = 0x42;


constexpr u8 Max17050DQAcc            = 0x45;
constexpr u8 Max17050DPAcc            = 0x46;

constexpr u8 Max17050SocVf0           = 0x48;

constexpr u8 Max17050Qh0              = 0x4C;
constexpr u8 Max17050Qh               = 0x4D;

constexpr u8 Max17050SocVfAccess      = 0x60;

constexpr u8 Max17050ModelAccess0     = 0x62;
constexpr u8 Max17050ModelAccess1     = 0x63;

constexpr u8 Max17050ModelChrTblStart = 0x80;
constexpr u8 Max17050ModelChrTblEnd   = 0xB0;


constexpr u8 Max17050VFocV            = 0xFB;
constexpr u8 Max17050SocVf            = 0xFF;

constexpr size_t Max17050ModelChrTblSize = Max17050ModelChrTblEnd - Max17050ModelChrTblStart;

struct Max17050Parameters {
    u16 relaxcfg;
    u16 rcomp0;
    u16 tempco;
    u16 ichgterm;
    u16 tgain;
    u16 toff;
    u16 vempty;
    u16 qresidual00;
    u16 qresidual10;
    u16 qresidual20;
    u16 qresidual30;
    u16 fullcap;
    u16 vffullcap;
    u16 modeltbl[Max17050ModelChrTblSize];
    u16 fullsocthr;
    u16 iavgempty;
};

static_assert(sizeof(Max17050Parameters) == 0x7E, "Max17050Parameters definition!");

constexpr Max17050Parameters Max17050ParamsA = {
    0x203B, /* relaxcfg */
    0x0053, /* rcomp0 */
    0x1C22, /* tempco */
    0x0333, /* ichgterm */
    0xE1F6, /* tgain */
    0x2BF2, /* toff */
    0xA05F, /* vempty */
    0x5786, /* qresidual00 */
    0x3184, /* qresidual10 */
    0x1E00, /* qresidual20 */
    0x1602, /* qresidual30 */
    0x2476, /* fullcap */
    0x2476, /* vffullcap */
    { /* modeltbl */
        0x9FF0, 0xAD30, 0xB5D0, 0xB9C0, 0xBAD0, 0xBBE0, 0xBC30, 0xBC90,
        0xBCE0, 0xBD40, 0xBE70, 0xC0E0, 0xC4E0, 0xC890, 0xCC90, 0xD0F0,
        0x0170, 0x0480, 0x0590, 0x0BE0, 0x0A00, 0x3C00, 0x3810, 0x3A00,
        0x3A30, 0x19F0, 0x0EF0, 0x0AF0, 0x0BD0, 0x07F0, 0x06F0, 0x06F0,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
    },
    0x5F00, /* fullsocthr */
    0x1D2A  /* iavgempty */
};

constexpr Max17050Parameters Max17050ParamsM = {
    0x203B, /* relaxcfg */
    0x0085, /* rcomp0 */
    0x1625, /* tempco */
    0x0333, /* ichgterm */
    0xE1F6, /* tgain */
    0x2BF2, /* toff */
    0xA05F, /* vempty */
    0x3100, /* qresidual00 */
    0x1B00, /* qresidual10 */
    0x1000, /* qresidual20 */
    0x0C81, /* qresidual30 */
    0x227A, /* fullcap */
    0x227A, /* vffullcap */
    { /* modeltbl */
        0xA340, 0xB840, 0xB900, 0xBB70, 0xBC90, 0xBD20, 0xBDC0, 0xBEA0,
        0xBF70, 0xC030, 0xC210, 0xC3F0, 0xC800, 0xC9E0, 0xCCA0, 0xD090,
        0x0160, 0x3800, 0x0800, 0x1E00, 0x2550, 0x3060, 0x15D0, 0x1810,
        0x1490, 0x0B80, 0x0BF0, 0x0AF0, 0x0CB0, 0x06F0, 0x09D0, 0x09D0,
        0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
        0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100, 0x0100,
    },
    0x5F00, /* fullsocthr */
    0x1D2A  /* iavgempty */
};

constexpr Max17050Parameters Max17050ParamsR = {
    0x203B, /* relaxcfg */
    0x0048, /* rcomp0 */
    0x2034, /* tempco */
    0x0333, /* ichgterm */
    0xE1F6, /* tgain */
    0x2BF2, /* toff */
    0xA05F, /* vempty */
    0x5A00, /* qresidual00 */
    0x3B00, /* qresidual10 */
    0x0F80, /* qresidual20 */
    0x0B02, /* qresidual30 */
    0x2466, /* fullcap */
    0x2466, /* vffullcap */
    { /* modeltbl */
        0x9C50, 0xAD90, 0xB270, 0xB6A0, 0xB8F0, 0xBB10, 0xBC00, 0xBD00,
        0xBD70, 0xBE70, 0xBF50, 0xC1F0, 0xC380, 0xC590, 0xC8E0, 0xD0B0,
        0x00D0, 0x0150, 0x0300, 0x0D00, 0x0E00, 0x1900, 0x2AC0, 0x2830,
        0x1760, 0x18F0, 0x0DF0, 0x0BC0, 0x0DF0, 0x0BF0, 0x06F0, 0x06F0,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
    },
    0x5F00, /* fullsocthr */
    0x1D2A  /* iavgempty */
};

constexpr Max17050Parameters Max17050Params1 = {
    0x203B, /* relaxcfg */
    0x0040, /* rcomp0 */
    0x1624, /* tempco */
    0x0333, /* ichgterm */
    0xE1F6, /* tgain */
    0x2BF2, /* toff */
    0xA05F, /* vempty */
    0x4690, /* qresidual00 */
    0x2605, /* qresidual10 */
    0x1605, /* qresidual20 */
    0x0F05, /* qresidual30 */
    0x1AE4, /* fullcap */
    0x1AE4, /* vffullcap */
    { /* modeltbl */
        0x8B50, 0x9C20, 0xACF0, 0xB160, 0xB3A0, 0xB5B0, 0xB950, 0xBBE0,
        0xBDC0, 0xBEF0, 0xC140, 0xC250, 0xC600, 0xC960, 0xCCE0, 0xD060,
        0x0070, 0x00F0, 0x0440, 0x0400, 0x0500, 0x0400, 0x0D00, 0x3270,
        0x0FB0, 0x0AF0, 0x10F0, 0x0CE0, 0x09E0, 0x07F0, 0x06F0, 0x06F0,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
    },
    0x5F00, /* fullsocthr */
    0x1584  /* iavgempty */
};

constexpr Max17050Parameters Max17050Params2 = {
    0x203B, /* relaxcfg */
    0x004A, /* rcomp0 */
    0x1D23, /* tempco */
    0x0333, /* ichgterm */
    0xE1F6, /* tgain */
    0x2BF2, /* toff */
    0xA05F, /* vempty */
    0x4000, /* qresidual00 */
    0x1E80, /* qresidual10 */
    0x0D83, /* qresidual20 */
    0x0783, /* qresidual30 */
    0x1C20, /* fullcap */
    0x1C20, /* vffullcap */
    { /* modeltbl */
        0x8040, 0x9A30, 0xB430, 0xB770, 0xBAB0, 0xBBC0, 0xBD00, 0xBE50,
        0xBF70, 0xC0D0, 0xC300, 0xC590, 0xC960, 0xCD40, 0xD1F0, 0xD5C0,
        0x0040, 0x0060, 0x0510, 0x0D30, 0x16C0, 0x2160, 0x1380, 0x1A10,
        0x0EC0, 0x0CE0, 0x08F0, 0x0940, 0x0920, 0x06F0, 0x06C0, 0x06C0,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
    },
    0x5500, /* fullsocthr */
    0x1680  /* iavgempty */
};

constexpr Max17050Parameters Max17050Params2M = {
    0x203B, /* relaxcfg */
    0x0049, /* rcomp0 */
    0x222A, /* tempco */
    0x0333, /* ichgterm */
    0xE1F6, /* tgain */
    0x2BF2, /* toff */
    0xA05F, /* vempty */
    0x4F00, /* qresidual00 */
    0x2680, /* qresidual10 */
    0x1205, /* qresidual20 */
    0x0C87, /* qresidual30 */
    0x1C68, /* fullcap */
    0x1C68, /* vffullcap */
    { /* modeltbl */
        0x8E40, 0xB570, 0xB8F0, 0xBB00, 0xBC20, 0xBCC0, 0xBE30, 0xBFE0,
        0xC200, 0xC400, 0xC720, 0xCB50, 0xCF00, 0xD100, 0xD480, 0xD5C0,
        0x00C0, 0x0C00, 0x0A10, 0x1800, 0x2C00, 0x1C10, 0x12D0, 0x09F0,
        0x0AF0, 0x0850, 0x09F0, 0x06F0, 0x06B0, 0x07E0, 0x01D0, 0x01D0,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
        0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200,
    },
    0x5500, /* fullsocthr */
    0x16B9  /* iavgempty */
};
