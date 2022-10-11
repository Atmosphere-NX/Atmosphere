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
#include <stratosphere/lr/lr_types.hpp>

#define AMS_LR_I_LOCATION_RESOLVER_INTERFACE_INFO(C, H)                                                                                                                                                                       \
    AMS_SF_METHOD_INFO(C, H,  0, Result, ResolveProgramPath,                                (sf::Out<lr::Path> out, ncm::ProgramId id),                         (out, id))                                                    \
    AMS_SF_METHOD_INFO(C, H,  1, Result, RedirectProgramPath,                               (const lr::Path &path, ncm::ProgramId id),                          (path, id))                                                   \
    AMS_SF_METHOD_INFO(C, H,  2, Result, ResolveApplicationControlPath,                     (sf::Out<lr::Path> out, ncm::ProgramId id),                         (out, id))                                                    \
    AMS_SF_METHOD_INFO(C, H,  3, Result, ResolveApplicationHtmlDocumentPath,                (sf::Out<lr::Path> out, ncm::ProgramId id),                         (out, id))                                                    \
    AMS_SF_METHOD_INFO(C, H,  4, Result, ResolveDataPath,                                   (sf::Out<lr::Path> out, ncm::DataId id),                            (out, id))                                                    \
    AMS_SF_METHOD_INFO(C, H,  5, Result, RedirectApplicationControlPathDeprecated,          (const lr::Path &path, ncm::ProgramId id),                          (path, id),           hos::Version_1_0_0, hos::Version_8_1_1) \
    AMS_SF_METHOD_INFO(C, H,  5, Result, RedirectApplicationControlPath,                    (const lr::Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), (path, id, owner_id), hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H,  6, Result, RedirectApplicationHtmlDocumentPathDeprecated,     (const lr::Path &path, ncm::ProgramId id),                          (path, id),           hos::Version_1_0_0, hos::Version_8_1_1) \
    AMS_SF_METHOD_INFO(C, H,  6, Result, RedirectApplicationHtmlDocumentPath,               (const lr::Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), (path, id, owner_id), hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H,  7, Result, ResolveApplicationLegalInformationPath,            (sf::Out<lr::Path> out, ncm::ProgramId id),                         (out, id))                                                    \
    AMS_SF_METHOD_INFO(C, H,  8, Result, RedirectApplicationLegalInformationPathDeprecated, (const lr::Path &path, ncm::ProgramId id),                          (path, id),           hos::Version_1_0_0, hos::Version_8_1_1) \
    AMS_SF_METHOD_INFO(C, H,  8, Result, RedirectApplicationLegalInformationPath,           (const lr::Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), (path, id, owner_id), hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H,  9, Result, Refresh,                                           (),                                                                 ())                                                           \
    AMS_SF_METHOD_INFO(C, H, 10, Result, RedirectApplicationProgramPathDeprecated,          (const lr::Path &path, ncm::ProgramId id),                          (path, id),           hos::Version_5_0_0, hos::Version_8_1_1) \
    AMS_SF_METHOD_INFO(C, H, 10, Result, RedirectApplicationProgramPath,                    (const lr::Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), (path, id, owner_id), hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 11, Result, ClearApplicationRedirectionDeprecated,             (),                                                                 (),                   hos::Version_5_0_0, hos::Version_8_1_1) \
    AMS_SF_METHOD_INFO(C, H, 11, Result, ClearApplicationRedirection,                       (const sf::InArray<ncm::ProgramId> &excluding_ids),                 (excluding_ids),      hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 12, Result, EraseProgramRedirection,                           (ncm::ProgramId id),                                                (id),                 hos::Version_5_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 13, Result, EraseApplicationControlRedirection,                (ncm::ProgramId id),                                                (id),                 hos::Version_5_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 14, Result, EraseApplicationHtmlDocumentRedirection,           (ncm::ProgramId id),                                                (id),                 hos::Version_5_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 15, Result, EraseApplicationLegalInformationRedirection,       (ncm::ProgramId id),                                                (id),                 hos::Version_5_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 16, Result, ResolveProgramPathForDebug,                        (sf::Out<lr::Path> out, ncm::ProgramId id),                         (out, id),            hos::Version_7_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 17, Result, RedirectProgramPathForDebug,                       (const lr::Path &path, ncm::ProgramId id),                          (path, id),           hos::Version_7_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 18, Result, RedirectApplicationProgramPathForDebugDeprecated,  (const lr::Path &path, ncm::ProgramId id),                          (path, id),           hos::Version_7_0_0, hos::Version_8_1_1) \
    AMS_SF_METHOD_INFO(C, H, 18, Result, RedirectApplicationProgramPathForDebug,            (const lr::Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), (path, id, owner_id), hos::Version_9_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 19, Result, EraseProgramRedirectionForDebug,                   (ncm::ProgramId id),                                                (id),                 hos::Version_7_0_0)                     \
    AMS_SF_METHOD_INFO(C, H, 20, Result, Disable,                                           (),                                                                 (),                   hos::Version_15_0_0)


AMS_SF_DEFINE_INTERFACE(ams::lr, ILocationResolver, AMS_LR_I_LOCATION_RESOLVER_INTERFACE_INFO, 0xB36C8B0E)
