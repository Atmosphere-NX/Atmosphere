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
#include <stratosphere.hpp>

namespace ams::cfg {

    /* Flag utilities. */
    bool HasFlag(const sm::MitmProcessInfo &process_info, const char *flag) {
        AMS_UNUSED(process_info, flag);
        return false;
    }

    bool HasContentSpecificFlag(ncm::ProgramId program_id, const char *flag) {
        AMS_UNUSED(program_id, flag);
        return false;
    }

    bool HasGlobalFlag(const char *flag) {
        AMS_UNUSED(flag);
        return false;
    }

    bool HasHblFlag(const char *flag) {
        AMS_UNUSED(flag);
        return false;
    }

    Result DeleteGlobalFlag(const char *flag) {
        AMS_UNUSED(flag);
        return false;
    }

}
