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
#include <stratosphere.hpp>

namespace ams::ldr {

    class LoaderService final {
        public:
            /* Official commands. */
            Result CreateProcess(sf::OutMoveHandle proc_h, PinId id, u32 flags, sf::CopyHandle reslimit_h);
            Result GetProgramInfo(sf::Out<ProgramInfo> out_program_info, const ncm::ProgramLocation &loc);
            Result PinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc);
            Result UnpinProgram(PinId id);
            Result SetProgramArgumentsDeprecated(ncm::ProgramId program_id, const sf::InPointerBuffer &args, u32 args_size);
            Result SetProgramArguments(ncm::ProgramId program_id, const sf::InPointerBuffer &args);
            Result FlushArguments();
            Result GetProcessModuleInfo(sf::Out<u32> count, const sf::OutPointerArray<ModuleInfo> &out, os::ProcessId process_id);
            Result SetEnabledProgramVerification(bool enabled);

            /* Atmosphere commands. */
            Result AtmosphereRegisterExternalCode(sf::OutMoveHandle out, ncm::ProgramId program_id);
            void   AtmosphereUnregisterExternalCode(ncm::ProgramId program_id);
            void   AtmosphereHasLaunchedProgram(sf::Out<bool> out, ncm::ProgramId program_id);
            Result AtmosphereGetProgramInfo(sf::Out<ProgramInfo> out_program_info, sf::Out<cfg::OverrideStatus> out_status, const ncm::ProgramLocation &loc);
            Result AtmospherePinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status);
    };
    static_assert(ams::ldr::impl::IsIProcessManagerInterface<LoaderService>);
    static_assert(ams::ldr::impl::IsIDebugMonitorInterface<LoaderService>);
    static_assert(ams::ldr::impl::IsIShellInterface<LoaderService>);

}
