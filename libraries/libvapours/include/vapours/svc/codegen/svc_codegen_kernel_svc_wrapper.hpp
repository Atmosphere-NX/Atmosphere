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
#include <vapours/svc/codegen/impl/svc_codegen_impl_kernel_svc_wrapper.hpp>

namespace ams::svc::codegen {

#if defined(ATMOSPHERE_ARCH_ARM64) || defined(ATMOSPHERE_ARCH_ARM)

    template<auto &Function64, auto &Function64From32>
    class KernelSvcWrapper {
        private:
            /* TODO: using Aarch32 = */
            using Aarch64       = impl::KernelSvcWrapperHelper<impl::Aarch64SvcInvokeAbi, impl::Aarch64Lp64Abi, impl::Aarch64Lp64Abi, Function64>;
            using Aarch64From32 = impl::KernelSvcWrapperHelper<impl::Aarch32SvcInvokeAbi, impl::Aarch32Ilp32Abi, impl::Aarch64Lp64Abi, Function64From32>;
        public:
/* Set omit-frame-pointer to prevent GCC from emitting MOV X29, SP instructions. */
#pragma GCC push_options
#pragma GCC optimize ("-O2")
#pragma GCC optimize ("omit-frame-pointer")

            static ALWAYS_INLINE void Call64() {
                if constexpr (std::is_same<typename Aarch64::ReturnType, void>::value) {
                    Aarch64::WrapSvcFunction();
                } else {
                    const auto &res = Aarch64::WrapSvcFunction();
                    __asm__ __volatile__("" :: [res]"r"(res));
                }

            }

            static ALWAYS_INLINE void Call64From32() {
                if constexpr (std::is_same<typename Aarch64::ReturnType, void>::value) {
                    Aarch64From32::WrapSvcFunction();
                } else {
                    const auto &res = Aarch64From32::WrapSvcFunction();
                    __asm__ __volatile__("" :: [res]"r"(res));
                }
            }

#pragma GCC pop_options
    };

#else

    #error "Unknown architecture for Kernel SVC Code Generation"

#endif

}