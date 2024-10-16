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
#pragma once
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_select_system_control.hpp>

namespace ams::kern {

    class KTargetSystem {
        private:
            friend class KSystemControlBase;
            friend class KSystemControl;
        private:
            struct KTargetSystemData {
                bool is_debug_mode;
                bool enable_debug_logging;
                bool enable_user_exception_handlers;
                bool enable_debug_memory_fill;
                bool enable_user_pmu_access;
                bool enable_kernel_debugging;
                bool enable_dynamic_resource_limits;
            };
        private:
            static inline constinit bool s_is_initialized = false;
            static inline constinit const volatile KTargetSystemData s_data = {
                .is_debug_mode                  = true,
                .enable_debug_logging           = true,
                .enable_user_exception_handlers = true,
                .enable_debug_memory_fill       = true,
                .enable_user_pmu_access         = true,
                .enable_kernel_debugging        = true,
                .enable_dynamic_resource_limits = false,
            };
        private:
            static ALWAYS_INLINE void SetInitialized() { s_is_initialized = true; }
        public:
            static ALWAYS_INLINE bool IsDebugMode() { return s_is_initialized && s_data.is_debug_mode; }
            static ALWAYS_INLINE bool IsDebugLoggingEnabled() { return s_is_initialized && s_data.enable_debug_logging; }
            static ALWAYS_INLINE bool IsUserExceptionHandlersEnabled() { return s_is_initialized && s_data.enable_user_exception_handlers; }
            static ALWAYS_INLINE bool IsDebugMemoryFillEnabled() { return s_is_initialized && s_data.enable_debug_memory_fill; }
            static ALWAYS_INLINE bool IsUserPmuAccessEnabled() { return s_is_initialized && s_data.enable_user_pmu_access; }
            static ALWAYS_INLINE bool IsKernelDebuggingEnabled() { return s_is_initialized && s_data.enable_kernel_debugging; }
            static ALWAYS_INLINE bool IsDynamicResourceLimitsEnabled() { return s_is_initialized && s_data.enable_dynamic_resource_limits; }
    };

}