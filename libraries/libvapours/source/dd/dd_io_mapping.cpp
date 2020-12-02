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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif

namespace ams::dd {

    uintptr_t QueryIoMapping(dd::PhysicalAddress phys_addr, size_t size) {
        #if defined(ATMOSPHERE_IS_EXOSPHERE)
            #if defined(ATMOSPHERE_ARCH_ARM64)
                /* TODO: Do this in a less shitty way. */
                if (secmon::MemoryRegionPhysicalDeviceClkRst.Contains(phys_addr, size)) {
                    return secmon::MemoryRegionVirtualDeviceClkRst.GetAddress() + phys_addr - secmon::MemoryRegionPhysicalDeviceClkRst.GetAddress();
                } else if (secmon::MemoryRegionPhysicalDeviceGpio.Contains(phys_addr, size)) {
                    return secmon::MemoryRegionVirtualDeviceGpio.GetAddress() + phys_addr - secmon::MemoryRegionPhysicalDeviceGpio.GetAddress();
                } else if (secmon::MemoryRegionPhysicalDeviceApbMisc.Contains(phys_addr, size)) {
                    return secmon::MemoryRegionVirtualDeviceApbMisc.GetAddress() + phys_addr - secmon::MemoryRegionPhysicalDeviceApbMisc.GetAddress();
                } else if (secmon::MemoryRegionPhysicalDeviceSdmmc.Contains(phys_addr, size)) {
                    return secmon::MemoryRegionVirtualDeviceSdmmc.GetAddress() + phys_addr - secmon::MemoryRegionPhysicalDeviceSdmmc.GetAddress();
                } else if (secmon::MemoryRegionPhysicalDevicePmc.Contains(phys_addr, size)) {
                    return secmon::MemoryRegionVirtualDevicePmc.GetAddress() + phys_addr - secmon::MemoryRegionPhysicalDevicePmc.GetAddress();
                } else if (secmon::MemoryRegionPhysicalDeviceI2c5.Contains(phys_addr, size)) {
                    return secmon::MemoryRegionVirtualDeviceI2c5.GetAddress() + phys_addr - secmon::MemoryRegionPhysicalDeviceI2c5.GetAddress();
                } else if (secmon::MemoryRegionPhysicalDeviceI2c1.Contains(phys_addr, size)) {
                    return secmon::MemoryRegionVirtualDeviceI2c1.GetAddress() + phys_addr - secmon::MemoryRegionPhysicalDeviceI2c1.GetAddress();
                } else {
                    AMS_UNUSED(size);
                    return static_cast<uintptr_t>(phys_addr);
                }

            #elif defined(ATMOSPHERE_ARCH_ARM)
                /* TODO: BPMP translation? */
                AMS_UNUSED(size);
                return static_cast<uintptr_t>(phys_addr);
            #else
                #error "Unknown architecture for ams::dd::QueryIoMapping (EXOSPHERE)!"
            #endif
        #elif defined(ATMOSPHERE_IS_MESOSPHERE)
            /* TODO: Kernel address translation? */
            AMS_UNUSED(size);
            return static_cast<uintptr_t>(phys_addr);
        #elif defined(ATMOSPHERE_IS_STRATOSPHERE)

            svc::Address virt_addr = 0;
            const dd::PhysicalAddress aligned_addr = util::AlignDown(phys_addr, os::MemoryPageSize);
            const size_t offset                    = phys_addr - aligned_addr;
            const size_t aligned_size              = size + offset;

            if (hos::GetVersion() >= hos::Version_10_0_0) {
                svc::Size region_size = 0;
                R_TRY_CATCH(svc::QueryIoMapping(&virt_addr, &region_size, aligned_addr, aligned_size)) {
                    /* Official software handles this by returning 0. */
                    R_CATCH(svc::ResultNotFound) { return 0; }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
                AMS_ASSERT(region_size >= aligned_size);
            } else {
                R_TRY_CATCH(svc::LegacyQueryIoMapping(&virt_addr, aligned_addr, aligned_size)) {
                    /* Official software handles this by returning 0. */
                    R_CATCH(svc::ResultNotFound) { return 0; }
                } R_END_TRY_CATCH_WITH_ABORT_UNLESS;
            }

            return static_cast<uintptr_t>(virt_addr) + offset;

        #else
            #error "Unknown execution context for ams::dd::QueryIoMapping!"
        #endif
    }

    namespace {

        #if defined(ATMOSPHERE_IS_STRATOSPHERE)

            #if defined(ATMOSPHERE_BOARD_NINTENDO_NX)

                constexpr dd::PhysicalAddress PmcPhysStart = 0x7000E400;
                constexpr dd::PhysicalAddress PmcPhysLast  = 0x7000EFFF;

                constexpr bool IsValidPmcPhysicalAddress(dd::PhysicalAddress phys_addr) {
                    return util::IsAligned(phys_addr, alignof(u32)) && PmcPhysStart <= phys_addr && phys_addr <= PmcPhysLast;
                }

                u32 ReadWritePmcRegisterImpl(dd::PhysicalAddress phys_addr, u32 value, u32 mask) {
                    u32 out_value;
                    R_ABORT_UNLESS(spl::smc::ConvertResult(spl::smc::AtmosphereReadWriteRegister(phys_addr, mask, value, &out_value)));
                    return out_value;
                }

                bool TryReadModifyWritePmcRegister(u32 *out, dd::PhysicalAddress phys_addr, u32 value, u32 mask) {
                    if (IsValidPmcPhysicalAddress(phys_addr)) {
                        *out = ReadWritePmcRegisterImpl(phys_addr, value, mask);
                        return true;
                    } else {
                        return false;
                    }
                }

            #else

                bool TryReadModifyWritePmcRegister(u32 *out, dd::PhysicalAddress phys_addr, u32 value, u32 mask) {
                    AMS_UNUSED(out, phys_addr, value, mask);
                    return false;
                }

            #endif

        #endif
    }

    u32 ReadIoRegister(dd::PhysicalAddress phys_addr) {
        #if defined(ATMOSPHERE_IS_EXOSPHERE) || defined(ATMOSPHERE_IS_MESOSPHERE)
            return reg::Read(dd::QueryIoMapping(phys_addr, sizeof(u32)));
        #elif defined(ATMOSPHERE_IS_STRATOSPHERE)

            u32 val;
            if (!TryReadModifyWritePmcRegister(std::addressof(val), phys_addr, 0, 0)) {
                R_ABORT_UNLESS(svc::ReadWriteRegister(std::addressof(val), phys_addr, 0, 0));
            }
            return val;

        #else
            #error "Unknown execution context for ams::dd::ReadIoRegister!"
        #endif
    }

    void WriteIoRegister(dd::PhysicalAddress phys_addr, u32 value) {
        #if defined(ATMOSPHERE_IS_EXOSPHERE) || defined(ATMOSPHERE_IS_MESOSPHERE)
            reg::Write(dd::QueryIoMapping(phys_addr, sizeof(u32)), value);
        #elif defined(ATMOSPHERE_IS_STRATOSPHERE)

            u32 out_val;
            if (!TryReadModifyWritePmcRegister(std::addressof(out_val), phys_addr, value, 0xFFFFFFFF)) {
                R_ABORT_UNLESS(svc::ReadWriteRegister(std::addressof(out_val), phys_addr, 0xFFFFFFFF, value));
            }
            AMS_UNUSED(out_val);

        #else
            #error "Unknown execution context for ams::dd::WriteIoRegister!"
        #endif
    }

    u32 ReadModifyWriteIoRegister(PhysicalAddress phys_addr, u32 value, u32 mask) {
        #if defined(ATMOSPHERE_IS_EXOSPHERE) || defined(ATMOSPHERE_IS_MESOSPHERE)
            AMS_UNUSED(phys_addr, value, mask);
            AMS_ABORT("ReadModifyWriteIoRegister TODO under non-stratosphere");
        #elif defined(ATMOSPHERE_IS_STRATOSPHERE)

            u32 out_val;
            if (!TryReadModifyWritePmcRegister(std::addressof(out_val), phys_addr, value, mask)) {
                R_ABORT_UNLESS(svc::ReadWriteRegister(std::addressof(out_val), phys_addr, mask, value));
            }
            return out_val;

        #else
            #error "Unknown execution context for ams::dd::ReadModifyWriteIoRegister!"
        #endif
    }

}
