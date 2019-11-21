/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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
#include <stratosphere.hpp>

namespace ams::mitm::bpc {

    class BpcMitmService  : public sf::IMitmServiceObject {
        private:
            enum class CommandId {
                ShutdownSystem = 0,
                RebootSystem   = 1,
            };
        public:
            static bool ShouldMitm(os::ProcessId process_id, ncm::ProgramId program_id) {
                /* We will mitm:
                 * - am, to intercept the Reboot/Power buttons in the overlay menu.
                 * - fatal, to simplify payload reboot logic significantly
                 * - applications and hbl, to allow homebrew to take advantage of the feature.
                 */
                return program_id == ncm::ProgramId::Am ||
                       program_id == ncm::ProgramId::Fatal ||
                       ncm::IsApplicationProgramId(program_id);
                       /* TODO: Hbl */
            }
        public:
            SF_MITM_SERVICE_OBJECT_CTOR(BpcMitmService) { /* ... */ }
        protected:
            /* Overridden commands. */
            Result ShutdownSystem();
            Result RebootSystem();
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(ShutdownSystem),
                MAKE_SERVICE_COMMAND_META(RebootSystem),
            };
    };

}
