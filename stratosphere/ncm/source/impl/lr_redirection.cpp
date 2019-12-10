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

namespace ams::lr::impl {

    class LocationRedirection  : public util::IntrusiveListBaseNode<LocationRedirection> {
        NON_COPYABLE(LocationRedirection);
        NON_MOVEABLE(LocationRedirection);
        private:
            ncm::ProgramId title_id;
            ncm::ProgramId owner_tid;
            Path path;
            u32 flags;
        public:
            LocationRedirection(ncm::ProgramId title_id, ncm::ProgramId owner_tid, const Path& path, u32 flags) :
                title_id(title_id), owner_tid(owner_tid), path(path), flags(flags) { /* ... */ }

            ncm::ProgramId GetTitleId() const {
                return this->title_id;
            }

            ncm::ProgramId GetOwnerTitleId() const {
                return this->owner_tid;
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

    bool LocationRedirector::FindRedirection(Path *out, ncm::ProgramId title_id) {
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

    void LocationRedirector::SetRedirection(ncm::ProgramId title_id, const Path &path, u32 flags) {
        this->SetRedirection(title_id, path, flags);
    }

    void LocationRedirector::SetRedirection(ncm::ProgramId title_id, ncm::ProgramId owner_tid, const Path &path, u32 flags) {
        this->EraseRedirection(title_id);
        this->redirection_list.push_back(*(new LocationRedirection(title_id, owner_tid, path, flags)));
    }

    void LocationRedirector::SetRedirectionFlags(ncm::ProgramId title_id, u32 flags) {
        for (auto &redirection : this->redirection_list) {
            if (redirection.GetTitleId() == title_id) {
                redirection.SetFlags(flags);
                break;
            }
        }
    }

    void LocationRedirector::EraseRedirection(ncm::ProgramId title_id) {
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

    void LocationRedirector::ClearRedirections(const ncm::ProgramId* excluding_tids, size_t num_tids) {
        for (auto it = this->redirection_list.begin(); it != this->redirection_list.end();) {
            bool skip = false;
            for (size_t i = 0; i < num_tids; i++) {
                ncm::ProgramId tid = excluding_tids[i];

                if (it->GetOwnerTitleId() == tid) {
                    skip = true;
                    break;
                }
            }

            if (skip) {
                continue;
            }

            auto old = it;
            it = this->redirection_list.erase(it);
            delete &(*old);
            it++;
        }
    }

}
