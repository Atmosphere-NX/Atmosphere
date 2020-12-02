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
#include <vapours.hpp>
#include <exosphere/mmu.hpp>

namespace ams::secmon {

    using Address = u64;

    struct MemoryRegion {
        Address start_address;
        Address end_address;

        constexpr MemoryRegion(Address address, size_t size) : start_address(address), end_address(address + size) {
            if (end_address < start_address) {
                __builtin_unreachable();
            }
        }

        constexpr Address GetStartAddress() const {
            return this->start_address;
        }

        constexpr Address GetAddress() const {
            return this->GetStartAddress();
        }

        constexpr Address GetEndAddress() const {
            return this->end_address;
        }

        constexpr Address GetLastAddress() const {
            return this->end_address - 1;
        }

        constexpr size_t GetSize() const {
            return this->end_address - this->start_address;
        }

        constexpr bool Contains(Address address, size_t size) const {
            return this->start_address <= address && (address + size - 1) <= this->GetLastAddress();
        }

        constexpr bool Contains(const MemoryRegion &rhs) const {
            return this->Contains(rhs.GetStartAddress(), rhs.GetSize());
        }

        template<typename T = void> requires (std::is_same<T, void>::value || util::is_pod<T>::value)
        ALWAYS_INLINE T *GetPointer() const {
            return reinterpret_cast<T *>(this->GetAddress());
        }

        template<typename T = void> requires (std::is_same<T, void>::value || util::is_pod<T>::value)
        ALWAYS_INLINE T *GetEndPointer() const {
            return reinterpret_cast<T *>(this->GetEndAddress());
        }
    };

    constexpr inline const MemoryRegion MemoryRegionVirtual  = MemoryRegion(UINT64_C(0x1F0000000), 2_MB);
    constexpr inline const MemoryRegion MemoryRegionPhysical = MemoryRegion(UINT64_C( 0x40000000), 1_GB);
    constexpr inline const MemoryRegion MemoryRegionDram     = MemoryRegion(UINT64_C( 0x80000000), 2_GB);
    constexpr inline const MemoryRegion MemoryRegionDramHigh = MemoryRegion(MemoryRegionDram.GetEndAddress(), 2_GB);

    constexpr inline const MemoryRegion MemoryRegionDramForMarikoProgram = MemoryRegion(UINT64_C(0xC0000000), 1_GB);
    constexpr inline const MemoryRegion MemoryRegionDramDcFramebuffer = MemoryRegion(UINT64_C(0xC0000000), 4_MB);
    static_assert(MemoryRegionDram.Contains(MemoryRegionDramForMarikoProgram));
    static_assert(MemoryRegionDramForMarikoProgram.Contains(MemoryRegionDramDcFramebuffer));

    constexpr inline const MemoryRegion MemoryRegionDramGpuCarveout = MemoryRegion(UINT64_C(0x80020000), UINT64_C(0x40000));
    static_assert(MemoryRegionDram.Contains(MemoryRegionDramGpuCarveout));

    constexpr inline const MemoryRegion MemoryRegionDramDefaultKernelCarveout = MemoryRegion(UINT64_C(0x80060000), UINT64_C(0x1FFE0000));
    static_assert(MemoryRegionDram.Contains(MemoryRegionDramDefaultKernelCarveout));

    constexpr inline const MemoryRegion MemoryRegionDramPackage2Payloads = MemoryRegion(MemoryRegionDram.GetAddress(), 8_MB);
    static_assert(MemoryRegionDram.Contains(MemoryRegionDramPackage2Payloads));

    constexpr inline const MemoryRegion MemoryRegionDramPackage2 = MemoryRegion(UINT64_C(0xA9800000), UINT64_C(0x07FC0000));
    static_assert(MemoryRegionDram.Contains(MemoryRegionDramPackage2));

