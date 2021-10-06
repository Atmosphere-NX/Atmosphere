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
#include <stratosphere.hpp>
#include <csetjmp>
#include "capsrv_server_jpeg_library_types.hpp"

namespace ams::capsrv::server::jpeg {

    struct JpegErrorHandler : public JpegLibraryType::jpeg_error_mgr {
        public:
            std::jmp_buf jmp_buf;
            Result result;
        public:
            static void HandleError(JpegLibraryType::jpeg_common_struct *common) {
                /* Retrieve the handler. */
                JpegErrorHandler *handler = reinterpret_cast<JpegErrorHandler *>(common->err);

                /* Set the result. */
                handler->result = GetResult(handler->msg_code, handler->msg_parm.i[0]);

                /* Return to the caller. */
                longjmp(handler->jmp_buf, -1);
            }

            static Result GetResult(int msg_code, int msg_param) {
                /* NOTE: Nintendo uses msg_param for error codes that we never trigger. */
                /* TODO: Fully support all J_MESSAGE_CODEs that Nintendo handles? */
                AMS_UNUSED(msg_param);

                switch (msg_code) {
                    case JpegLibraryType::J_MESSAGE_CODE::JERR_BUFFER_SIZE:
                    case JpegLibraryType::J_MESSAGE_CODE::JERR_NO_BACKING_STORE:
                    case JpegLibraryType::J_MESSAGE_CODE::JERR_OUT_OF_MEMORY:
                    case JpegLibraryType::J_MESSAGE_CODE::JERR_TFILE_CREATE:
                    case JpegLibraryType::J_MESSAGE_CODE::JERR_TFILE_READ:
                    case JpegLibraryType::J_MESSAGE_CODE::JERR_TFILE_SEEK:
                    case JpegLibraryType::J_MESSAGE_CODE::JERR_TFILE_WRITE:
                        return capsrv::ResultInternalJpegWorkMemoryShortage();
                    default:
                        return capsrv::ResultInternalJpegEncoderError();
                }
            }
    };

}
