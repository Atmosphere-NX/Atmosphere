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
#include <vapours.hpp>
#include "crypto_aes_impl.arch.x64.hpp"

namespace ams::crypto::impl {

    namespace {

        constexpr bool IsSupportedKeySize(size_t size) {
            return size == 16 || size == 24 || size == 32;
        }

        constexpr int WordsPerBlock = AesImpl<16>::BlockSize / sizeof(u32);
        static_assert(AesImpl<16>::BlockSize == AesImpl<24>::BlockSize);
        static_assert(AesImpl<16>::BlockSize == AesImpl<32>::BlockSize);

        bool GetAesNiAvailabilityImpl() {
            /* Call cpu id. */
            int a = 0, b = 0, c = 0, d = 0;
            __asm__ __volatile__("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "0"(1) : "memory");

            /* Check for AES-NI and SSE2. */
            return (c & (1 << 25)) && (d & (1 << 26));
        }

        static_assert(util::IsLittleEndian());

        constexpr const u8 RoundKeyRcon0[] = {
            0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1B, 0x36, 0x6C, 0xD8, 0xAB, 0x4D, 0x9A, 0x2F,
            0x5E, 0xBC, 0x63, 0xC6, 0x97, 0x35, 0x6A, 0xD4, 0xB3, 0x7D, 0xFA, 0xEF, 0xC5, 0x91,
        };

        constexpr const u8 SubBytesTable[0x100] = {
            0x63, 0x7C, 0x77, 0x7B, 0xF2, 0x6B, 0x6F, 0xC5, 0x30, 0x01, 0x67, 0x2B, 0xFE, 0xD7, 0xAB, 0x76,
            0xCA, 0x82, 0xC9, 0x7D, 0xFA, 0x59, 0x47, 0xF0, 0xAD, 0xD4, 0xA2, 0xAF, 0x9C, 0xA4, 0x72, 0xC0,
            0xB7, 0xFD, 0x93, 0x26, 0x36, 0x3F, 0xF7, 0xCC, 0x34, 0xA5, 0xE5, 0xF1, 0x71, 0xD8, 0x31, 0x15,
            0x04, 0xC7, 0x23, 0xC3, 0x18, 0x96, 0x05, 0x9A, 0x07, 0x12, 0x80, 0xE2, 0xEB, 0x27, 0xB2, 0x75,
            0x09, 0x83, 0x2C, 0x1A, 0x1B, 0x6E, 0x5A, 0xA0, 0x52, 0x3B, 0xD6, 0xB3, 0x29, 0xE3, 0x2F, 0x84,
            0x53, 0xD1, 0x00, 0xED, 0x20, 0xFC, 0xB1, 0x5B, 0x6A, 0xCB, 0xBE, 0x39, 0x4A, 0x4C, 0x58, 0xCF,
            0xD0, 0xEF, 0xAA, 0xFB, 0x43, 0x4D, 0x33, 0x85, 0x45, 0xF9, 0x02, 0x7F, 0x50, 0x3C, 0x9F, 0xA8,
            0x51, 0xA3, 0x40, 0x8F, 0x92, 0x9D, 0x38, 0xF5, 0xBC, 0xB6, 0xDA, 0x21, 0x10, 0xFF, 0xF3, 0xD2,
            0xCD, 0x0C, 0x13, 0xEC, 0x5F, 0x97, 0x44, 0x17, 0xC4, 0xA7, 0x7E, 0x3D, 0x64, 0x5D, 0x19, 0x73,
            0x60, 0x81, 0x4F, 0xDC, 0x22, 0x2A, 0x90, 0x88, 0x46, 0xEE, 0xB8, 0x14, 0xDE, 0x5E, 0x0B, 0xDB,
            0xE0, 0x32, 0x3A, 0x0A, 0x49, 0x06, 0x24, 0x5C, 0xC2, 0xD3, 0xAC, 0x62, 0x91, 0x95, 0xE4, 0x79,
            0xE7, 0xC8, 0x37, 0x6D, 0x8D, 0xD5, 0x4E, 0xA9, 0x6C, 0x56, 0xF4, 0xEA, 0x65, 0x7A, 0xAE, 0x08,
            0xBA, 0x78, 0x25, 0x2E, 0x1C, 0xA6, 0xB4, 0xC6, 0xE8, 0xDD, 0x74, 0x1F, 0x4B, 0xBD, 0x8B, 0x8A,
            0x70, 0x3E, 0xB5, 0x66, 0x48, 0x03, 0xF6, 0x0E, 0x61, 0x35, 0x57, 0xB9, 0x86, 0xC1, 0x1D, 0x9E,
            0xE1, 0xF8, 0x98, 0x11, 0x69, 0xD9, 0x8E, 0x94, 0x9B, 0x1E, 0x87, 0xE9, 0xCE, 0x55, 0x28, 0xDF,
            0x8C, 0xA1, 0x89, 0x0D, 0xBF, 0xE6, 0x42, 0x68, 0x41, 0x99, 0x2D, 0x0F, 0xB0, 0x54, 0xBB, 0x16,
        };

        constexpr const u8 InvSubBytesTable[0x100] = {
            0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
            0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
            0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
            0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
            0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
            0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
            0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
            0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
            0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
            0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
            0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
            0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
            0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
            0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
            0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
            0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D,
        };

        constexpr bool IsSubBytesTableValid() {
            for (size_t i = 0; i < 0x100; ++i) {
                if (SubBytesTable[InvSubBytesTable[i]] != i) {
                    return false;
                }
                if (InvSubBytesTable[SubBytesTable[i]] != i) {
                    return false;
                }
            }

            return true;
        }

        static_assert(IsSubBytesTableValid());

        constexpr const u32 EncryptTable[0x100] = {
            0xA56363C6, 0x847C7CF8, 0x997777EE, 0x8D7B7BF6, 0x0DF2F2FF, 0xBD6B6BD6, 0xB16F6FDE, 0x54C5C591,
            0x50303060, 0x03010102, 0xA96767CE, 0x7D2B2B56, 0x19FEFEE7, 0x62D7D7B5, 0xE6ABAB4D, 0x9A7676EC,
            0x45CACA8F, 0x9D82821F, 0x40C9C989, 0x877D7DFA, 0x15FAFAEF, 0xEB5959B2, 0xC947478E, 0x0BF0F0FB,
            0xECADAD41, 0x67D4D4B3, 0xFDA2A25F, 0xEAAFAF45, 0xBF9C9C23, 0xF7A4A453, 0x967272E4, 0x5BC0C09B,
            0xC2B7B775, 0x1CFDFDE1, 0xAE93933D, 0x6A26264C, 0x5A36366C, 0x413F3F7E, 0x02F7F7F5, 0x4FCCCC83,
            0x5C343468, 0xF4A5A551, 0x34E5E5D1, 0x08F1F1F9, 0x937171E2, 0x73D8D8AB, 0x53313162, 0x3F15152A,
            0x0C040408, 0x52C7C795, 0x65232346, 0x5EC3C39D, 0x28181830, 0xA1969637, 0x0F05050A, 0xB59A9A2F,
            0x0907070E, 0x36121224, 0x9B80801B, 0x3DE2E2DF, 0x26EBEBCD, 0x6927274E, 0xCDB2B27F, 0x9F7575EA,
            0x1B090912, 0x9E83831D, 0x742C2C58, 0x2E1A1A34, 0x2D1B1B36, 0xB26E6EDC, 0xEE5A5AB4, 0xFBA0A05B,
            0xF65252A4, 0x4D3B3B76, 0x61D6D6B7, 0xCEB3B37D, 0x7B292952, 0x3EE3E3DD, 0x712F2F5E, 0x97848413,
            0xF55353A6, 0x68D1D1B9, 0x00000000, 0x2CEDEDC1, 0x60202040, 0x1FFCFCE3, 0xC8B1B179, 0xED5B5BB6,
            0xBE6A6AD4, 0x46CBCB8D, 0xD9BEBE67, 0x4B393972, 0xDE4A4A94, 0xD44C4C98, 0xE85858B0, 0x4ACFCF85,
            0x6BD0D0BB, 0x2AEFEFC5, 0xE5AAAA4F, 0x16FBFBED, 0xC5434386, 0xD74D4D9A, 0x55333366, 0x94858511,
            0xCF45458A, 0x10F9F9E9, 0x06020204, 0x817F7FFE, 0xF05050A0, 0x443C3C78, 0xBA9F9F25, 0xE3A8A84B,
            0xF35151A2, 0xFEA3A35D, 0xC0404080, 0x8A8F8F05, 0xAD92923F, 0xBC9D9D21, 0x48383870, 0x04F5F5F1,
            0xDFBCBC63, 0xC1B6B677, 0x75DADAAF, 0x63212142, 0x30101020, 0x1AFFFFE5, 0x0EF3F3FD, 0x6DD2D2BF,
            0x4CCDCD81, 0x140C0C18, 0x35131326, 0x2FECECC3, 0xE15F5FBE, 0xA2979735, 0xCC444488, 0x3917172E,
            0x57C4C493, 0xF2A7A755, 0x827E7EFC, 0x473D3D7A, 0xAC6464C8, 0xE75D5DBA, 0x2B191932, 0x957373E6,
            0xA06060C0, 0x98818119, 0xD14F4F9E, 0x7FDCDCA3, 0x66222244, 0x7E2A2A54, 0xAB90903B, 0x8388880B,
            0xCA46468C, 0x29EEEEC7, 0xD3B8B86B, 0x3C141428, 0x79DEDEA7, 0xE25E5EBC, 0x1D0B0B16, 0x76DBDBAD,
            0x3BE0E0DB, 0x56323264, 0x4E3A3A74, 0x1E0A0A14, 0xDB494992, 0x0A06060C, 0x6C242448, 0xE45C5CB8,
            0x5DC2C29F, 0x6ED3D3BD, 0xEFACAC43, 0xA66262C4, 0xA8919139, 0xA4959531, 0x37E4E4D3, 0x8B7979F2,
            0x32E7E7D5, 0x43C8C88B, 0x5937376E, 0xB76D6DDA, 0x8C8D8D01, 0x64D5D5B1, 0xD24E4E9C, 0xE0A9A949,
            0xB46C6CD8, 0xFA5656AC, 0x07F4F4F3, 0x25EAEACF, 0xAF6565CA, 0x8E7A7AF4, 0xE9AEAE47, 0x18080810,
            0xD5BABA6F, 0x887878F0, 0x6F25254A, 0x722E2E5C, 0x241C1C38, 0xF1A6A657, 0xC7B4B473, 0x51C6C697,
            0x23E8E8CB, 0x7CDDDDA1, 0x9C7474E8, 0x211F1F3E, 0xDD4B4B96, 0xDCBDBD61, 0x868B8B0D, 0x858A8A0F,
            0x907070E0, 0x423E3E7C, 0xC4B5B571, 0xAA6666CC, 0xD8484890, 0x05030306, 0x01F6F6F7, 0x120E0E1C,
            0xA36161C2, 0x5F35356A, 0xF95757AE, 0xD0B9B969, 0x91868617, 0x58C1C199, 0x271D1D3A, 0xB99E9E27,
            0x38E1E1D9, 0x13F8F8EB, 0xB398982B, 0x33111122, 0xBB6969D2, 0x70D9D9A9, 0x898E8E07, 0xA7949433,
            0xB69B9B2D, 0x221E1E3C, 0x92878715, 0x20E9E9C9, 0x49CECE87, 0xFF5555AA, 0x78282850, 0x7ADFDFA5,
            0x8F8C8C03, 0xF8A1A159, 0x80898909, 0x170D0D1A, 0xDABFBF65, 0x31E6E6D7, 0xC6424284, 0xB86868D0,
            0xC3414182, 0xB0999929, 0x772D2D5A, 0x110F0F1E, 0xCBB0B07B, 0xFC5454A8, 0xD6BBBB6D, 0x3A16162C,
        };

        constexpr const u32 DecryptTable[0x100] = {
            0x50A7F451, 0x5365417E, 0xC3A4171A, 0x965E273A, 0xCB6BAB3B, 0xF1459D1F, 0xAB58FAAC, 0x9303E34B,
            0x55FA3020, 0xF66D76AD, 0x9176CC88, 0x254C02F5, 0xFCD7E54F, 0xD7CB2AC5, 0x80443526, 0x8FA362B5,
            0x495AB1DE, 0x671BBA25, 0x980EEA45, 0xE1C0FE5D, 0x02752FC3, 0x12F04C81, 0xA397468D, 0xC6F9D36B,
            0xE75F8F03, 0x959C9215, 0xEB7A6DBF, 0xDA595295, 0x2D83BED4, 0xD3217458, 0x2969E049, 0x44C8C98E,
            0x6A89C275, 0x78798EF4, 0x6B3E5899, 0xDD71B927, 0xB64FE1BE, 0x17AD88F0, 0x66AC20C9, 0xB43ACE7D,
            0x184ADF63, 0x82311AE5, 0x60335197, 0x457F5362, 0xE07764B1, 0x84AE6BBB, 0x1CA081FE, 0x942B08F9,
            0x58684870, 0x19FD458F, 0x876CDE94, 0xB7F87B52, 0x23D373AB, 0xE2024B72, 0x578F1FE3, 0x2AAB5566,
            0x0728EBB2, 0x03C2B52F, 0x9A7BC586, 0xA50837D3, 0xF2872830, 0xB2A5BF23, 0xBA6A0302, 0x5C8216ED,
            0x2B1CCF8A, 0x92B479A7, 0xF0F207F3, 0xA1E2694E, 0xCDF4DA65, 0xD5BE0506, 0x1F6234D1, 0x8AFEA6C4,
            0x9D532E34, 0xA055F3A2, 0x32E18A05, 0x75EBF6A4, 0x39EC830B, 0xAAEF6040, 0x069F715E, 0x51106EBD,
            0xF98A213E, 0x3D06DD96, 0xAE053EDD, 0x46BDE64D, 0xB58D5491, 0x055DC471, 0x6FD40604, 0xFF155060,
            0x24FB9819, 0x97E9BDD6, 0xCC434089, 0x779ED967, 0xBD42E8B0, 0x888B8907, 0x385B19E7, 0xDBEEC879,
            0x470A7CA1, 0xE90F427C, 0xC91E84F8, 0x00000000, 0x83868009, 0x48ED2B32, 0xAC70111E, 0x4E725A6C,
            0xFBFF0EFD, 0x5638850F, 0x1ED5AE3D, 0x27392D36, 0x64D90F0A, 0x21A65C68, 0xD1545B9B, 0x3A2E3624,
            0xB1670A0C, 0x0FE75793, 0xD296EEB4, 0x9E919B1B, 0x4FC5C080, 0xA220DC61, 0x694B775A, 0x161A121C,
            0x0ABA93E2, 0xE52AA0C0, 0x43E0223C, 0x1D171B12, 0x0B0D090E, 0xADC78BF2, 0xB9A8B62D, 0xC8A91E14,
            0x8519F157, 0x4C0775AF, 0xBBDD99EE, 0xFD607FA3, 0x9F2601F7, 0xBCF5725C, 0xC53B6644, 0x347EFB5B,
            0x7629438B, 0xDCC623CB, 0x68FCEDB6, 0x63F1E4B8, 0xCADC31D7, 0x10856342, 0x40229713, 0x2011C684,
            0x7D244A85, 0xF83DBBD2, 0x1132F9AE, 0x6DA129C7, 0x4B2F9E1D, 0xF330B2DC, 0xEC52860D, 0xD0E3C177,
            0x6C16B32B, 0x99B970A9, 0xFA489411, 0x2264E947, 0xC48CFCA8, 0x1A3FF0A0, 0xD82C7D56, 0xEF903322,
            0xC74E4987, 0xC1D138D9, 0xFEA2CA8C, 0x360BD498, 0xCF81F5A6, 0x28DE7AA5, 0x268EB7DA, 0xA4BFAD3F,
            0xE49D3A2C, 0x0D927850, 0x9BCC5F6A, 0x62467E54, 0xC2138DF6, 0xE8B8D890, 0x5EF7392E, 0xF5AFC382,
            0xBE805D9F, 0x7C93D069, 0xA92DD56F, 0xB31225CF, 0x3B99ACC8, 0xA77D1810, 0x6E639CE8, 0x7BBB3BDB,
            0x097826CD, 0xF418596E, 0x01B79AEC, 0xA89A4F83, 0x656E95E6, 0x7EE6FFAA, 0x08CFBC21, 0xE6E815EF,
            0xD99BE7BA, 0xCE366F4A, 0xD4099FEA, 0xD67CB029, 0xAFB2A431, 0x31233F2A, 0x3094A5C6, 0xC066A235,
            0x37BC4E74, 0xA6CA82FC, 0xB0D090E0, 0x15D8A733, 0x4A9804F1, 0xF7DAEC41, 0x0E50CD7F, 0x2FF69117,
            0x8DD64D76, 0x4DB0EF43, 0x544DAACC, 0xDF0496E4, 0xE3B5D19E, 0x1B886A4C, 0xB81F2CC1, 0x7F516546,
            0x04EA5E9D, 0x5D358C01, 0x737487FA, 0x2E410BFB, 0x5A1D67B3, 0x52D2DB92, 0x335610E9, 0x1347D66D,
            0x8C61D79A, 0x7A0CA137, 0x8E14F859, 0x893C13EB, 0xEE27A9CE, 0x35C961B7, 0xEDE51CE1, 0x3CB1477A,
            0x59DFD29C, 0x3F73F255, 0x79CE1418, 0xBF37C773, 0xEACDF753, 0x5BAAFD5F, 0x146F3DDF, 0x86DB4478,
            0x81F3AFCA, 0x3EC468B9, 0x2C342438, 0x5F40A3C2, 0x72C31D16, 0x0C25E2BC, 0x8B493C28, 0x41950DFF,
            0x7101A839, 0xDEB30C08, 0x9CE4B4D8, 0x90C15664, 0x6184CB7B, 0x70B632D5, 0x745C6C48, 0x4257B8D0,
        };

        constexpr auto AesWordByte0Shift = 0 * BITSIZEOF(u8);
        constexpr auto AesWordByte1Shift = 1 * BITSIZEOF(u8);
        constexpr auto AesWordByte2Shift = 2 * BITSIZEOF(u8);
        constexpr auto AesWordByte3Shift = 3 * BITSIZEOF(u8);

        constexpr auto AesMixShift = 3 * BITSIZEOF(u8);

        constexpr void InverseMixColumns(u32 *dst, const u32 *src) {
            for (auto i = 0; i < WordsPerBlock; ++i) {
                const u32 v0 = src[i];
                const u32 v1 = (((v0 & 0x7F7F7F7Fu) << 1) ^ (((v0 & 0x80808080) >> 7) * 0x1B));
                const u32 v2 = (((v1 & 0x7F7F7F7Fu) << 1) ^ (((v1 & 0x80808080) >> 7) * 0x1B));
                const u32 v3 = (((v2 & 0x7F7F7F7Fu) << 1) ^ (((v2 & 0x80808080) >> 7) * 0x1B));

                u32 v = v0 ^ v3;
                v ^= util::RotateLeft(v, AesMixShift) ^ v2;
                v ^= util::RotateLeft(v, AesMixShift) ^ v1;
                v ^= util::RotateLeft(v, AesMixShift) ^ v0;

                dst[i] = v;
            }
        }

        constexpr u32 SubBytesAndRotate(u32 v) {
            return (static_cast<u32>(SubBytesTable[(v >> AesWordByte0Shift) & 0xFFu]) << AesWordByte3Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte1Shift) & 0xFFu]) << AesWordByte0Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte2Shift) & 0xFFu]) << AesWordByte1Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte3Shift) & 0xFFu]) << AesWordByte2Shift);
        }

        constexpr u32 SubBytes(u32 v) {
            return (static_cast<u32>(SubBytesTable[(v >> AesWordByte0Shift) & 0xFFu]) << AesWordByte0Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte1Shift) & 0xFFu]) << AesWordByte1Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte2Shift) & 0xFFu]) << AesWordByte2Shift) ^
                   (static_cast<u32>(SubBytesTable[(v >> AesWordByte3Shift) & 0xFFu]) << AesWordByte3Shift);
        }

        constexpr u32 ShiftSubMix(u32 v0, u32 v1, u32 v2, u32 v3) {
            return (util::RotateLeft(static_cast<u32>(EncryptTable[(v0 >> AesWordByte0Shift) & 0xFFu]), AesWordByte0Shift)) ^
                   (util::RotateLeft(static_cast<u32>(EncryptTable[(v1 >> AesWordByte1Shift) & 0xFFu]), AesWordByte1Shift)) ^
                   (util::RotateLeft(static_cast<u32>(EncryptTable[(v2 >> AesWordByte2Shift) & 0xFFu]), AesWordByte2Shift)) ^
                   (util::RotateLeft(static_cast<u32>(EncryptTable[(v3 >> AesWordByte3Shift) & 0xFFu]), AesWordByte3Shift));
        }

        constexpr u32 ShiftSub(u32 v0, u32 v1, u32 v2, u32 v3) {
            return (static_cast<u32>(SubBytesTable[(v0 >> AesWordByte0Shift) & 0xFFu]) << AesWordByte0Shift) ^
                   (static_cast<u32>(SubBytesTable[(v1 >> AesWordByte1Shift) & 0xFFu]) << AesWordByte1Shift) ^
                   (static_cast<u32>(SubBytesTable[(v2 >> AesWordByte2Shift) & 0xFFu]) << AesWordByte2Shift) ^
                   (static_cast<u32>(SubBytesTable[(v3 >> AesWordByte3Shift) & 0xFFu]) << AesWordByte3Shift);
        }

        constexpr u32 InvShiftSubMix(u32 v0, u32 v1, u32 v2, u32 v3) {
            return (util::RotateLeft(static_cast<u32>(DecryptTable[(v0 >> AesWordByte0Shift) & 0xFFu]), AesWordByte0Shift)) ^
                   (util::RotateLeft(static_cast<u32>(DecryptTable[(v1 >> AesWordByte1Shift) & 0xFFu]), AesWordByte1Shift)) ^
                   (util::RotateLeft(static_cast<u32>(DecryptTable[(v2 >> AesWordByte2Shift) & 0xFFu]), AesWordByte2Shift)) ^
                   (util::RotateLeft(static_cast<u32>(DecryptTable[(v3 >> AesWordByte3Shift) & 0xFFu]), AesWordByte3Shift));
        }

        constexpr u32 InvShiftSub(u32 v0, u32 v1, u32 v2, u32 v3) {
            return (static_cast<u32>(InvSubBytesTable[(v0 >> AesWordByte0Shift) & 0xFFu]) << AesWordByte0Shift) ^
                   (static_cast<u32>(InvSubBytesTable[(v1 >> AesWordByte1Shift) & 0xFFu]) << AesWordByte1Shift) ^
                   (static_cast<u32>(InvSubBytesTable[(v2 >> AesWordByte2Shift) & 0xFFu]) << AesWordByte2Shift) ^
                   (static_cast<u32>(InvSubBytesTable[(v3 >> AesWordByte3Shift) & 0xFFu]) << AesWordByte3Shift);
        }

    }

    const bool g_is_aes_ni_available = GetAesNiAvailabilityImpl();

    template<size_t KeySize>
    AesImpl<KeySize>::~AesImpl() {
        ClearMemory(this, sizeof(*this));
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::Initialize(const void *key, size_t key_size, bool is_encrypt) {
        /* Check pre-conditions. */
        static_assert(IsSupportedKeySize(KeySize));
        AMS_ASSERT(key != nullptr);
        AMS_ASSERT(key_size == KeySize);
        AMS_UNUSED(key_size);

        /* Set up key. */
        u32 *dst = m_round_keys;
        std::memcpy(dst, key, KeySize);

        /* Perform key scheduling. */
        constexpr auto InitialKeyWords = KeySize / sizeof(u32);
        u32 tmp = dst[InitialKeyWords - 1];

        for (auto i = InitialKeyWords; i < (RoundCount  + 1) * 4; ++i) {
            const auto idx_in_key = i % InitialKeyWords;
            if (idx_in_key == 0) {
                /* At start of key word, we need to handle sub/rotate/rcon. */
                tmp = SubBytesAndRotate(tmp);
                tmp ^= (RoundKeyRcon0[i / InitialKeyWords - 1] << AesWordByte0Shift);
            } else if ((InitialKeyWords > 6) && idx_in_key == 4) {
                /* Halfway into a 256-bit key word, we need to do an additional subbytes. */
                tmp = SubBytes(tmp);
            }

            /* Set the key word. */
            tmp ^= dst[i - InitialKeyWords];
            dst[i] = tmp;
        }

        /* If decrypting, perform inverse mix columns on all round keys. */
        if (!is_encrypt) {
            if (IsAesNiAvailable()) {
                auto *key8 = reinterpret_cast<u8 *>(m_round_keys) + BlockSize;

                for (auto i = 1; i < RoundCount; ++i) {
                    auto * const key128 = reinterpret_cast<__m128i *>(key8);
                    _mm_storeu_si128(key128, _mm_aesimc_si128(_mm_loadu_si128(key128)));
                    key8 += BlockSize;
                }
            } else {
                for (auto i = 1; i < RoundCount; ++i) {
                    InverseMixColumns(m_round_keys + WordsPerBlock * i, m_round_keys + WordsPerBlock * i);
                }
            }
        }
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::EncryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        static_assert(IsSupportedKeySize(KeySize));
        AMS_ASSERT(dst_size == BlockSize && src_size == BlockSize);
        AMS_UNUSED(dst_size, src_size);

        /* Perform block encryption. */
        if (IsAesNiAvailable()) {
            const auto *key8 = reinterpret_cast<const u8 *>(m_round_keys);

            /* Load the block. */
            auto block = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));

            /* Add the first round key. */
            block = _mm_xor_si128(block, _mm_loadu_si128(reinterpret_cast<const __m128i *>(key8)));
            key8 += BlockSize;

            /* Perform aes round on remaining round keys. */
            for (auto i = 1; i < RoundCount; ++i) {
                block = _mm_aesenc_si128(block, _mm_loadu_si128(reinterpret_cast<const __m128i *>(key8)));
                key8 += BlockSize;
            }

            /* Do final update. */
            block = _mm_aesenclast_si128(block, _mm_loadu_si128(reinterpret_cast<const __m128i *>(key8)));

            /* Store the output. */
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), block);
        } else {
            static_assert(WordsPerBlock == 4);

            /* Without AES-NI, we'll operate on words. */
            const u32 *key32 = m_round_keys;
            const u32 *src32 = static_cast<const u32 *>(src);
                  u32 *dst32 = static_cast<      u32 *>(dst);

            /* Add the first round key. */
            u32 v0 = src32[0] ^ key32[0];
            u32 v1 = src32[1] ^ key32[1];
            u32 v2 = src32[2] ^ key32[2];
            u32 v3 = src32[3] ^ key32[3];
            key32 += 4;

            /* Perform each round. */
            auto round = RoundCount;
            while (--round > 0) {
                /* Perform aes round. */
                const u32 e0 = ShiftSubMix(v0, v1, v2, v3);
                const u32 e1 = ShiftSubMix(v1, v2, v3, v0);
                const u32 e2 = ShiftSubMix(v2, v3, v0, v1);
                const u32 e3 = ShiftSubMix(v3, v0, v1, v2);

                /* Add the round key. */
                v0 = e0 ^ key32[0];
                v1 = e1 ^ key32[1];
                v2 = e2 ^ key32[2];
                v3 = e3 ^ key32[3];
                key32 += 4;
            }

            /* Perform the final round. */
            dst32[0] = key32[0] ^ ShiftSub(v0, v1, v2, v3);
            dst32[1] = key32[1] ^ ShiftSub(v1, v2, v3, v0);
            dst32[2] = key32[2] ^ ShiftSub(v2, v3, v0, v1);
            dst32[3] = key32[3] ^ ShiftSub(v3, v0, v1, v2);
        }
    }

    template<size_t KeySize>
    void AesImpl<KeySize>::DecryptBlock(void *dst, size_t dst_size, const void *src, size_t src_size) const {
        static_assert(IsSupportedKeySize(KeySize));
        AMS_ASSERT(dst_size == BlockSize && src_size == BlockSize);
        AMS_UNUSED(dst_size, src_size);

        /* Perform block decryption. */
        if (IsAesNiAvailable()) {
            const auto *key8 = reinterpret_cast<const u8 *>(m_round_keys) + (RoundCount * BlockSize);

            /* Load the block. */
            auto block = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));

            /* Add the final round key. */
            block = _mm_xor_si128(block, _mm_loadu_si128(reinterpret_cast<const __m128i *>(key8)));
            key8 -= BlockSize;

            /* Perform aes invround on remaining round keys. */
            for (auto i = RoundCount; i > 1; --i) {
                block = _mm_aesdec_si128(block, _mm_loadu_si128(reinterpret_cast<const __m128i *>(key8)));
                key8 -= BlockSize;
            }

            /* Do final update. */
            block = _mm_aesdeclast_si128(block, _mm_loadu_si128(reinterpret_cast<const __m128i *>(key8)));

            /* Store the output. */
            _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), block);
        } else {
            static_assert(WordsPerBlock == 4);

            /* Without AES-NI, we'll operate on words. */
            const u32 *key32 = m_round_keys + WordsPerBlock * RoundCount;
            const u32 *src32 = static_cast<const u32 *>(src);
                  u32 *dst32 = static_cast<      u32 *>(dst);

            /* Add the final round key. */
            u32 v0 = src32[0] ^ key32[0];
            u32 v1 = src32[1] ^ key32[1];
            u32 v2 = src32[2] ^ key32[2];
            u32 v3 = src32[3] ^ key32[3];
            key32 -= 4;

            /* Perform each round. */
            auto round = RoundCount;
            while (--round > 0) {
                /* Perform aes inv round. */
                const u32 e0 = InvShiftSubMix(v0, v3, v2, v1);
                const u32 e1 = InvShiftSubMix(v1, v0, v3, v2);
                const u32 e2 = InvShiftSubMix(v2, v1, v0, v3);
                const u32 e3 = InvShiftSubMix(v3, v2, v1, v0);

                /* Add the round key. */
                v0 = e0 ^ key32[0];
                v1 = e1 ^ key32[1];
                v2 = e2 ^ key32[2];
                v3 = e3 ^ key32[3];
                key32 -= 4;
            }

            /* Perform the final round. */
            dst32[0] = key32[0] ^ InvShiftSub(v0, v3, v2, v1);
            dst32[1] = key32[1] ^ InvShiftSub(v1, v0, v3, v2);
            dst32[2] = key32[2] ^ InvShiftSub(v2, v1, v0, v3);
            dst32[3] = key32[3] ^ InvShiftSub(v3, v2, v1, v0);
        }
    }

    /* Explicitly instantiate the three supported key sizes. */
    template class AesImpl<16>;
    template class AesImpl<24>;
    template class AesImpl<32>;

}