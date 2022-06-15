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

#if defined(ATMOSPHERE_OS_HORIZON)

    namespace ams::svc {

        #if defined(ATMOSPHERE_ARCH_ARM64)

            namespace aarch64::lp64 {

                /* Convenience accessor. */
                ALWAYS_INLINE Result SetHeapSize(uintptr_t *out_address, ::ams::svc::Size size) {
                    static_assert(sizeof(::ams::svc::Address) == sizeof(uintptr_t));
                    return ::ams::svc::aarch64::lp64::SetHeapSize(reinterpret_cast<::ams::svc::Address *>(out_address), size);
                }

            }

        #endif

        ALWAYS_INLINE bool IsKernelMesosphere() {
            uint64_t dummy;
            return R_SUCCEEDED(::ams::svc::GetInfo(std::addressof(dummy), ::ams::svc::InfoType_MesosphereMeta, ::ams::svc::InvalidHandle, ::ams::svc::MesosphereMetaInfo_KernelVersion));
        }

        ALWAYS_INLINE bool IsKTraceEnabled() {
            uint64_t value = 0;
            return R_SUCCEEDED(::ams::svc::GetInfo(std::addressof(value), ::ams::svc::InfoType_MesosphereMeta, ::ams::svc::InvalidHandle, ::ams::svc::MesosphereMetaInfo_IsKTraceEnabled)) && value != 0;
        }

    }

#endif