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

namespace test {

    constexpr inline auto Other = GetSecureScratchMasks<SecureRegister_Other>();
    static_assert(Other[0] == 0x00700FF0u);
    static_assert(Other[1] == 0xFC000000u);
    static_assert(Other[2] == 0x3F0FFF00u);
    static_assert(Other[3] == 0x00000000u);
    static_assert(Other[4] == 0x00000000u);
    static_assert(Other[5] == 0x0C000000u);
    static_assert(Other[6] == 0x00000000u);
    static_assert(Other[7] == 0xFF00FF00u);

    constexpr inline auto DramParameters = GetSecureScratchMasks<SecureRegister_DramParameters>();
    static_assert(DramParameters[0] == 0x00000000u);
    static_assert(DramParameters[1] == 0x03FCFFFFu);
    static_assert(DramParameters[2] == 0x00000000u);
    static_assert(DramParameters[3] == 0x3F3FFFFFu);
    static_assert(DramParameters[4] == 0xFFFFFFFFu);
    static_assert(DramParameters[5] == 0xF3FFC00Fu);
    static_assert(DramParameters[6] == 0x003FFFFFu);
    static_assert(DramParameters[7] == 0x000000FFu);

    constexpr inline auto ResetVector = GetSecureScratchMasks<SecureRegister_ResetVector>();
    static_assert(ResetVector[0] == 0x00000000u);
    static_assert(ResetVector[1] == 0x00000000u);
    static_assert(ResetVector[2] == 0x00F00000u);
    static_assert(ResetVector[3] == 0x00000000u);
    static_assert(ResetVector[4] == 0x00000000u);
    static_assert(ResetVector[5] == 0x00000000u);
    static_assert(ResetVector[6] == 0x00000000u);
    static_assert(ResetVector[7] == 0x00000000u);

    constexpr inline auto CmacWrite = GetSecureScratchMasks<SecureRegister_CmacWrite>();
    static_assert(CmacWrite[0] == 0x00000000u);
    static_assert(CmacWrite[1] == 0x00000000u);
    static_assert(CmacWrite[2] == 0x00000000u);
    static_assert(CmacWrite[3] == 0x00000000u);
    static_assert(CmacWrite[4] == 0x00000000u);
    static_assert(CmacWrite[5] == 0x00000000u);
    static_assert(CmacWrite[6] == 0x00000000u);
    static_assert(CmacWrite[7] == 0x00550000u);

    constexpr inline auto CmacRead = GetSecureScratchMasks<SecureRegister_CmacRead>();
    static_assert(CmacRead[0] == 0x00000000u);
    static_assert(CmacRead[1] == 0x00000000u);
    static_assert(CmacRead[2] == 0x00000000u);
    static_assert(CmacRead[3] == 0x00000000u);
    static_assert(CmacRead[4] == 0x00000000u);
    static_assert(CmacRead[5] == 0x00000000u);
    static_assert(CmacRead[6] == 0x00000000u);
    static_assert(CmacRead[7] == 0x00AA0000u);

    constexpr inline auto KeySourceWrite = GetSecureScratchMasks<SecureRegister_KeySourceWrite>();
    static_assert(KeySourceWrite[0] == 0x00000000u);
    static_assert(KeySourceWrite[1] == 0x00000000u);
    static_assert(KeySourceWrite[2] == 0x00000055u);
    static_assert(KeySourceWrite[3] == 0x00000000u);
    static_assert(KeySourceWrite[4] == 0x00000000u);
    static_assert(KeySourceWrite[5] == 0x00000000u);
    static_assert(KeySourceWrite[6] == 0x00000000u);
    static_assert(KeySourceWrite[7] == 0x00000000u);

    constexpr inline auto KeySourceRead = GetSecureScratchMasks<SecureRegister_KeySourceRead>();
    static_assert(KeySourceRead[0] == 0x00000000u);
    static_assert(KeySourceRead[1] == 0x00000000u);
    static_assert(KeySourceRead[2] == 0x000000AAu);
    static_assert(KeySourceRead[3] == 0x00000000u);
    static_assert(KeySourceRead[4] == 0x00000000u);
    static_assert(KeySourceRead[5] == 0x00000000u);
    static_assert(KeySourceRead[6] == 0x00000000u);
    static_assert(KeySourceRead[7] == 0x00000000u);

    constexpr inline auto Srk = GetSecureScratchMasks<SecureRegister_Srk>();
    static_assert(Srk[0] == 0x000FF000u);
    static_assert(Srk[1] == 0x00000000u);
    static_assert(Srk[2] == 0x00000000u);
    static_assert(Srk[3] == 0x00000000u);
    static_assert(Srk[4] == 0x00000000u);
    static_assert(Srk[5] == 0x00000000u);
    static_assert(Srk[6] == 0x00000000u);
    static_assert(Srk[7] == 0x00000000u);


}