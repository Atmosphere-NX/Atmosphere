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

namespace ams::ldr {

    /* Utility reference to make code mounting automatic. */
    class ScopedCodeMount {
        NON_COPYABLE(ScopedCodeMount);
        private:
            Result result;
            bool is_code_mounted;
            bool is_hbl_mounted;
        public:
            ScopedCodeMount(const ncm::ProgramLocation &loc);
            ~ScopedCodeMount();

            Result GetResult() const {
                return this->result;
            }

            bool IsCodeMounted() const {
                return this->is_code_mounted;
            }

            bool IsHblMounted() const {
                return this->is_hbl_mounted;
            }

        private:
            Result Initialize(const ncm::ProgramLocation &loc);

            Result MountCodeFileSystem(const ncm::ProgramLocation &loc);
            Result MountSdCardCodeFileSystem(const ncm::ProgramLocation &loc);
            Result MountHblFileSystem();
    };

    /* Content Management API. */
    Result OpenCodeFile(FILE *&out, ncm::ProgramId program_id, const char *relative_path);
    Result OpenCodeFileFromBaseExefs(FILE *&out, ncm::ProgramId program_id, const char *relative_path);

    /* Redirection API. */
    Result ResolveContentPath(char *out_path, const ncm::ProgramLocation &loc);
    Result RedirectContentPath(const char *path, const ncm::ProgramLocation &loc);
    Result RedirectHtmlDocumentPathForHbl(const ncm::ProgramLocation &loc);

}
