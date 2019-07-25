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

    bool LocationRedirector::FindRedirection(Path *out, ncm::TitleId title_id) {
        if (this->redirection_list.empty()) {
            return false;
        }

        for (auto it = this->redirection_list.begin(); it != this->redirection_list.end(); it++) {
            if (it->title_id == title_id) {
                *out = it->path;
                return true;
            }
        }

        return false;
    }

    void LocationRedirector::SetRedirection(ncm::TitleId title_id, const Path &path, u32 flags) {
        this->EraseRedirection(title_id);
        auto redirection = new LocationRedirection(title_id, path, flags);
        this->redirection_list.push_back(*redirection);
    }

    void LocationRedirector::SetRedirectionFlags(ncm::TitleId title_id, u32 flags) {
        if (!this->redirection_list.empty()) {
            for (auto it = this->redirection_list.begin(); it != this->redirection_list.end(); it++) {
                if (it->title_id == title_id) {
                    it->flags = flags;
                    break;
                }
            }
        }
    }

    void LocationRedirector::EraseRedirection(ncm::TitleId title_id) {
        if (!this->redirection_list.empty()) {
            for (auto it = this->redirection_list.begin(); it != this->redirection_list.end(); it++) {
                if (it->title_id == title_id) {
                    auto old = it;
                    this->redirection_list.erase(old);
                    delete &(*old);
                    break;
                }
            }
        }
    }

    void LocationRedirector::ClearRedirections(u32 flags) {
        for (auto it = this->redirection_list.begin(); it != this->redirection_list.end();) {
            if ((it->flags & flags) == flags) {
                auto old = it;
                it = this->redirection_list.erase(it);
                delete &(*old);
            } else {
                it++;
            }
        }
    }

    bool RegisteredLocationRedirector::FindRedirection(Path *out, ncm::TitleId title_id) {
        auto redirection = this->redirections.Find(title_id);

        if (redirection) {
            *out = *redirection;
            return true;
        }
        
        return false;
    }

    bool RegisteredLocationRedirector::SetRedirection(ncm::TitleId title_id, const Path& path) {
        if (this->redirections.IsFull()) {
            return false;
        }

        this->redirections[title_id] = path;
        return true;
    }

    void RegisteredLocationRedirector::EraseRedirection(ncm::TitleId title_id) {
        this->redirections.Remove(title_id);
    }

    void RegisteredLocationRedirector::ClearRedirections() {
        this->redirections.RemoveAll();
    }

    bool AddOnContentRedirector::FindRedirection(ncm::StorageId *out, ncm::TitleId title_id) {
        auto redirection = this->redirections.Find(title_id);

        if (redirection) {
            *out = *redirection;
            return true;
        }
        
        return false;
    }

    Result AddOnContentRedirector::SetRedirection(ncm::TitleId title_id, ncm::StorageId storage_id) {
        if (this->redirections.IsFull()) {
            return ResultLrTooManyRegisteredPaths;
        }

        this->redirections[title_id] = storage_id;
        return ResultSuccess;
    }

    void AddOnContentRedirector::ClearRedirections() {
        this->redirections.RemoveAll();
    }

}