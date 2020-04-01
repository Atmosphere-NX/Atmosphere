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
#include "lr_location_redirector.hpp"

namespace ams::lr {

    class LocationRedirector::Redirection : public util::IntrusiveListBaseNode<Redirection> {
        NON_COPYABLE(Redirection);
        NON_MOVEABLE(Redirection);
        private:
            ncm::ProgramId program_id;
            ncm::ProgramId owner_id;
            Path path;
            u32 flags;
        public:
            Redirection(ncm::ProgramId program_id, ncm::ProgramId owner_id, const Path &path, u32 flags) :
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

    bool LocationRedirector::FindRedirection(Path *out, ncm::ProgramId program_id) const {
        /* Obtain the path of a matching redirection. */
        for (const auto &redirection : this->redirection_list) {
            if (redirection.GetProgramId() == program_id) {
                redirection.GetPath(out);
                return true;
            }
        }
        return false;
    }

    void LocationRedirector::SetRedirection(ncm::ProgramId program_id, const Path &path, u32 flags) {
        this->SetRedirection(program_id, ncm::InvalidProgramId, path, flags);
    }

    void LocationRedirector::SetRedirection(ncm::ProgramId program_id, ncm::ProgramId owner_id, const Path &path, u32 flags) {
        /* Remove any existing redirections for this program id. */
        this->EraseRedirection(program_id);

        /* Insert a new redirection into the list. */
        this->redirection_list.push_back(*(new Redirection(program_id, owner_id, path, flags)));
    }

    void LocationRedirector::SetRedirectionFlags(ncm::ProgramId program_id, u32 flags) {
        /* Set the flags of a redirection with a matching program id. */
        for (auto &redirection : this->redirection_list) {
            if (redirection.GetProgramId() == program_id) {
                redirection.SetFlags(flags);
                break;
            }
        }
    }

    void LocationRedirector::EraseRedirection(ncm::ProgramId program_id)
    {
        /* Remove any redirections with a matching program id. */
        for (auto &redirection : this->redirection_list) {
            if (redirection.GetProgramId() == program_id) {
                this->redirection_list.erase(this->redirection_list.iterator_to(redirection));
                delete &redirection;
                break;
            }
        }
    }

    void LocationRedirector::ClearRedirections(u32 flags) {
        /* Remove any redirections with matching flags. */
        for (auto it = this->redirection_list.begin(); it != this->redirection_list.end();) {
            if ((it->GetFlags() & flags) == flags) {
                auto old = it;
                it = this->redirection_list.erase(it);
                delete std::addressof(*old);
            } else {
                it++;
            }
        }
    }

    void LocationRedirector::ClearRedirectionsExcludingOwners(const ncm::ProgramId *excluding_ids, size_t num_ids) {
        for (auto it = this->redirection_list.begin(); it != this->redirection_list.end();) {
            /* Skip removal if the redirection has an excluded owner program id. */
            if (this->IsExcluded(it->GetOwnerProgramId(), excluding_ids, num_ids)) {
                it++;
                continue;
            }

            /* Remove the redirection. */
            auto old = it;
            it = this->redirection_list.erase(it);
            delete std::addressof(*old);
        }
    }

}
