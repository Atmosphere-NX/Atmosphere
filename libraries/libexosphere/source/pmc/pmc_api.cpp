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
#include <exosphere.hpp>

namespace ams::pmc {

    namespace {

        constinit uintptr_t g_register_address = secmon::MemoryRegionPhysicalDevicePmc.GetAddress();

        constexpr inline u32 WriteMask = 0x1;
        constexpr inline u32  ReadMask = 0x2;

        enum class LockMode {
            Read,
            Write,
            ReadWrite,
        };

        template<LockMode Mode>
        constexpr inline u32 LockMask = [] {
            switch (Mode) {
                case LockMode::Read:      return ReadMask;
                case LockMode::Write:     return WriteMask;
                case LockMode::ReadWrite: return ReadMask | WriteMask;
                default: __builtin_unreachable();
            }
        }();

        constexpr inline size_t NumSecureScratchRegisters = 120;
        constexpr inline size_t NumSecureDisableRegisters = 8;

        template<size_t SecureScratch> requires (SecureScratch < NumSecureScratchRegisters)
        constexpr inline std::pair<size_t, size_t> DisableRegisterIndex = [] {
            if constexpr (SecureScratch < 8) {
                return std::pair<size_t, size_t>{0, 4 + 2 * SecureScratch};
            } else {
                constexpr size_t Relative = SecureScratch - 8;
                return std::pair<size_t, size_t>{1 + (Relative / 16), 2 * (Relative % 16)};
            }
        }();

        struct LockInfo {
            size_t scratch;
            LockMode mode;
        };

        template<LockInfo Info>
        constexpr ALWAYS_INLINE void SetSecureScratchMask(std::array<u32, NumSecureDisableRegisters> &disables) {
            constexpr std::pair<size_t, size_t> Location = DisableRegisterIndex<Info.scratch>;
            disables[Location.first] |= LockMask<Info.mode> << Location.second;
        }

        template<LockInfo... Info>
        constexpr ALWAYS_INLINE void SetSecureScratchMasks(std::array<u32, NumSecureDisableRegisters> &disables) {
            (SetSecureScratchMask<Info>(disables), ...);
        }

        template<size_t... Ix>
        constexpr ALWAYS_INLINE void SetSecureScratchReadWriteMasks(std::array<u32, NumSecureDisableRegisters> &disables) {
            (SetSecureScratchMask<LockInfo{Ix, LockMode::ReadWrite}>(disables), ...);
        }

        template<size_t... Ix>
        constexpr ALWAYS_INLINE void SetSecureScratchReadMasks(std::array<u32, NumSecureDisableRegisters> &disables) {
            (SetSecureScratchMask<LockInfo{Ix, LockMode::Read}>(disables), ...);
        }

        template<size_t... Ix>
        constexpr ALWAYS_INLINE void SetSecureScratchWriteMasks(std::array<u32, NumSecureDisableRegisters> &disables) {
            (SetSecureScratchMask<LockInfo{Ix, LockMode::Write}>(disables), ...);
        }

