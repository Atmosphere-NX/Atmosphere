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

namespace ams::ncm {

    class RegisteredHostContent::RegisteredPath : public util::IntrusiveListBaseNode<RegisteredPath> {
        NON_COPYABLE(RegisteredPath);
        NON_MOVEABLE(RegisteredPath);
        private:
            ContentId m_content_id;
            Path m_path;
        public:
            RegisteredPath(const ncm::ContentId &content_id, const Path &p) : m_content_id(content_id), m_path(p) {
                /* ... */
            }

            ncm::ContentId GetContentId() const {
                return m_content_id;
            }

            void GetPath(Path *out) const {
                *out = m_path;
            }

            void SetPath(const Path &path) {
                m_path = path;
            }
    };

    RegisteredHostContent::~RegisteredHostContent() {
        /* ... */
    }

    Result RegisteredHostContent::RegisterPath(const ncm::ContentId &content_id, const ncm::Path &path) {
        std::scoped_lock lk(m_mutex);

        /* Replace the path of any existing entries. */
        for (auto &registered_path : m_path_list) {
            if (registered_path.GetContentId() == content_id) {
                registered_path.SetPath(path);
                return ResultSuccess();
            }
        }

        /* Allocate a new registered path. TODO: Verify allocator. */
        RegisteredPath *registered_path = new RegisteredPath(content_id, path);
        R_UNLESS(registered_path != nullptr, ncm::ResultBufferInsufficient());

        /* Insert the path into the list. */
        m_path_list.push_back(*registered_path);
        return ResultSuccess();
    }

    Result RegisteredHostContent::GetPath(Path *out, const ncm::ContentId &content_id) {
        std::scoped_lock lk(m_mutex);

        /* Obtain the path of the content. */
        for (const auto &registered_path : m_path_list) {
            if (registered_path.GetContentId() == content_id) {
                registered_path.GetPath(out);
                return ResultSuccess();
            }
        }
        return ncm::ResultContentNotFound();
    }

    void RegisteredHostContent::ClearPaths() {
        /* Remove all paths. */
        for (auto it = m_path_list.begin(); it != m_path_list.end(); /* ... */) {
            auto *obj = std::addressof(*it);
            it = m_path_list.erase(it);
            delete obj;
        }
    }

}
