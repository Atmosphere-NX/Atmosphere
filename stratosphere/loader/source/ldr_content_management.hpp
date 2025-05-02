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

    /* Utility reference to make code mounting automatic. */
    class ScopedCodeMount {
        NON_COPYABLE(ScopedCodeMount);
        NON_MOVEABLE(ScopedCodeMount);
        private:
            std::scoped_lock<os::SdkMutex> m_lk;
            cfg::OverrideStatus m_override_status;
            fs::CodeVerificationData m_ams_code_verification_data;
            fs::CodeVerificationData m_sd_or_base_code_verification_data;
            fs::CodeVerificationData m_base_code_verification_data;
            Result m_result;
            bool m_has_status;
            bool m_mounted_ams;
            bool m_mounted_sd_or_code;
            bool m_mounted_code;
        public:
            ScopedCodeMount(const ncm::ProgramLocation &loc, const ldr::ProgramAttributes &attrs);
            ScopedCodeMount(const ncm::ProgramLocation &loc, const cfg::OverrideStatus &override_status, const ldr::ProgramAttributes &attrs);
            ~ScopedCodeMount();

            Result GetResult() const {
                return m_result;
            }

            const cfg::OverrideStatus &GetOverrideStatus() const {
                AMS_ABORT_UNLESS(m_has_status);
                return m_override_status;
            }

            const fs::CodeVerificationData &GetAtmosphereCodeVerificationData() const {
                return m_ams_code_verification_data;
            }

            const fs::CodeVerificationData &GetSdOrBaseCodeVerificationData() const {
                return m_sd_or_base_code_verification_data;
            }

            const fs::CodeVerificationData &GetCodeVerificationData() const {
                return m_base_code_verification_data;
            }
        private:
            Result Initialize(const ncm::ProgramLocation &loc, const ldr::ProgramAttributes &attrs);
            void EnsureOverrideStatus(const ncm::ProgramLocation &loc);
    };

    constexpr inline const char * const AtmosphereCodeMountName   = "ams-code";
    constexpr inline const char * const AtmosphereCompatMountName = "ams-cmpt";
    constexpr inline const char * const SdOrCodeMountName         = "sd-code";
    constexpr inline const char * const CodeMountName             = "code";
    constexpr inline const char * const CompatMountName           = "cmpt";

    #define ENCODE_ATMOSPHERE_CODE_PATH(relative) "ams-code:" relative
    #define ENCODE_ATMOSPHERE_CMPT_PATH(relative) "ams-cmpt:" relative
    #define ENCODE_SD_OR_CODE_PATH(relative) "sd-code:" relative
    #define ENCODE_CODE_PATH(relative) "code:" relative
    #define ENCODE_CMPT_PATH(relative) "cmpt:" relative

    /* Redirection API. */
    Result GetProgramPath(char *out_path, size_t out_size, const ncm::ProgramLocation &loc, const ldr::ProgramAttributes &attrs);
    Result RedirectProgramPath(const char *path, size_t size, const ncm::ProgramLocation &loc);
    Result RedirectHtmlDocumentPathForHbl(const ncm::ProgramLocation &loc);

}
