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
#include <vapours/results/results_common.hpp>

namespace ams::capsrv {

    R_DEFINE_NAMESPACE_RESULT_MODULE(206);

    R_DEFINE_ERROR_RANGE(AlbumError, 2, 99);
        R_DEFINE_ERROR_RESULT(AlbumWorkMemoryError,                     3);

        R_DEFINE_ERROR_RESULT(AlbumAlreadyOpened,                       7);
        R_DEFINE_ERROR_RESULT(AlbumOutOfRange,                          8);

        R_DEFINE_ERROR_RANGE(AlbumInvalidFileId, 10, 19);
            R_DEFINE_ERROR_RESULT(AlbumInvalidApplicationId,            11);
            R_DEFINE_ERROR_RESULT(AlbumInvalidTimestamp,                12);
            R_DEFINE_ERROR_RESULT(AlbumInvalidStorage,                  13);
            R_DEFINE_ERROR_RESULT(AlbumInvalidFileContents,             14);

        R_DEFINE_ERROR_RESULT(AlbumIsNotMounted,                        21);
        R_DEFINE_ERROR_RESULT(AlbumIsFull,                              22);
        R_DEFINE_ERROR_RESULT(AlbumFileNotFound,                        23);
        R_DEFINE_ERROR_RESULT(AlbumInvalidFileData,                     24);
        R_DEFINE_ERROR_RESULT(AlbumFileCountLimit,                      25);
        R_DEFINE_ERROR_RESULT(AlbumFileNoThumbnail,                     26);

        R_DEFINE_ERROR_RESULT(AlbumReadBufferShortage,                  30);

        R_DEFINE_ERROR_RANGE(AlbumFileSystemError, 90, 99);
            R_DEFINE_ERROR_RANGE(AlbumAccessCorrupted, 94, 96);
                R_DEFINE_ERROR_RESULT(AlbumDestinationAccessCorrupted,  96);

    R_DEFINE_ERROR_RANGE(ControlError, 800, 899);
    R_DEFINE_ERROR_RESULT(ControlResourceLimit, 820);
    R_DEFINE_ERROR_RESULT(ControlNotOpened,     822);

    R_DEFINE_ERROR_RESULT(NotSupported, 1023);

    R_DEFINE_ERROR_RANGE(InternalError, 1024, 2047);
        R_DEFINE_ERROR_RESULT(InternalJpegEncoderError,         1210);
        R_DEFINE_ERROR_RESULT(InternalJpegWorkMemoryShortage,   1212);

        R_DEFINE_ERROR_RANGE(InternalFileDataVerificationError, 1300, 1399);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationEmptyFileData,                1301);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationExifExtractionFailed,         1302);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationExifAnalyzationFailed,        1303);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationDateTimeExtractionFailed,     1304);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationInvalidDateTimeLength,        1305);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationInconsistentDateTime,         1306);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationMakerNoteExtractionFailed,    1307);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationInconsistentApplicationId,    1308);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationInconsistentSignature,        1309);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationUnsupportedOrientation,       1310);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationInvalidDataDimension,         1311);
            R_DEFINE_ERROR_RESULT(InternalFileDataVerificationInconsistentOrientation,      1312);

        R_DEFINE_ERROR_RANGE(InternalAlbumLimitationError, 1400, 1499);
            R_DEFINE_ERROR_RESULT(InternalAlbumLimitationFileCountLimit,    1401);

        R_DEFINE_ERROR_RANGE(InternalSignatureError, 1500, 1599);
        R_DEFINE_ERROR_RESULT(InternalSignatureExifExtractionFailed,        1501);
        R_DEFINE_ERROR_RESULT(InternalSignatureMakerNoteExtractionFailed,   1502);

        R_DEFINE_ERROR_RANGE(InternalAlbumSessionError, 1700, 1799);
            R_DEFINE_ERROR_RESULT(InternalAlbumLimitationSessionCountLimit, 1701);

        R_DEFINE_ERROR_RANGE(InternalAlbumTemporaryFileError, 1900, 1999);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileCountLimit,             1901);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileCreateError,            1902);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileCreateRetryCountLimit,  1903);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileOpenError,              1904);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileGetFileSizeError,       1905);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileSetFileSizeError,       1906);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileReadFileError,          1907);
            R_DEFINE_ERROR_RESULT(InternalAlbumTemporaryFileWriteFileError,         1908);

}
