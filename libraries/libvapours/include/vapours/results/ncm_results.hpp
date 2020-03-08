/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
#include <vapours/results/results_common.hpp>

namespace ams::ncm {

    R_DEFINE_NAMESPACE_RESULT_MODULE(5);

    R_DEFINE_ERROR_RESULT(InvalidContentStorageBase,                1);
    R_DEFINE_ERROR_RESULT(PlaceHolderAlreadyExists,                 2);
    R_DEFINE_ERROR_RESULT(PlaceHolderNotFound,                      3);
    R_DEFINE_ERROR_RESULT(ContentAlreadyExists,                     4);
    R_DEFINE_ERROR_RESULT(ContentNotFound,                          5);
    R_DEFINE_ERROR_RESULT(ContentMetaNotFound,                      7);
    R_DEFINE_ERROR_RESULT(AllocationFailed,                         8);
    R_DEFINE_ERROR_RESULT(UnknownStorage,                           12);

    R_DEFINE_ERROR_RESULT(InvalidContentStorage,                    100);
    R_DEFINE_ERROR_RESULT(InvalidContentMetaDatabase,               110);

    R_DEFINE_ERROR_RESULT(InvalidPackageFormat,                     130);

    R_DEFINE_ERROR_RESULT(InvalidPlaceHolderFile,                   170);
    R_DEFINE_ERROR_RESULT(BufferInsufficient,                       180);
    R_DEFINE_ERROR_RESULT(WriteToReadOnlyContentStorage,            190);
    R_DEFINE_ERROR_RESULT(InvalidContentMetaKey,                    240);

    R_DEFINE_ERROR_RESULT(ContentStorageBaseNotFound,               310);

    R_DEFINE_ERROR_RANGE(ContentStorageNotActive, 250, 258);
        R_DEFINE_ERROR_RESULT(GameCardContentStorageNotActive,              251);
        R_DEFINE_ERROR_RESULT(BuiltInSystemContentStorageNotActive,         252);
        R_DEFINE_ERROR_RESULT(BuiltInUserContentStorageNotActive,           253);
        R_DEFINE_ERROR_RESULT(SdCardContentStorageNotActive,                254);
        R_DEFINE_ERROR_RESULT(UnknownContentStorageNotActive,               258);

    R_DEFINE_ERROR_RANGE(ContentMetaDatabaseNotActive, 260, 268);
        R_DEFINE_ERROR_RESULT(GameCardContentMetaDatabaseNotActive,         261);
        R_DEFINE_ERROR_RESULT(BuiltInSystemContentMetaDatabaseNotActive,    262);
        R_DEFINE_ERROR_RESULT(BuiltInUserContentMetaDatabaseNotActive,      263);
        R_DEFINE_ERROR_RESULT(SdCardContentMetaDatabaseNotActive,           264);
        R_DEFINE_ERROR_RESULT(UnknownContentMetaDatabaseNotActive,          268);

    R_DEFINE_ERROR_RANGE(InvalidArgument, 8181, 8191);
        R_DEFINE_ERROR_RESULT(InvalidOffset, 8182);

}
