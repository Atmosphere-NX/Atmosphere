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

#pragma once
#include <stratosphere/lr/lr_types.hpp>

namespace ams::lr {

    #define AMS_LR_I_LOCATION_RESOLVER_INTERFACE_INFO(C, H)                                                                                                                                             \
        AMS_SF_METHOD_INFO(C, H,  0, Result, ResolveProgramPath,                                (sf::Out<Path> out, ncm::ProgramId id))                                                                 \
        AMS_SF_METHOD_INFO(C, H,  1, Result, RedirectProgramPath,                               (const Path &path, ncm::ProgramId id))                                                                  \
        AMS_SF_METHOD_INFO(C, H,  2, Result, ResolveApplicationControlPath,                     (sf::Out<Path> out, ncm::ProgramId id))                                                                 \
        AMS_SF_METHOD_INFO(C, H,  3, Result, ResolveApplicationHtmlDocumentPath,                (sf::Out<Path> out, ncm::ProgramId id))                                                                 \
        AMS_SF_METHOD_INFO(C, H,  4, Result, ResolveDataPath,                                   (sf::Out<Path> out, ncm::DataId id))                                                                    \
        AMS_SF_METHOD_INFO(C, H,  5, Result, RedirectApplicationControlPathDeprecated,          (const Path &path, ncm::ProgramId id),                          hos::Version_1_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H,  5, Result, RedirectApplicationControlPath,                    (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H,  6, Result, RedirectApplicationHtmlDocumentPathDeprecated,     (const Path &path, ncm::ProgramId id),                          hos::Version_1_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H,  6, Result, RedirectApplicationHtmlDocumentPath,               (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H,  7, Result, ResolveApplicationLegalInformationPath,            (sf::Out<Path> out, ncm::ProgramId id))                                                                 \
        AMS_SF_METHOD_INFO(C, H,  8, Result, RedirectApplicationLegalInformationPathDeprecated, (const Path &path, ncm::ProgramId id),                          hos::Version_1_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H,  8, Result, RedirectApplicationLegalInformationPath,           (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H,  9, Result, Refresh,                                           ())                                                                                                     \
        AMS_SF_METHOD_INFO(C, H, 10, Result, RedirectApplicationProgramPathDeprecated,          (const Path &path, ncm::ProgramId id),                          hos::Version_5_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H, 10, Result, RedirectApplicationProgramPath,                    (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 11, Result, ClearApplicationRedirectionDeprecated,             (),                                                             hos::Version_5_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H, 11, Result, ClearApplicationRedirection,                       (const sf::InArray<ncm::ProgramId> &excluding_ids),             hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 12, Result, EraseProgramRedirection,                           (ncm::ProgramId id),                                            hos::Version_5_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 13, Result, EraseApplicationControlRedirection,                (ncm::ProgramId id),                                            hos::Version_5_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 14, Result, EraseApplicationHtmlDocumentRedirection,           (ncm::ProgramId id),                                            hos::Version_5_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 15, Result, EraseApplicationLegalInformationRedirection,       (ncm::ProgramId id),                                            hos::Version_5_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 16, Result, ResolveProgramPathForDebug,                        (sf::Out<Path> out, ncm::ProgramId id),                         hos::Version_7_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 17, Result, RedirectProgramPathForDebug,                       (const Path &path, ncm::ProgramId id),                          hos::Version_7_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 18, Result, RedirectApplicationProgramPathForDebugDeprecated,  (const Path &path, ncm::ProgramId id),                          hos::Version_7_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H, 18, Result, RedirectApplicationProgramPathForDebug,            (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 19, Result, EraseProgramRedirectionForDebug,                   (ncm::ProgramId id),                                            hos::Version_7_0_0)


    AMS_SF_DEFINE_INTERFACE(ILocationResolver, AMS_LR_I_LOCATION_RESOLVER_INTERFACE_INFO)

}
