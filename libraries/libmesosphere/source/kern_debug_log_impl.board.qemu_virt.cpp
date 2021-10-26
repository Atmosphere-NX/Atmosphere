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
#include <mesosphere.hpp>
#include "kern_debug_log_impl.hpp"

namespace ams::kern {

#if defined(MESOSPHERE_DEBUG_LOG_USE_SEMIHOSTING)

    bool KDebugLogImpl::Initialize() {
        return true;
    }

    void KDebugLogImpl::PutChar(char c) {
        /* TODO */
        AMS_UNUSED(c);
    }

    void KDebugLogImpl::Flush() {
        /* ... */
    }

    void KDebugLogImpl::Save() {
        /* ... */
    }

    void KDebugLogImpl::Restore() {
        /* ... */
    }

#else

    #error "Unknown Debug device!"

#endif

}
