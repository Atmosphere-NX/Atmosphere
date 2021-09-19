/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
            ContentId content_id;
            Path path;
        public:
            RegisteredPath(const ncm::ContentId &content_id, const Path &p) : content_id(content_id), path(p) {
                /* ... */
            }

            ncm::ContentId GetContentId() const {
                return this->content_id;
            }

            void GetPath(Path *out) const {
                *out = this->path;
            }

            void SetPath(const Path &path) {
                this->path = path;
            }
    };

    RegisteredHostContent::~RegisteredHostContent() {
        /* ... */
    }

    Result RegisteredHostContent::RegisterPath(const ncm::ContentId &content_id, const ncm::Path &path) {
        std::scoped_lock lk(this->mutex);

        /* Replace the path of any existing entries. */
        for (auto &registered_path : this->path_list) {
            if (registered_path.GetContentId() == content_id) {
                registered_path.SetPath(path);
                return ResultSuccess();
            }
        }

        /* Allocate a new registered path. TODO: Verify allocator. */
        RegisteredPath *registered_path = new RegisteredPath(content_id, path);
        R_UNLESS(registered_path != nullptr, ncm::ResultBufferInsufficient());

        /* Insert the path into the list. */
        this->path_list.push_back(*registered_path);
        return ResultSuccess();
    }

    Result RegisteredHostContent::GetPath(Path *out, const ncm::ContentId &content_id) {
        std::scoped_lock lk(this->mutex);

        /* Obtain the path of the content. */
        for (const auto &registered_path : this->path_list) {
            if (registered_path.GetContentId() == content_id) {
                registered_path.GetPath(out);
                return ResultSuccess();
            }
        }
        return ncm::ResultContentNotFound();
    }

    void RegisteredHostContent::ClearPaths() {
        /* Remove all paths. */
        for (auto it = this->path_list.begin(); it != this->path_list.end(); /* ... */) {
            auto *obj = std::addressof(*it);
            it = this->path_list.erase(it);
            delete obj;
        }
    }

}