        template<SecureRegister Register>
        constexpr ALWAYS_INLINE std::array<u32, NumSecureDisableRegisters> GetSecureScratchMasks() {
            std::array<u32, NumSecureDisableRegisters> disables = {};

            if constexpr ((Register & SecureRegister_Other) != 0) {
                constexpr std::array<u32, NumSecureDisableRegisters> NonOtherDisables = GetSecureScratchMasks<static_cast<SecureRegister>(~SecureRegister_Other)>();
                for (size_t i = 0; i < NumSecureDisableRegisters; i++) {
                    disables[i] |= ~NonOtherDisables[i];
                }
                disables[0] &= 0x007FFFF0;
            }
            if constexpr ((Register & SecureRegister_DramParameters) != 0) {
                SetSecureScratchReadWriteMasks<  8,   9,  10,  11,  12,  13,  14,  15,
                                                17,  18,  19,  20,
                                                40,  41,  42,  43,  44,  45,  46,  47,  48,  49,  50,
                                                52,  53,  54,
                                                56,  57,  58,  59,  60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,
                                                79,  80,  81,  82,  83,  84,
                                                86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,
                                               104, 105, 106, 107
                                              >(disables);
            }
            if constexpr ((Register & SecureRegister_ResetVector) != 0) {
                SetSecureScratchReadWriteMasks<34, 35>(disables);
            }
            if constexpr ((Register & SecureRegister_Carveout) != 0) {
                SetSecureScratchReadWriteMasks<16, 39, 51, 55, 74, 75, 76, 77, 78, 99, 100, 101, 102, 103>(disables);
            }
            if constexpr ((Register & SecureRegister_CmacWrite) != 0) {
                SetSecureScratchWriteMasks<112, 113, 114, 115>(disables);
            }
            if constexpr ((Register & SecureRegister_CmacRead) != 0) {
                SetSecureScratchReadMasks<112, 113, 114, 115>(disables);
            }
            if constexpr ((Register & SecureRegister_KeySourceWrite) != 0) {
                SetSecureScratchWriteMasks<24, 25, 26, 27>(disables);
            }
            if constexpr ((Register & SecureRegister_KeySourceRead) != 0) {
                SetSecureScratchReadMasks<24, 25, 26, 27>(disables);
            }
            if constexpr ((Register & SecureRegister_Srk) != 0) {
                SetSecureScratchReadWriteMasks<4, 5, 6, 7>(disables);
            }

            return disables;
        }

        /* Validate that the secure scratch masks produced are correct. */
        #include "pmc_secure_scratch_test.inc"

        ALWAYS_INLINE void LockBits(uintptr_t address, u32 mask) {
            reg::Write(address, reg::Read(address) | mask);
        }

