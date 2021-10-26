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
            static inline constinit bool s_is_debug_mode;
            static inline constinit bool s_enable_debug_logging;
            static inline constinit bool s_enable_user_exception_handlers;
            static inline constinit bool s_enable_debug_memory_fill;
            static inline constinit bool s_enable_user_pmu_access;
            static inline constinit bool s_enable_kernel_debugging;
            static inline constinit bool s_enable_dynamic_resource_limits;
        private:
            static ALWAYS_INLINE void SetIsDebugMode(bool en) { s_is_debug_mode = en; }
            static ALWAYS_INLINE void EnableDebugLogging(bool en) { s_enable_debug_logging = en; }
            static ALWAYS_INLINE void EnableUserExceptionHandlers(bool en) { s_enable_user_exception_handlers = en; }
            static ALWAYS_INLINE void EnableDebugMemoryFill(bool en) { s_enable_debug_memory_fill = en; }
            static ALWAYS_INLINE void EnableUserPmuAccess(bool en) { s_enable_user_pmu_access = en; }
            static ALWAYS_INLINE void EnableKernelDebugging(bool en) { s_enable_kernel_debugging = en; }
            static ALWAYS_INLINE void EnableDynamicResourceLimits(bool en) { s_enable_dynamic_resource_limits = en; }
        public:
            static ALWAYS_INLINE bool IsDebugMode() { return s_is_debug_mode; }
            static ALWAYS_INLINE bool IsDebugLoggingEnabled() { return s_enable_debug_logging; }
            static ALWAYS_INLINE bool IsUserExceptionHandlersEnabled() { return s_enable_user_exception_handlers; }
            static ALWAYS_INLINE bool IsDebugMemoryFillEnabled() { return s_enable_debug_memory_fill; }
            static ALWAYS_INLINE bool IsUserPmuAccessEnabled() { return s_enable_user_pmu_access; }
            static ALWAYS_INLINE bool IsKernelDebuggingEnabled() { return s_enable_kernel_debugging; }
            static ALWAYS_INLINE bool IsDynamicResourceLimitsEnabled() { return s_enable_dynamic_resource_limits; }
    };

}