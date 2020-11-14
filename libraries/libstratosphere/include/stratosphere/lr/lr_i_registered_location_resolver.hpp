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

    #define AMS_LR_I_REGISTERED_LOCATION_RESOLVER_INTERFACE_INFO(C, H)                                                                                                                  \
        AMS_SF_METHOD_INFO(C, H, 0, Result, ResolveProgramPath,                 (sf::Out<Path> out, ncm::ProgramId id))                                                                 \
        AMS_SF_METHOD_INFO(C, H, 1, Result, RegisterProgramPathDeprecated,      (const Path &path, ncm::ProgramId id),                          hos::Version_1_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H, 1, Result, RegisterProgramPath,                (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 2, Result, UnregisterProgramPath,              (ncm::ProgramId id))                                                                                    \
        AMS_SF_METHOD_INFO(C, H, 3, Result, RedirectProgramPathDeprecated,      (const Path &path, ncm::ProgramId id),                          hos::Version_1_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H, 3, Result, RedirectProgramPath,                (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 4, Result, ResolveHtmlDocumentPath,            (sf::Out<Path> out, ncm::ProgramId id),                         hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 5, Result, RegisterHtmlDocumentPathDeprecated, (const Path &path, ncm::ProgramId id),                          hos::Version_2_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H, 5, Result, RegisterHtmlDocumentPath,           (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 6, Result, UnregisterHtmlDocumentPath,         (ncm::ProgramId id),                                            hos::Version_2_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 7, Result, RedirectHtmlDocumentPathDeprecated, (const Path &path, ncm::ProgramId id),                          hos::Version_2_0_0, hos::Version_8_1_1) \
        AMS_SF_METHOD_INFO(C, H, 7, Result, RedirectHtmlDocumentPath,           (const Path &path, ncm::ProgramId id, ncm::ProgramId owner_id), hos::Version_9_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 8, Result, Refresh,                            (),                                                             hos::Version_7_0_0)                     \
        AMS_SF_METHOD_INFO(C, H, 9, Result, RefreshExcluding,                   (const sf::InArray<ncm::ProgramId> &ids),                       hos::Version_9_0_0)

    AMS_SF_DEFINE_INTERFACE(IRegisteredLocationResolver, AMS_LR_I_REGISTERED_LOCATION_RESOLVER_INTERFACE_INFO)

}
