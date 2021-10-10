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
#include <stratosphere/os.hpp>
#include <stratosphere/ncm/ncm_content_id.hpp>
#include <stratosphere/ncm/ncm_path.hpp>

namespace ams::ncm {

    class RegisteredHostContent {
        NON_COPYABLE(RegisteredHostContent);
        NON_MOVEABLE(RegisteredHostContent);
        private:
            class RegisteredPath;
        private:
            using RegisteredPathList = ams::util::IntrusiveListBaseTraits<RegisteredPath>::ListType;
        private:
            os::SdkMutex m_mutex;
            RegisteredPathList m_path_list;
        public:
            RegisteredHostContent() : m_mutex(), m_path_list() { /* ... */ }
            ~RegisteredHostContent();

            Result RegisterPath(const ncm::ContentId &content_id, const ncm::Path &path);
            Result GetPath(Path *out, const ncm::ContentId &content_id);
            void ClearPaths();
    };

}
