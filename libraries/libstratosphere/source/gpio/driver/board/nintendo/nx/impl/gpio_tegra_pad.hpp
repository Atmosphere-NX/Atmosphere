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
#pragma once
#include <stratosphere.hpp>

namespace ams::gpio::driver::board::nintendo_nx::impl {

    enum GpioPadPort {
        GpioPadPort_A     =  0,
        GpioPadPort_B     =  1,
        GpioPadPort_C     =  2,
        GpioPadPort_D     =  3,
        GpioPadPort_E     =  4,
        GpioPadPort_F     =  5,
        GpioPadPort_G     =  6,
        GpioPadPort_H     =  7,
        GpioPadPort_I     =  8,
        GpioPadPort_J     =  9,
        GpioPadPort_K     = 10,
        GpioPadPort_L     = 11,
        GpioPadPort_M     = 12,
        GpioPadPort_N     = 13,
        GpioPadPort_O     = 14,
        GpioPadPort_P     = 15,
        GpioPadPort_Q     = 16,
        GpioPadPort_R     = 17,
        GpioPadPort_S     = 18,
        GpioPadPort_T     = 19,
        GpioPadPort_U     = 20,
        GpioPadPort_V     = 21,
        GpioPadPort_W     = 22,
        GpioPadPort_X     = 23,
        GpioPadPort_Y     = 24,
        GpioPadPort_Z     = 25,
        GpioPadPort_AA    = 26,
        GpioPadPort_BB    = 27,
        GpioPadPort_CC    = 28,
        GpioPadPort_DD    = 29,
        GpioPadPort_EE    = 30,
        GpioPadPort_FF    = 31,
        GpioPadPort_Count = 32,
    };

    using InternalGpioPadNumber = int;

    constexpr unsigned int GetInternalGpioPadNumber(GpioPadPort port, unsigned int which) {
        AMS_ASSERT(which < 8);

        return (static_cast<unsigned int>(port) * 8) + which;
    }

    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_None      = -1;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_0  = 0x00;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_1  = 0x01;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_2  = 0x02;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_3  = 0x03;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_4  = 0x04;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_5  = 0x05;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_6  = 0x06;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_A_7  = 0x07;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_0  = 0x08;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_1  = 0x09;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_2  = 0x0A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_3  = 0x0B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_4  = 0x0C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_5  = 0x0D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_6  = 0x0E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_B_7  = 0x0F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_0  = 0x10;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_1  = 0x11;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_2  = 0x12;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_3  = 0x13;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_4  = 0x14;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_5  = 0x15;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_6  = 0x16;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_C_7  = 0x17;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_0  = 0x18;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_1  = 0x19;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_2  = 0x1A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_3  = 0x1B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_4  = 0x1C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_5  = 0x1D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_6  = 0x1E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_D_7  = 0x1F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_0  = 0x20;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_1  = 0x21;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_2  = 0x22;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_3  = 0x23;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_4  = 0x24;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_5  = 0x25;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_6  = 0x26;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_E_7  = 0x27;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_0  = 0x28;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_1  = 0x29;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_2  = 0x2A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_3  = 0x2B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_4  = 0x2C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_5  = 0x2D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_6  = 0x2E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_F_7  = 0x2F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_0  = 0x30;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_1  = 0x31;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_2  = 0x32;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_3  = 0x33;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_4  = 0x34;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_5  = 0x35;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_6  = 0x36;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_G_7  = 0x37;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_0  = 0x38;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_1  = 0x39;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_2  = 0x3A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_3  = 0x3B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_4  = 0x3C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_5  = 0x3D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_6  = 0x3E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_H_7  = 0x3F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_0  = 0x40;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_1  = 0x41;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_2  = 0x42;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_3  = 0x43;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_4  = 0x44;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_5  = 0x45;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_6  = 0x46;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_I_7  = 0x47;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_0  = 0x48;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_1  = 0x49;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_2  = 0x4A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_3  = 0x4B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_4  = 0x4C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_5  = 0x4D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_6  = 0x4E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_J_7  = 0x4F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_0  = 0x50;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_1  = 0x51;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_2  = 0x52;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_3  = 0x53;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_4  = 0x54;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_5  = 0x55;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_6  = 0x56;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_K_7  = 0x57;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_0  = 0x58;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_1  = 0x59;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_2  = 0x5A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_3  = 0x5B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_4  = 0x5C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_5  = 0x5D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_6  = 0x5E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_L_7  = 0x5F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_0  = 0x60;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_1  = 0x61;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_2  = 0x62;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_3  = 0x63;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_4  = 0x64;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_5  = 0x65;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_6  = 0x66;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_M_7  = 0x67;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_0  = 0x68;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_1  = 0x69;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_2  = 0x6A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_3  = 0x6B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_4  = 0x6C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_5  = 0x6D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_6  = 0x6E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_N_7  = 0x6F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_0  = 0x70;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_1  = 0x71;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_2  = 0x72;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_3  = 0x73;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_4  = 0x74;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_5  = 0x75;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_6  = 0x76;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_O_7  = 0x77;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_0  = 0x78;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_1  = 0x79;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_2  = 0x7A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_3  = 0x7B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_4  = 0x7C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_5  = 0x7D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_6  = 0x7E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_P_7  = 0x7F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_0  = 0x80;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_1  = 0x81;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_2  = 0x82;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_3  = 0x83;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_4  = 0x84;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_5  = 0x85;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_6  = 0x86;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Q_7  = 0x87;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_0  = 0x88;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_1  = 0x89;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_2  = 0x8A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_3  = 0x8B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_4  = 0x8C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_5  = 0x8D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_6  = 0x8E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_R_7  = 0x8F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_0  = 0x90;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_1  = 0x91;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_2  = 0x92;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_3  = 0x93;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_4  = 0x94;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_5  = 0x95;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_6  = 0x96;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_S_7  = 0x97;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_0  = 0x98;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_1  = 0x99;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_2  = 0x9A;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_3  = 0x9B;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_4  = 0x9C;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_5  = 0x9D;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_6  = 0x9E;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_T_7  = 0x9F;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_0  = 0xA0;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_1  = 0xA1;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_2  = 0xA2;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_3  = 0xA3;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_4  = 0xA4;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_5  = 0xA5;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_6  = 0xA6;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_U_7  = 0xA7;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_0  = 0xA8;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_1  = 0xA9;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_2  = 0xAA;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_3  = 0xAB;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_4  = 0xAC;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_5  = 0xAD;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_6  = 0xAE;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_V_7  = 0xAF;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_0  = 0xB0;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_1  = 0xB1;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_2  = 0xB2;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_3  = 0xB3;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_4  = 0xB4;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_5  = 0xB5;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_6  = 0xB6;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_W_7  = 0xB7;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_0  = 0xB8;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_1  = 0xB9;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_2  = 0xBA;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_3  = 0xBB;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_4  = 0xBC;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_5  = 0xBD;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_6  = 0xBE;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_X_7  = 0xBF;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_0  = 0xC0;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_1  = 0xC1;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_2  = 0xC2;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_3  = 0xC3;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_4  = 0xC4;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_5  = 0xC5;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_6  = 0xC6;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Y_7  = 0xC7;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_0  = 0xC8;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_1  = 0xC9;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_2  = 0xCA;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_3  = 0xCB;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_4  = 0xCC;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_5  = 0xCD;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_6  = 0xCE;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_Z_7  = 0xCF;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_0 = 0xD0;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_1 = 0xD1;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_2 = 0xD2;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_3 = 0xD3;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_4 = 0xD4;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_5 = 0xD5;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_6 = 0xD6;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_AA_7 = 0xD7;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_0 = 0xD8;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_1 = 0xD9;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_2 = 0xDA;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_3 = 0xDB;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_4 = 0xDC;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_5 = 0xDD;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_6 = 0xDE;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_BB_7 = 0xDF;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_0 = 0xE0;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_1 = 0xE1;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_2 = 0xE2;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_3 = 0xE3;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_4 = 0xE4;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_5 = 0xE5;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_6 = 0xE6;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_CC_7 = 0xE7;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_0 = 0xE8;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_1 = 0xE9;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_2 = 0xEA;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_3 = 0xEB;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_4 = 0xEC;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_5 = 0xED;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_6 = 0xEE;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_DD_7 = 0xEF;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_0 = 0xF0;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_1 = 0xF1;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_2 = 0xF2;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_3 = 0xF3;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_4 = 0xF4;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_5 = 0xF5;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_6 = 0xF6;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_EE_7 = 0xF7;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_0 = 0xF8;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_1 = 0xF9;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_2 = 0xFA;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_3 = 0xFB;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_4 = 0xFC;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_5 = 0xFD;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_6 = 0xFE;
    constexpr inline InternalGpioPadNumber InternalGpioPadNumber_Port_FF_7 = 0xFF;

