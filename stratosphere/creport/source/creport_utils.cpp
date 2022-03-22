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
#include "creport_utils.hpp"

namespace ams::creport {

    os::ProcessId ParseProcessIdArgument(const char *s) {
        /* Official creport uses this custom parsing logic... */
        u64 out_val = 0;

        for (unsigned int i = 0; i < 20 && s[i]; i++) {
            if ('0' <= s[i] && s[i] <= '9') {
                out_val *= 10;
                out_val += (s[i] - '0');
            } else {
                break;
            }
        }

        return os::ProcessId{out_val};
    }

}