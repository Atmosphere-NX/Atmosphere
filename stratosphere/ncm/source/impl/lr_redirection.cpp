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
            ncm::ProgramId program_id;
            ncm::ProgramId owner_id;
            Path path;
            u32 flags;
        public:
            LocationRedirection(ncm::ProgramId program_id, ncm::ProgramId owner_id, const Path& path, u32 flags) :
                program_id(program_id), owner_id(owner_id), path(path), flags(flags) { /* ... */ }

            ncm::ProgramId GetProgramId() const {
                return this->program_id;
            }

            ncm::ProgramId GetOwnerProgramId() const {
                return this->owner_id;
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

    bool LocationRedirector::FindRedirection(Path *out, ncm::ProgramId program_id) {
        if (this->redirection_list.empty()) {
            return false;
        }

        for (const auto &redirection : this->redirection_list) {
            if (redirection.GetProgramId() == program_id) {
                redirection.GetPath(out);
                return true;
            }
        }
        return false;
    }

    void LocationRedirector::SetRedirection(ncm::ProgramId program_id, const Path &path, u32 flags) {
        this->SetRedirection(program_id, path, flags);
    }

    void LocationRedirector::SetRedirection(ncm::ProgramId program_id, ncm::ProgramId owner_id, const Path &path, u32 flags) {
        this->EraseRedirection(program_id);
        this->redirection_list.push_back(*(new LocationRedirection(program_id, owner_id, path, flags)));
    }

    void LocationRedirector::SetRedirectionFlags(ncm::ProgramId program_id, u32 flags) {
        for (auto &redirection : this->redirection_list) {
            if (redirection.GetProgramId() == program_id) {
                redirection.SetFlags(flags);
                break;
            }
        }
    }

    void LocationRedirector::EraseRedirection(ncm::ProgramId program_id) {
        for (auto &redirection : this->redirection_list) {
            if (redirection.GetProgramId() == program_id) {
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

    void LocationRedirector::ClearRedirections(const ncm::ProgramId* excluding_ids, size_t num_ids) {
        for (auto it = this->redirection_list.begin(); it != this->redirection_list.end();) {
            bool skip = false;
            for (size_t i = 0; i < num_ids; i++) {
                ncm::ProgramId id = excluding_ids[i];

                if (it->GetOwnerProgramId() == id) {
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