    constexpr inline const MemoryRegion MemoryRegionPhysicalIram  = MemoryRegion(UINT64_C(0x40000000), 0x40000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzram = MemoryRegion(UINT64_C(0x7C010000), 0x10000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramMariko = MemoryRegion(UINT64_C(0x7C010000), 0x40000);
    static_assert(MemoryRegionPhysical.Contains(MemoryRegionPhysicalIram));
    static_assert(MemoryRegionPhysical.Contains(MemoryRegionPhysicalTzram));
    static_assert(MemoryRegionPhysicalTzramMariko.Contains(MemoryRegionPhysicalTzram));

    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramVolatile(UINT64_C(0x7C010000),  0x2000);
    static_assert(MemoryRegionPhysicalTzram.Contains(MemoryRegionPhysicalTzramVolatile));

    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramNonVolatile(UINT64_C(0x7C012000),  0xE000);
    static_assert(MemoryRegionPhysicalTzram.Contains(MemoryRegionPhysicalTzramNonVolatile));

    static_assert(MemoryRegionPhysicalTzram.GetSize() == MemoryRegionPhysicalTzramNonVolatile.GetSize() + MemoryRegionPhysicalTzramVolatile.GetSize());

    constexpr inline const MemoryRegion MemoryRegionVirtualL1  = MemoryRegion(util::AlignDown(MemoryRegionVirtual.GetAddress(),  mmu::L1EntrySize), mmu::L1EntrySize);
    constexpr inline const MemoryRegion MemoryRegionPhysicalL1 = MemoryRegion(util::AlignDown(MemoryRegionPhysical.GetAddress(), mmu::L1EntrySize), mmu::L1EntrySize);
    static_assert(MemoryRegionVirtualL1.Contains(MemoryRegionVirtual));
    static_assert(MemoryRegionPhysicalL1.Contains(MemoryRegionPhysical));

    constexpr inline const MemoryRegion MemoryRegionVirtualL2       = MemoryRegion(util::AlignDown(MemoryRegionVirtual.GetAddress(),       mmu::L2EntrySize), mmu::L2EntrySize);
    constexpr inline const MemoryRegion MemoryRegionPhysicalIramL2  = MemoryRegion(util::AlignDown(MemoryRegionPhysicalIram.GetAddress(),  mmu::L2EntrySize), mmu::L2EntrySize);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramL2 = MemoryRegion(util::AlignDown(MemoryRegionPhysicalTzram.GetAddress(), mmu::L2EntrySize), mmu::L2EntrySize);
    static_assert(MemoryRegionVirtualL2.Contains(MemoryRegionVirtual));
    static_assert(MemoryRegionPhysicalIramL2.Contains(MemoryRegionPhysicalIram));
    static_assert(MemoryRegionPhysicalTzramL2.Contains(MemoryRegionPhysicalTzram));

    constexpr inline const MemoryRegion MemoryRegionPhysicalIramBootCode = MemoryRegion(UINT64_C(0x40020000), 0x20000);
    static_assert(MemoryRegionPhysicalIram.Contains(MemoryRegionPhysicalIramBootCode));

    constexpr inline const MemoryRegion MemoryRegionVirtualDevice = MemoryRegion(UINT64_C(0x1F0040000), UINT64_C(0x40000));
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualDevice));

    constexpr inline const MemoryRegion MemoryRegionVirtualDeviceEmpty = MemoryRegion(MemoryRegionVirtualDevice.GetStartAddress(), 0);

