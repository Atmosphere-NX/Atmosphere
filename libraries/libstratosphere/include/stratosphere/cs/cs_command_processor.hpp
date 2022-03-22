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
#include <vapours.hpp>
#include <stratosphere/scs/scs_command_processor.hpp>
#include <stratosphere/vi/vi_layer_stack.hpp>

namespace ams::cs {

    using CommandHeader  = scs::CommandHeader;
    using ResponseHeader = scs::ResponseHeader;

    class CommandProcessor : public scs::CommandProcessor {
        public:
            virtual bool ProcessCommand(const CommandHeader &header, const u8 *body, s32 socket) override;
        private:
            void TakeScreenShot(const CommandHeader &header, s32 socket, vi::LayerStack layer_stack);
        private:
            static void SendFirmwareVersion(s32 socket, const CommandHeader &header);
    };

}
