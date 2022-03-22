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

namespace ams::os {

    namespace {

        struct CommandLineParameter {
            int argc;
            char **argv;
        };

        constinit const char *g_command_line_parameter_argv[2] = { "", nullptr };
        constinit CommandLineParameter g_command_line_parameter = {
            1,
            const_cast<char **>(g_command_line_parameter_argv),
        };

    }

    void SetHostArgc(int argc) {
        g_command_line_parameter.argc = argc;
    }

    void SetHostArgv(char **argv) {
        g_command_line_parameter.argv = argv;
    }

    int GetHostArgc() {
        return g_command_line_parameter.argc;
    }

    char **GetHostArgv() {
        return g_command_line_parameter.argv;
    }

}
