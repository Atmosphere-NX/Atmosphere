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
#include <stratosphere.hpp>

namespace ams::ldr {

    class LoaderService {
        public:
            /* Official commands. */
            Result CreateProcess(sf::OutMoveHandle proc_h, PinId id, u32 flags, sf::CopyHandle &&reslimit_h, const ProgramAttributes &attrs) {
                /* Create a handle to set the output to when done. */
                os::NativeHandle handle = os::InvalidNativeHandle;
                ON_SCOPE_EXIT { proc_h.SetValue(handle, true); };

                R_RETURN(this->CreateProcess(std::addressof(handle), id, flags, reslimit_h.GetOsHandle(), attrs));
            }

            Result GetProgramInfo(sf::Out<ProgramInfo> out_program_info, const ncm::ProgramLocation &loc, const ProgramAttributes &attrs) {
                R_RETURN(this->GetProgramInfo(out_program_info.GetPointer(), nullptr, loc, attrs));
            }

            Result PinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc) {
                R_RETURN(this->PinProgram(out_id.GetPointer(), loc, cfg::OverrideStatus{}));
            }

            Result UnpinProgram(PinId id);

            Result SetProgramArgumentDeprecated(ncm::ProgramId program_id, const sf::InPointerBuffer &args, u32 args_size) {
                AMS_UNUSED(args_size);
                R_RETURN(this->SetProgramArgument(program_id, args.GetPointer(), std::min<size_t>(args_size, args.GetSize())));
            }

            Result SetProgramArgument(ncm::ProgramId program_id, const sf::InPointerBuffer &args) {
                R_RETURN(this->SetProgramArgument(program_id, args.GetPointer(), args.GetSize()));
            }

            Result FlushArguments();

            Result GetProcessModuleInfo(sf::Out<u32> count, const sf::OutPointerArray<ModuleInfo> &out, os::ProcessId process_id) {
                R_UNLESS(out.GetSize() <= std::numeric_limits<s32>::max(), ldr::ResultInvalidSize());

                R_RETURN(this->GetProcessModuleInfo(count.GetPointer(), out.GetPointer(), out.GetSize(), process_id));
            }

            Result SetEnabledProgramVerification(bool enabled);

            /* Atmosphere commands. */
            Result AtmosphereRegisterExternalCode(sf::OutMoveHandle out, ncm::ProgramId program_id) {
                /* Create a handle to set the output to when done. */
                os::NativeHandle handle = os::InvalidNativeHandle;
                ON_SCOPE_EXIT { out.SetValue(handle, true); };

                R_RETURN(this->RegisterExternalCode(std::addressof(handle), program_id));
            }

            void AtmosphereUnregisterExternalCode(ncm::ProgramId program_id) {
                return this->UnregisterExternalCode(program_id);
            }

            void AtmosphereHasLaunchedBootProgram(sf::Out<bool> out, ncm::ProgramId program_id) {
                return this->HasLaunchedBootProgram(out.GetPointer(), program_id);
            }

            Result AtmosphereGetProgramInfo(sf::Out<ProgramInfo> out_program_info, sf::Out<cfg::OverrideStatus> out_status, const ncm::ProgramLocation &loc, const ProgramAttributes &attrs) {
                R_RETURN(this->GetProgramInfo(out_program_info.GetPointer(), out_status.GetPointer(), loc, attrs));
            }

            Result AtmospherePinProgram(sf::Out<PinId> out_id, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status) {
                R_RETURN(this->PinProgram(out_id.GetPointer(), loc, override_status));
            }
        private:
            Result CreateProcess(os::NativeHandle *out, PinId pin_id, u32 flags, os::NativeHandle resource_limit, const ProgramAttributes &attrs);
            Result GetProgramInfo(ProgramInfo *out, cfg::OverrideStatus *out_status, const ncm::ProgramLocation &loc, const ProgramAttributes &attrs);
            Result PinProgram(PinId *out, const ncm::ProgramLocation &loc, const cfg::OverrideStatus &status);
            Result SetProgramArgument(ncm::ProgramId program_id, const void *argument, size_t size);
            Result GetProcessModuleInfo(u32 *out_count, ModuleInfo *out, size_t max_out_count, os::ProcessId process_id);
            Result RegisterExternalCode(os::NativeHandle *out, ncm::ProgramId program_id);
            void   UnregisterExternalCode(ncm::ProgramId program_id);
            void   HasLaunchedBootProgram(bool *out, ncm::ProgramId program_id);
    };
    static_assert(ams::ldr::impl::IsIProcessManagerInterface<LoaderService>);
    static_assert(ams::ldr::impl::IsIDebugMonitorInterface<LoaderService>);
    static_assert(ams::ldr::impl::IsIShellInterface<LoaderService>);

}