    #define AMS_SECMON_FOREACH_DEVICE_REGION(HANDLER, ...)                                                            \
        HANDLER(GicDistributor,                Empty, UINT64_C(0x50041000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(GicCpuInterface,      GicDistributor, UINT64_C(0x50042000),  UINT64_C(0x2000),  true, ## __VA_ARGS__) \
        HANDLER(Uart,                GicCpuInterface, UINT64_C(0x70006000),  UINT64_C(0x1000), false, ## __VA_ARGS__) \
        HANDLER(ClkRst,                         Uart, UINT64_C(0x60006000),  UINT64_C(0x1000), false, ## __VA_ARGS__) \
        HANDLER(RtcPmc,                       ClkRst, UINT64_C(0x7000E000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(Timer,                        RtcPmc, UINT64_C(0x60005000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(System,                        Timer, UINT64_C(0x6000C000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(SecurityEngine,               System, UINT64_C(0x70012000),  UINT64_C(0x2000),  true, ## __VA_ARGS__) \
        HANDLER(SecurityEngine2,      SecurityEngine, UINT64_C(0x70412000),  UINT64_C(0x2000),  true, ## __VA_ARGS__) \
        HANDLER(SysCtr0,             SecurityEngine2, UINT64_C(0x700F0000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(MemoryController,            SysCtr0, UINT64_C(0x70019000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(FuseKFuse,          MemoryController, UINT64_C(0x7000F000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(ApbMisc,                   FuseKFuse, UINT64_C(0x70000000),  UINT64_C(0x4000),  true, ## __VA_ARGS__) \
        HANDLER(FlowController,              ApbMisc, UINT64_C(0x60007000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(BootloaderParams,     FlowController, UINT64_C(0x40000000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(I2c5,               BootloaderParams, UINT64_C(0x7000D000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(Gpio,                           I2c5, UINT64_C(0x6000D000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(I2c1,                           Gpio, UINT64_C(0x7000C000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(ExceptionVectors,               I2c1, UINT64_C(0x6000F000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(MemoryController0,  ExceptionVectors, UINT64_C(0x7001C000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(MemoryController1, MemoryController0, UINT64_C(0x7001D000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(Sdmmc,             MemoryController1, UINT64_C(0x700B0000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(Disp1,                         Sdmmc, UINT64_C(0x54200000),  UINT64_C(0x3000),  true, ## __VA_ARGS__) \
        HANDLER(Dsi,                           Disp1, UINT64_C(0x54300000),  UINT64_C(0x1000),  true, ## __VA_ARGS__) \
        HANDLER(MipiCal,                         Dsi, UINT64_C(0x700E3000),  UINT64_C(0x1000),  true, ## __VA_ARGS__)

    #define DEFINE_DEVICE_REGION(_NAME_, _PREV_, _ADDRESS_, _SIZE_, _SECURE_)                                                                                      \
        constexpr inline const MemoryRegion MemoryRegionVirtualDevice##_NAME_  = MemoryRegion(MemoryRegionVirtualDevice##_PREV_.GetEndAddress() + 0x1000, _SIZE_); \
        constexpr inline const MemoryRegion MemoryRegionPhysicalDevice##_NAME_ = MemoryRegion(_ADDRESS_, _SIZE_);                                                  \
        static_assert(MemoryRegionVirtualDevice.Contains(MemoryRegionVirtualDevice##_NAME_));                                                                      \
        static_assert(MemoryRegionPhysical.Contains(MemoryRegionPhysicalDevice##_NAME_));

    AMS_SECMON_FOREACH_DEVICE_REGION(DEFINE_DEVICE_REGION)

    #undef DEFINE_DEVICE_REGION

    constexpr inline const MemoryRegion MemoryRegionVirtualDeviceFuses = MemoryRegion(MemoryRegionVirtualDeviceFuseKFuse.GetAddress() + 0x800, 0x400);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDeviceFuses = MemoryRegion(MemoryRegionPhysicalDeviceFuseKFuse.GetAddress() + 0x800, 0x400);
    static_assert(MemoryRegionVirtualDeviceFuseKFuse.Contains(MemoryRegionVirtualDeviceFuses));
    static_assert(MemoryRegionPhysicalDeviceFuseKFuse.Contains(MemoryRegionPhysicalDeviceFuses));

    constexpr inline const MemoryRegion MemoryRegionVirtualDeviceActivityMonitor = MemoryRegion(MemoryRegionVirtualDeviceSystem.GetAddress() + 0x800, 0x400);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDeviceActivityMonitor = MemoryRegion(MemoryRegionPhysicalDeviceSystem.GetAddress() + 0x800, 0x400);
    static_assert(MemoryRegionVirtualDeviceSystem.Contains(MemoryRegionVirtualDeviceActivityMonitor));
    static_assert(MemoryRegionPhysicalDeviceSystem.Contains(MemoryRegionPhysicalDeviceActivityMonitor));

    constexpr inline const MemoryRegion MemoryRegionVirtualDeviceUartA  = MemoryRegion(MemoryRegionVirtualDeviceUart.GetAddress()  + 0x000, 0x040);
    constexpr inline const MemoryRegion MemoryRegionVirtualDeviceUartB  = MemoryRegion(MemoryRegionVirtualDeviceUart.GetAddress()  + 0x040, 0x040);
    constexpr inline const MemoryRegion MemoryRegionVirtualDeviceUartC  = MemoryRegion(MemoryRegionVirtualDeviceUart.GetAddress()  + 0x200, 0x100);
    static_assert(MemoryRegionVirtualDeviceUart.Contains(MemoryRegionVirtualDeviceUartA));
    static_assert(MemoryRegionVirtualDeviceUart.Contains(MemoryRegionVirtualDeviceUartB));
    static_assert(MemoryRegionVirtualDeviceUart.Contains(MemoryRegionVirtualDeviceUartC));

    constexpr inline const MemoryRegion MemoryRegionPhysicalDeviceUartA = MemoryRegion(MemoryRegionPhysicalDeviceUart.GetAddress() + 0x000, 0x040);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDeviceUartB = MemoryRegion(MemoryRegionPhysicalDeviceUart.GetAddress() + 0x040, 0x040);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDeviceUartC = MemoryRegion(MemoryRegionPhysicalDeviceUart.GetAddress() + 0x200, 0x100);
    static_assert(MemoryRegionPhysicalDeviceUart.Contains(MemoryRegionPhysicalDeviceUartA));
    static_assert(MemoryRegionPhysicalDeviceUart.Contains(MemoryRegionPhysicalDeviceUartB));
    static_assert(MemoryRegionPhysicalDeviceUart.Contains(MemoryRegionPhysicalDeviceUartC));

    constexpr inline const MemoryRegion MemoryRegionVirtualDevicePmc  = MemoryRegion(MemoryRegionVirtualDeviceRtcPmc.GetAddress()   + 0x400, 0xC00);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDevicePmc  = MemoryRegion(MemoryRegionPhysicalDeviceRtcPmc.GetAddress() + 0x400, 0xC00);
    static_assert(MemoryRegionVirtualDeviceRtcPmc.Contains(MemoryRegionVirtualDevicePmc));
    static_assert(MemoryRegionPhysicalDeviceRtcPmc.Contains(MemoryRegionPhysicalDevicePmc));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramReadOnlyAlias  = MemoryRegion(UINT64_C(0x1F00A0000), MemoryRegionPhysicalTzram.GetSize());
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramReadOnlyAlias = MemoryRegion(MemoryRegionPhysicalTzram.GetAddress(), MemoryRegionPhysicalTzram.GetSize());
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualTzramReadOnlyAlias));
    static_assert(MemoryRegionPhysicalTzram.Contains(MemoryRegionPhysicalTzramReadOnlyAlias));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramProgram(UINT64_C(0x1F00C0000), 0xC000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualTzramProgram));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramProgramExceptionVectors(UINT64_C(0x1F00C0000), 0x800);
    static_assert(MemoryRegionVirtualTzramProgram.Contains(MemoryRegionVirtualTzramProgramExceptionVectors));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramMarikoProgram(UINT64_C(0x1F00D0000), 0x20000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramMarikoProgram(UINT64_C(0x7C020000), 0x20000);
    static_assert(MemoryRegionPhysicalTzramMariko.Contains(MemoryRegionPhysicalTzramMarikoProgram));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramMarikoProgramFatalErrorContext(UINT64_C(0x1F00EF000), 0x1000);
    static_assert(MemoryRegionVirtualTzramMarikoProgram.Contains(MemoryRegionVirtualTzramMarikoProgramFatalErrorContext));

    constexpr inline const MemoryRegion MemoryRegionPhysicalIramFatalErrorContext(UINT64_C(0x4003E000), 0x1000);
    static_assert(MemoryRegionPhysicalIram.Contains(MemoryRegionPhysicalIramFatalErrorContext));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramMarikoProgramStack(UINT64_C(0x1F00F4000), 0x8000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramMarikoProgramStack(UINT64_C(0x7C040000), 0x8000);
    static_assert(MemoryRegionPhysicalTzramMariko.Contains(MemoryRegionPhysicalTzramMarikoProgramStack));

    constexpr inline const MemoryRegion MemoryRegionPhysicalMarikoProgramImage(UINT64_C(0x80020000), 0x20000);
    static_assert(MemoryRegionDram.Contains(MemoryRegionPhysicalMarikoProgramImage));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramProgramMain(UINT64_C(0x1F00C0800), 0xB800);
    static_assert(MemoryRegionVirtualTzramProgram.Contains(MemoryRegionVirtualTzramProgramMain));

    static_assert(MemoryRegionVirtualTzramProgram.GetSize() == MemoryRegionVirtualTzramProgramExceptionVectors.GetSize() + MemoryRegionVirtualTzramProgramMain.GetSize());

    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramProgram(UINT64_C(0x7C012000),  0xC000);
    static_assert(MemoryRegionPhysicalTzramNonVolatile.Contains(MemoryRegionPhysicalTzramProgram));

    constexpr inline const Address PhysicalTzramProgramResetVector = MemoryRegionPhysicalTzramProgram.GetAddress() + MemoryRegionVirtualTzramProgramExceptionVectors.GetSize();
    static_assert(static_cast<u32>(PhysicalTzramProgramResetVector) == PhysicalTzramProgramResetVector);

    constexpr uintptr_t GetPhysicalTzramProgramAddress(uintptr_t virtual_address) {
        return virtual_address - MemoryRegionVirtualTzramProgram.GetStartAddress() + MemoryRegionPhysicalTzramNonVolatile.GetStartAddress();
    }

    constexpr inline const MemoryRegion MemoryRegionVirtualIramSc7Work  = MemoryRegion(UINT64_C(0x1F0120000), MemoryRegionPhysicalTzram.GetSize());
    constexpr inline const MemoryRegion MemoryRegionPhysicalIramSc7Work = MemoryRegion( UINT64_C(0x40020000), MemoryRegionPhysicalTzram.GetSize());
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualIramSc7Work));
    static_assert(MemoryRegionPhysicalIram.Contains(MemoryRegionPhysicalIramSc7Work));

    constexpr inline const MemoryRegion MemoryRegionVirtualIramSc7Firmware  = MemoryRegion(UINT64_C(0x1F0140000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalIramSc7Firmware = MemoryRegion( UINT64_C(0x40003000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualIramSc7Firmware));
    static_assert(MemoryRegionPhysicalIram.Contains(MemoryRegionPhysicalIramSc7Firmware));

    constexpr inline const MemoryRegion MemoryRegionPhysicalIramSecureMonitorDebug(UINT64_C(0x40034000), 0x4000);
    static_assert(MemoryRegionPhysicalIram.Contains(MemoryRegionPhysicalIramSecureMonitorDebug));

    constexpr inline const MemoryRegion MemoryRegionVirtualDebugCode = MemoryRegion(UINT64_C(0x1F0150000), 0x4000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDebugCode = MemoryRegion(UINT64_C(0x40034000), 0x4000);
    static_assert(MemoryRegionPhysicalIramSecureMonitorDebug.Contains(MemoryRegionPhysicalDebugCode));

    constexpr inline const MemoryRegion MemoryRegionVirtualDebug = MemoryRegion(UINT64_C(0x1F0160000), 0x10000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualDebug));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramBootCode  = MemoryRegion(UINT64_C(0x1F01C0000), 0x2000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramBootCode = MemoryRegion( UINT64_C(0x7C010000), 0x2000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualTzramBootCode));
    static_assert(MemoryRegionPhysicalTzramVolatile.Contains(MemoryRegionPhysicalTzramBootCode));

    constexpr inline const MemoryRegion MemoryRegionPhysicalDramMonitorConfiguration = MemoryRegion( UINT64_C(0x8000F000), 0x1000);

    constexpr inline const MemoryRegion MemoryRegionVirtualDramSecureDataStore  = MemoryRegion(UINT64_C(0x1F0100000), 0x10000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramSecureDataStore = MemoryRegion( UINT64_C(0x80010000), 0x10000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualDramSecureDataStore));
    static_assert(MemoryRegionDram.Contains(MemoryRegionPhysicalDramSecureDataStore));

    constexpr inline const MemoryRegion MemoryRegionVirtualDramDebugDataStore  = MemoryRegion(UINT64_C(0x1F0110000), 0x4000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramDebugDataStore = MemoryRegion( UINT64_C(0x8000C000), 0x4000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualDramSecureDataStore));
    static_assert(MemoryRegionDram.Contains(MemoryRegionPhysicalDramSecureDataStore));

    constexpr inline const MemoryRegion MemoryRegionVirtualDramSdmmcMappedData = MemoryRegion(UINT64_C(0x1F0100000), 0xC000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramSdmmcMappedData = MemoryRegion(UINT64_C(0x80010000), 0xC000);

    constexpr inline const MemoryRegion MemoryRegionVirtualDramDcL0DevicePageTable  = MemoryRegion(UINT64_C(0x1F010C000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramDcL0DevicePageTable = MemoryRegion( UINT64_C(0x8001C000), 0x1000);

    constexpr inline const MemoryRegion MemoryRegionVirtualDramSdmmc1L0DevicePageTable  = MemoryRegion(UINT64_C(0x1F010E000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramSdmmc1L0DevicePageTable = MemoryRegion( UINT64_C(0x8001E000), 0x1000);

    constexpr inline const MemoryRegion MemoryRegionVirtualDramSdmmc1L1DevicePageTable  = MemoryRegion(UINT64_C(0x1F010F000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramSdmmc1L1DevicePageTable = MemoryRegion( UINT64_C(0x8001F000), 0x1000);

    constexpr inline const MemoryRegion MemoryRegionVirtualDramSecureDataStoreTzram               = MemoryRegion(UINT64_C(0x1F0100000), 0xE000);
    constexpr inline const MemoryRegion MemoryRegionVirtualDramSecureDataStoreWarmbootFirmware    = MemoryRegion(UINT64_C(0x1F010E000), 0x17C0);
    constexpr inline const MemoryRegion MemoryRegionVirtualDramSecureDataStoreSecurityEngineState = MemoryRegion(UINT64_C(0x1F010F7C0), 0x0840);
    static_assert(MemoryRegionVirtualDramSecureDataStore.Contains(MemoryRegionVirtualDramSecureDataStoreTzram));
    static_assert(MemoryRegionVirtualDramSecureDataStore.Contains(MemoryRegionVirtualDramSecureDataStoreWarmbootFirmware));
    static_assert(MemoryRegionVirtualDramSecureDataStore.Contains(MemoryRegionVirtualDramSecureDataStoreSecurityEngineState));

    constexpr inline const MemoryRegion MemoryRegionPhysicalDramSecureDataStoreTzram               = MemoryRegion(UINT64_C(0x80010000), 0xE000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramSecureDataStoreWarmbootFirmware    = MemoryRegion(UINT64_C(0x8001E000), 0x17C0);
    constexpr inline const MemoryRegion MemoryRegionPhysicalDramSecureDataStoreSecurityEngineState = MemoryRegion(UINT64_C(0x8001F7C0), 0x0840);
    static_assert(MemoryRegionPhysicalDramSecureDataStore.Contains(MemoryRegionPhysicalDramSecureDataStoreTzram));
    static_assert(MemoryRegionPhysicalDramSecureDataStore.Contains(MemoryRegionPhysicalDramSecureDataStoreWarmbootFirmware));
    static_assert(MemoryRegionPhysicalDramSecureDataStore.Contains(MemoryRegionPhysicalDramSecureDataStoreSecurityEngineState));

    constexpr inline const MemoryRegion MemoryRegionVirtualAtmosphereIramPage      = MemoryRegion(UINT64_C(0x1F01F0000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualAtmosphereIramPage));

    constexpr inline const MemoryRegion MemoryRegionVirtualAtmosphereUserPage      = MemoryRegion(UINT64_C(0x1F01F2000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualAtmosphereUserPage));

    constexpr inline const MemoryRegion MemoryRegionVirtualSmcUserPage             = MemoryRegion(UINT64_C(0x1F01F4000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualSmcUserPage));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramVolatileData       = MemoryRegion(UINT64_C(0x1F01F6000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramVolatileData      = MemoryRegion( UINT64_C(0x7C010000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualTzramVolatileData));
    static_assert(MemoryRegionPhysicalTzramVolatile.Contains(MemoryRegionPhysicalTzramVolatileData));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramVolatileStack      = MemoryRegion(UINT64_C(0x1F01F8000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramVolatileStack     = MemoryRegion( UINT64_C(0x7C011000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualTzramVolatileStack));
    static_assert(MemoryRegionPhysicalTzramVolatile.Contains(MemoryRegionPhysicalTzramVolatileStack));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramConfigurationData  = MemoryRegion(UINT64_C(0x1F01FA000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramConfigurationData = MemoryRegion( UINT64_C(0x7C01E000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualTzramConfigurationData));
    static_assert(MemoryRegionPhysicalTzramNonVolatile.Contains(MemoryRegionPhysicalTzramConfigurationData));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramL1PageTable  = MemoryRegion(UINT64_C(0x1F01FCFC0), 0x40);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramL1PageTable = MemoryRegion( UINT64_C(0x7C01EFC0), 0x40);
    static_assert(MemoryRegionPhysicalTzramConfigurationData.Contains(MemoryRegionPhysicalTzramL1PageTable));

    constexpr inline const MemoryRegion MemoryRegionVirtualTzramL2L3PageTable  = MemoryRegion(UINT64_C(0x1F01FE000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramL2L3PageTable = MemoryRegion( UINT64_C(0x7C01F000), 0x1000);
    static_assert(MemoryRegionVirtual.Contains(MemoryRegionVirtualTzramL2L3PageTable));
    static_assert(MemoryRegionPhysicalTzramNonVolatile.Contains(MemoryRegionPhysicalTzramL2L3PageTable));

    constexpr inline const MemoryRegion MemoryRegionPhysicalTzramFullProgramImage = MemoryRegion(UINT64_C(0x7C010800), 0xD800);
    constexpr inline const MemoryRegion MemoryRegionPhysicalIramBootCodeImage     = MemoryRegion(UINT64_C(0x40032000), 0x6000);

    constexpr inline const MemoryRegion MemoryRegionPhysicalIramBootCodeCode      = MemoryRegion(UINT64_C(0x40032000), 0x1000);
    constexpr inline const MemoryRegion MemoryRegionPhysicalIramBootCodeKeys      = MemoryRegion(UINT64_C(0x40033000), 0x1000);

    constexpr inline const MemoryRegion MemoryRegionPhysicalIramWarmbootBin = MemoryRegion(UINT64_C(0x4003E000), 0x17F0);
    constexpr inline const MemoryRegion MemoryRegionPhysicalIramBootConfig  = MemoryRegion(UINT64_C(0x4003F800), 0x400);

    constexpr inline const MemoryRegion MemoryRegionPhysicalIramRebootStub = MemoryRegion(UINT64_C(0x4003F000), 0x1000);

}
