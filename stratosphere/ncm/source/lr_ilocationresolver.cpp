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

#include "lr_ilocationresolver.hpp"

namespace sts::lr {

    Result ILocationResolver::ResolveProgramPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectProgramPath(InPointer<const Path> path, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::ResolveApplicationControlPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::ResolveApplicationHtmlDocumentPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::ResolveDataPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationControlPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationControlPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationHtmlDocumentPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationHtmlDocumentPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        std::abort();
    }

    Result ILocationResolver::ResolveApplicationLegalInformationPath(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationLegalInformationPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationLegalInformationPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        std::abort();
    }

    Result ILocationResolver::Refresh() {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationProgramPathDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationProgramPath(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        std::abort();
    }

    Result ILocationResolver::ClearApplicationRedirectionDeprecated() {
        std::abort();
    }

    Result ILocationResolver::ClearApplicationRedirection(InBuffer<ncm::TitleId> excluding_tids) {
        std::abort();
    }

    Result ILocationResolver::EraseProgramRedirection(ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::EraseApplicationControlRedirection(ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::EraseApplicationHtmlDocumentRedirection(ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::EraseApplicationLegalInformationRedirection(ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::ResolveProgramPathForDebug(OutPointerWithServerSize<Path, 0x1> out, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationProgramPathForDebugDeprecated(InPointer<const Path> path, ncm::TitleId tid) {
        std::abort();
    }

    Result ILocationResolver::RedirectApplicationProgramPathForDebug(InPointer<const Path> path, ncm::TitleId tid, ncm::TitleId title_id_2) {
        std::abort();
    }

    Result ILocationResolver::EraseProgramRedirectionForDebug(ncm::TitleId tid) {
        std::abort();
    }

}
