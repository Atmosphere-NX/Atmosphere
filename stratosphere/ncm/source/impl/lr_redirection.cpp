/*
 * Copyright (c) 2019 Adubbz
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

#include "lr_redirection.hpp"

namespace sts::lr::impl {

    class LocationRedirection  : public util::IntrusiveListBaseNode<LocationRedirection> {
        NON_COPYABLE(LocationRedirection);
        NON_MOVEABLE(LocationRedirection);
        private:
            ncm::TitleId title_id;
            Path path;
            u32 flags;
        public:
            LocationRedirection(ncm::TitleId title_id, const Path& path, u32 flags) :
                title_id(title_id), path(path), flags(flags) { /* ... */ }

            ncm::TitleId GetTitleId() const {
                return this->title_id;
            }

            void GetPath(Path *out) const {
                *out = this->path;
            }

            u32 GetFlags() const {
                return this->flags;
            }

            void SetFlags(u32 flags) {
                this->flags = flags;
            }
    };

    bool LocationRedirector::FindRedirection(Path *out, ncm::TitleId title_id) {
        if (this->redirection_list.empty()) {
            return false;
        }

        for (const auto &redirection : this->redirection_list) {
            if (redirection.GetTitleId() == title_id) {
                redirection.GetPath(out);
                return true;
            }
        }
        return false;
    }

    void LocationRedirector::SetRedirection(ncm::TitleId title_id, const Path &path, u32 flags) {
        this->EraseRedirection(title_id);
        this->redirection_list.push_back(*(new LocationRedirection(title_id, path, flags)));
    }

    void LocationRedirector::SetRedirectionFlags(ncm::TitleId title_id, u32 flags) {
        for (auto &redirection : this->redirection_list) {
            if (redirection.GetTitleId() == title_id) {
                redirection.SetFlags(flags);
                break;
            }
        }
    }

    void LocationRedirector::EraseRedirection(ncm::TitleId title_id) {
        for (auto &redirection : this->redirection_list) {
            if (redirection.GetTitleId() == title_id) {
                this->redirection_list.erase(this->redirection_list.iterator_to(redirection));
                delete &redirection;
                break;
            }
        }
    }

    void LocationRedirector::ClearRedirections(u32 flags) {
        for (auto it = this->redirection_list.begin(); it != this->redirection_list.end();) {
            if ((it->GetFlags() & flags) == flags) {
                auto old = it;
                it = this->redirection_list.erase(it);
                delete &(*old);
            } else {
                it++;
            }
        }
    }

}