        template<SecureRegister Register>
        ALWAYS_INLINE void SetSecureScratchMasks(uintptr_t address) {
            constexpr auto Masks = GetSecureScratchMasks<Register>();

            if constexpr (Masks[0] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE , Masks[0]); }
            if constexpr (Masks[1] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE2, Masks[1]); }
            if constexpr (Masks[2] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE3, Masks[2]); }
            if constexpr (Masks[3] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE4, Masks[3]); }
            if constexpr (Masks[4] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE5, Masks[4]); }
            if constexpr (Masks[5] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE6, Masks[5]); }
            if constexpr (Masks[6] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE7, Masks[6]); }
            if constexpr (Masks[7] != 0) { LockBits(address + APBDEV_PMC_SEC_DISABLE8, Masks[7]); }

            static_assert(Masks.size() == 8);
        }

        template<SecureRegister Register>
        ALWAYS_INLINE bool TestSecureScratchMasks(uintptr_t address) {
            constexpr auto Masks = GetSecureScratchMasks<Register>();

            if constexpr (Masks[0] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE ) & Masks[0]) != Masks[0]) { return false; } }
            if constexpr (Masks[1] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE2) & Masks[1]) != Masks[1]) { return false; } }
            if constexpr (Masks[2] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE3) & Masks[2]) != Masks[2]) { return false; } }
            if constexpr (Masks[3] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE4) & Masks[3]) != Masks[3]) { return false; } }
            if constexpr (Masks[4] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE5) & Masks[4]) != Masks[4]) { return false; } }
            if constexpr (Masks[5] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE6) & Masks[5]) != Masks[5]) { return false; } }
            if constexpr (Masks[6] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE7) & Masks[6]) != Masks[6]) { return false; } }
            if constexpr (Masks[7] != 0) { if ((reg::Read(address + APBDEV_PMC_SEC_DISABLE8) & Masks[7]) != Masks[7]) { return false; } }
            static_assert(Masks.size() == 8);

            return true;
        }

        NOINLINE void WriteRandomValueToRegister(uintptr_t offset) {
            /* Create an aligned buffer. */
            util::AlignedBuffer<hw::DataCacheLineSize, sizeof(u32)> buf;

            /* Generate random bytes into it. */
            se::GenerateRandomBytes(buf, sizeof(u32));

            /* Read the random value. */
            const u32 random = *reinterpret_cast<const u32 *>(static_cast<u8 *>(buf));

            /* Get the address. */
            const uintptr_t address = g_register_address + offset;

            /* Write the value. */
            reg::Write(address, random);

            /* Verify it was written. */
            AMS_ABORT_UNLESS(reg::Read(address) == random);
        }

    }

    void SetRegisterAddress(uintptr_t address) {
        g_register_address = address;
    }

    void InitializeRandomScratch() {
        /* Write random data to the scratch that contains the SRK. */
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH4);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH5);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH6);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH7);

        /* Lock the SRK scratch. */
        LockSecureRegister(SecureRegister_Srk);

        /* Write random data to the scratch used for tzram cmac. */
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH112);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH113);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH114);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH115);

        /* Write random data to the scratch used for tzram key source. */
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH24);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH25);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH26);
        WriteRandomValueToRegister(APBDEV_PMC_SECURE_SCRATCH27);

        /* Here, Nintendo locks the SRK scratch a second time. */
        /* This may just be "to be sure". */
        LockSecureRegister(SecureRegister_Srk);
    }

    void EnableWakeEventDetection() {
        /* Get the address. */
        const uintptr_t address = g_register_address;

        /* Wait 75us, then enable event detection, then wait another 75us. */
        util::WaitMicroSeconds(75);
        reg::ReadWrite(address + APBDEV_PMC_CNTRL2,  PMC_REG_BITS_ENUM(CNTRL2_WAKE_DET_EN, ENABLE));
        util::WaitMicroSeconds(75);

        /* Enable all wake events. */
        reg::Write(address + APBDEV_PMC_WAKE_STATUS,  0xFFFFFFFFu);
        reg::Write(address + APBDEV_PMC_WAKE2_STATUS, 0xFFFFFFFFu);
        util::WaitMicroSeconds(75);
    }

    void ConfigureForSc7Entry() {
        /* Get the address. */
        const uintptr_t address = g_register_address;

        /* Configure the bootrom to perform a warmboot. */
        reg::Write(address + APBDEV_PMC_SCRATCH0, 0x1);

        /* Enable the TSC multiplier. */
        reg::ReadWrite(address + APBDEV_PMC_DPD_ENABLE, PMC_REG_BITS_ENUM(DPD_ENABLE_TSC_MULT_EN, ENABLE));
    }

    void LockSecureRegister(SecureRegister reg) {
        /* Get the address. */
        const uintptr_t address = g_register_address;

        /* Apply each mask. */
        #define PMC_PROCESS_REG(REG) do { if ((reg & SecureRegister_##REG) != 0) { SetSecureScratchMasks<SecureRegister_##REG>(address); } } while (0)
        PMC_PROCESS_REG(Other);
        PMC_PROCESS_REG(DramParameters);
        PMC_PROCESS_REG(ResetVector);
        PMC_PROCESS_REG(Carveout);
        PMC_PROCESS_REG(CmacWrite);
        PMC_PROCESS_REG(CmacRead);
        PMC_PROCESS_REG(KeySourceWrite);
        PMC_PROCESS_REG(KeySourceRead);
        PMC_PROCESS_REG(Srk);
        #undef PMC_PROCESS_REG

    }

    LockState GetSecureRegisterLockState(SecureRegister reg) {
        bool all_valid = true;
        bool any_valid = false;

        /* Get the address. */
        const uintptr_t address = g_register_address;

        /* Test each mask. */
        #define PMC_PROCESS_REG(REG) do { if ((reg & SecureRegister_##REG) != 0) { const bool test = TestSecureScratchMasks<SecureRegister_##REG>(address); all_valid &= test; any_valid |= test; } } while (0)
        PMC_PROCESS_REG(Other);
        PMC_PROCESS_REG(DramParameters);
        PMC_PROCESS_REG(ResetVector);
        PMC_PROCESS_REG(Carveout);
        PMC_PROCESS_REG(CmacWrite);
        PMC_PROCESS_REG(CmacRead);
        PMC_PROCESS_REG(KeySourceWrite);
        PMC_PROCESS_REG(KeySourceRead);
        PMC_PROCESS_REG(Srk);
        #undef PMC_PROCESS_REG

        if (all_valid) {
            return LockState::Locked;
        } else if (any_valid) {
            return LockState::PartiallyLocked;
        } else {
            return LockState::NotLocked;
        }
    }

}