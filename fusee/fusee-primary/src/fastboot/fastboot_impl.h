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

#include<stddef.h>

namespace ams {

    namespace fastboot {
    
        class FastbootGadget;
    
        class FastbootImpl {
            public:
            FastbootImpl(FastbootGadget &gadget);

            /* Implementation is allowed to modify the command buffer for string
               parsing reasons. */
            Result ProcessCommand(char *command_buffer);
            Result Continue();

            private:
            FastbootGadget &gadget;

            enum class Action {
                Invalid,
            } current_action;
        
            union {
                /* Actions */
            };

            /* Commands */
            Result GetVar(const char *arg);
            Result Download(const char *arg);
            Result Flash(const char *arg);
            Result Reboot(const char *arg);
            Result Boot(const char *arg);
            Result OemCRC32(const char *arg);

            /* Continuations */
        };
        
    } // namespace fastboot

} // namespace ams
