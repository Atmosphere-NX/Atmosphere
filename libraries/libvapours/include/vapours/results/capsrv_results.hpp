/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace ams::capsrv {

    R_DEFINE_NAMESPACE_RESULT_MODULE(206);

    R_DEFINE_ERROR_RESULT(InvalidArgument,                  2);
    R_DEFINE_ERROR_RESULT(OutOfMemory,                      3);

    R_DEFINE_ERROR_RESULT(InvalidState,                     7);
    R_DEFINE_ERROR_RESULT(OutOfRange,                       8);

    R_DEFINE_ERROR_RESULT(InvalidApplicationId,             10);
    R_DEFINE_ERROR_RESULT(InvalidUnknown,                   11);
    R_DEFINE_ERROR_RESULT(InvalidFileId,                    12);
    R_DEFINE_ERROR_RESULT(InvalidStorageId,                 13);
    R_DEFINE_ERROR_RESULT(InvalidContentType,               14);

    R_DEFINE_ERROR_RESULT(FailedToMountImageDirectory,      21);
    R_DEFINE_ERROR_RESULT(ReachedSizeLimit,                 22);
    R_DEFINE_ERROR_RESULT(FileInaccessible,                 23);
    R_DEFINE_ERROR_RESULT(InvalidFileData,                  24);
    R_DEFINE_ERROR_RESULT(ReachedCountLimit,                25);
    R_DEFINE_ERROR_RESULT(InvalidThumbnail,                 26);

    R_DEFINE_ERROR_RESULT(BufferInsufficient,               30);

    R_DEFINE_ERROR_RESULT(FileReserved,                     94);
    R_DEFINE_ERROR_RESULT(FileReservedRead,                 96);

    R_DEFINE_ERROR_RESULT(TooManyApplicationsRegistered,    820);
    R_DEFINE_ERROR_RESULT(ApplicationNotRegistered,         822);

    R_DEFINE_ERROR_RESULT(DebugModeDisabled,                1023);
    R_DEFINE_ERROR_RESULT(InvalidMakerNote,                 1024);

    R_DEFINE_ERROR_RESULT(OutOfWorkMemory,                  1212);

    R_DEFINE_ERROR_RANGE(JpegMeta, 1300, 1399);
        R_DEFINE_ERROR_RESULT(ContentTypeMissmatch,             1300);
        R_DEFINE_ERROR_RESULT(InvalidJPEG,                      1301);
        R_DEFINE_ERROR_RESULT(InvalidJFIF,                      1302);
        R_DEFINE_ERROR_RESULT(InvalidEXIF,                      1303);
        R_DEFINE_ERROR_RESULT(MissingDateTime,                  1304);
        R_DEFINE_ERROR_RESULT(InvalidDateTimeLength,            1305);
        R_DEFINE_ERROR_RESULT(DateTimeMissmatch,                1306);
        R_DEFINE_ERROR_RESULT(MissingMakerNote,                 1307);
        R_DEFINE_ERROR_RESULT(ApplicationIdMissmatch,           1308);
        R_DEFINE_ERROR_RESULT(InvalidMacHash,                   1309);
        R_DEFINE_ERROR_RESULT(InvalidOrientation,               1310);
        R_DEFINE_ERROR_RESULT(InvalidDimension,                 1311);
        R_DEFINE_ERROR_RESULT(MakerNoteUnk,                     1312);

    R_DEFINE_ERROR_RESULT(TooManyFiles,                     1401);

    R_DEFINE_ERROR_RESULT(InvalidJpegHeader,                1501);
    R_DEFINE_ERROR_RESULT(InvalidJpegSize,                  1502);

    R_DEFINE_ERROR_RESULT(SessionAllocationFailure,         1701);

    R_DEFINE_ERROR_RANGE(TemporaryFile, 1900, 1999);
        R_DEFINE_ERROR_RESULT(TemporaryFileMaxCountReached,     1901);
        R_DEFINE_ERROR_RESULT(TemporaryFileOpenFailed,          1902);
        R_DEFINE_ERROR_RESULT(TemporaryFileAccessFailed,        1903);
        R_DEFINE_ERROR_RESULT(TemporaryFileGetCountFailed,      1904);
        R_DEFINE_ERROR_RESULT(TemporaryFileSetSizeFailed,       1906);
        R_DEFINE_ERROR_RESULT(TemporaryFileReadFailed,          1907);
        R_DEFINE_ERROR_RESULT(TemporaryFileWriteFailed,         1908);

}
