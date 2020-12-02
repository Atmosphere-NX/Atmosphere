/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::lm {

    namespace detail {

        void LogManagerLogObserver(const diag::LogMetaData &log_metadata, const diag::LogBody &log_body, void *user_data);

    }

    namespace {

        bool g_initialized;

    }

    void Initialize() {
        AMS_ASSERT(!g_initialized);
        /* R_ABORT_UNLESS(lmInitialize()); */
        /* Log by default via LogManager. */
        diag::ReplaceDefaultLogObserver(detail::LogManagerLogObserver);
        g_initialized = true;
    }

    void Finalize() {
        AMS_ASSERT(g_initialized);
        /* lmExit(); */
        diag::ResetDefaultLogObserver();
        g_initialized = false;
    }

}
