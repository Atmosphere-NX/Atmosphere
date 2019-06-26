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
#include <switch.h>
#include <stratosphere.hpp>
#include <stratosphere/ldr.hpp>

namespace sts::ldr {

    /* Utility reference to make code mounting automatic. */
    class ScopedCodeMount {
        NON_COPYABLE(ScopedCodeMount);
        private:
            bool is_code_mounted;
            bool is_hbl_mounted;
        public:
            ScopedCodeMount() : is_code_mounted(false), is_hbl_mounted(false) { /* ... */ }
            ScopedCodeMount(bool c, bool h) : is_code_mounted(c), is_hbl_mounted(h) { /* ... */ }
            ~ScopedCodeMount();

            ScopedCodeMount(ScopedCodeMount&& rhs) {
                this->is_code_mounted = rhs.is_code_mounted;
                this->is_hbl_mounted = rhs.is_hbl_mounted;
                rhs.is_code_mounted = false;
                rhs.is_hbl_mounted = false;
            }

            ScopedCodeMount& operator=(ScopedCodeMount&& rhs) {
                rhs.Swap(*this);
                return *this;
            }

            void Swap(ScopedCodeMount& rhs) {
                std::swap(this->is_code_mounted, rhs.is_code_mounted);
                std::swap(this->is_hbl_mounted, rhs.is_hbl_mounted);
            }

            void SetCodeMounted() {
                this->is_code_mounted = true;
            }

            void SetHblMounted() {
                this->is_hbl_mounted = true;
            }

            bool IsCodeMounted() const {
                return this->is_code_mounted;
            }

            bool IsHblMounted() const {
                return this->is_hbl_mounted;
            }
    };

    /* Content Management API. */
    Result MountCode(ScopedCodeMount &out, const ncm::TitleLocation &loc);
    Result OpenCodeFile(FILE *&out, ncm::TitleId title_id, const char *relative_path);
    Result OpenCodeFileFromBaseExefs(FILE *&out, ncm::TitleId title_id, const char *relative_path);

    /* Redirection API. */
    Result ResolveContentPath(char *out_path, const ncm::TitleLocation &loc);
    Result RedirectContentPath(const char *path, const ncm::TitleLocation &loc);
    Result RedirectHtmlDocumentPathForHbl(const ncm::TitleLocation &loc);

}