    struct PadMapCombination {
        DeviceCode device_code;
        InternalGpioPadNumber internal_number;
        wec::WakeEvent wake_event;
    };

    #include "gpio_internal_pad_map_combination.inc"

    struct PadInfo {
        wec::WakeEvent wake_event;

        constexpr PadInfo() : wake_event(wec::WakeEvent_None) { /* ... */ }

        constexpr explicit PadInfo(wec::WakeEvent we) : wake_event(we) { /* ... */ }

        constexpr bool operator ==(const PadInfo &rhs) const { return this->wake_event == rhs.wake_event; }
        constexpr bool operator !=(const PadInfo &rhs) const { return !(*this == rhs); }
    };

    struct PadStatus {
        bool is_wake_active;
        bool is_wake_active_debug;

        constexpr PadStatus() : is_wake_active(false), is_wake_active_debug(false) { /* ... */ }
    };

    class TegraPad : public ::ams::gpio::driver::Pad {
        AMS_DDSF_CASTABLE_TRAITS(ams::gpio::driver::board::nintendo_nx::impl::TegraPad, ::ams::gpio::driver::Pad);
        private:
            using Base = ::ams::gpio::driver::Pad;
        private:
            util::IntrusiveListNode interrupt_list_node;
            PadInfo info;
            PadStatus status;
        public:
            using InterruptListTraits = util::IntrusiveListMemberTraitsDeferredAssert<&TegraPad::interrupt_list_node>;
            using InterruptList       = typename InterruptListTraits::ListType;
            friend class util::IntrusiveList<TegraPad, util::IntrusiveListMemberTraitsDeferredAssert<&TegraPad::interrupt_list_node>>;
        public:
            TegraPad() : Pad(), interrupt_list_node(), info(), status() { /* ... */ }

            const PadInfo &GetInfo() const { return this->info; }
            PadStatus &GetStatus() { return this->status; }

            void SetParameters(int pad, const PadInfo &i) {
                Base::SetPadNumber(pad);
                this->info = info;
            }

            bool IsLinkedToInterruptBoundPadList() const {
                return this->interrupt_list_node.IsLinked();
            }
    };

}
