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
#include <haze.hpp>
#include <haze/ptp_data_builder.hpp>
#include <haze/ptp_data_parser.hpp>
#include <haze/ptp_responder_types.hpp>

namespace haze {

    Result PtpResponder::GetPartialObject64(PtpDataParser &dp) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        /* Get the object ID, offset, and size for the file we want to read. */
        u32 object_id, size;
        u64 offset;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Read(std::addressof(offset)));
        R_TRY(dp.Read(std::addressof(size)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Lock the object as a file. */
        FsFile file;
        R_TRY(m_fs.OpenFile(obj->GetName(), FsOpenMode_Read, std::addressof(file)));

        /* Ensure we maintain a clean state on exit. */
        ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

        /* Get the file's size. */
        s64 file_size = 0;
        R_TRY(m_fs.GetFileSize(std::addressof(file), std::addressof(file_size)));

        /* Ensure the requested offset and size are within range. */
        R_UNLESS(offset + size > offset, haze::ResultInvalidArgument());
        R_UNLESS(static_cast<u64>(file_size) <= offset + size, haze::ResultInvalidArgument());

        /* Send the header and data size. */
        R_TRY(db.AddDataHeader(m_request_header, size));

        /* Begin reading the file, writing data to the builder as we progress. */
        s64 size_remaining = size;
        while (true) {
            /* Get the next batch. */
            u64 bytes_to_read = std::min<s64>(FsBufferSize, size_remaining);
            u64 bytes_read;

            R_TRY(m_fs.ReadFile(std::addressof(file), offset, m_buffers->file_system_data_buffer, bytes_to_read, FsReadOption_None, std::addressof(bytes_read)));

            size_remaining -= bytes_read;
            offset += bytes_read;

            /* Write to output. */
            R_TRY(db.AddBuffer(m_buffers->file_system_data_buffer, bytes_read));

            /* If we read fewer bytes than the batch size, or have read enough data, we're done. */
            if (bytes_read < FsBufferSize || size_remaining == 0) {
                break;
            }
        }

        /* Flush the data response. */
        R_TRY(db.Commit());

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::SendPartialObject(PtpDataParser &rdp) {
        /* Get the object ID, offset, and size for the file we want to write. */
        u32 object_id, size;
        u64 offset;
        R_TRY(rdp.Read(std::addressof(object_id)));
        R_TRY(rdp.Read(std::addressof(size)));
        R_TRY(rdp.Read(std::addressof(offset)));
        R_TRY(rdp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(m_send_object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Lock the object as a file. */
        FsFile file;
        R_TRY(m_fs.OpenFile(obj->GetName(), FsOpenMode_Write | FsOpenMode_Append, std::addressof(file)));

        /* Ensure we maintain a clean state on exit. */
        ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

        /* Get the file's size. */
        s64 file_size = 0;
        R_TRY(m_fs.GetFileSize(std::addressof(file), std::addressof(file_size)));

        /* Ensure the requested offset and size are within range. */
        R_UNLESS(offset + size > offset, haze::ResultInvalidArgument());
        R_UNLESS(static_cast<u64>(file_size) <= offset, haze::ResultInvalidArgument());

        /* Prepare a data parser for the data we are about to receive. */
        PtpDataParser dp(m_buffers->usb_bulk_read_buffer, std::addressof(m_usb_server));

        /* Ensure we have a data header. */
        PtpUsbBulkContainer data_header;
        R_TRY(dp.Read(std::addressof(data_header)));
        R_UNLESS(data_header.type == PtpUsbBulkContainerType_Data,  haze::ResultUnknownRequestType());
        R_UNLESS(data_header.code == m_request_header.code,         haze::ResultOperationNotSupported());
        R_UNLESS(data_header.trans_id == m_request_header.trans_id, haze::ResultOperationNotSupported());

        /* Begin writing to the filesystem. */
        s64 size_remaining = size;
        while (true) {
            /* Read as many bytes as we can. */
            u32 bytes_received;
            const Result read_res = dp.ReadBuffer(m_buffers->file_system_data_buffer, FsBufferSize, std::addressof(bytes_received));

            /* Write to the file. */
            u32 bytes_to_write = std::min<s64>(size_remaining, bytes_received);
            R_TRY(m_fs.WriteFile(std::addressof(file), offset, m_buffers->file_system_data_buffer, bytes_to_write, 0));

            size_remaining -= bytes_to_write;
            offset += bytes_to_write;

            /* If we received fewer bytes than the batch size, or have written enough data, we're done. */
            if (haze::ResultEndOfTransmission::Includes(read_res) || size_remaining == 0) {
                break;
            }

            R_TRY(read_res);
        }

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::TruncateObject(PtpDataParser &dp) {
        /* Get the object ID and size for the file we want to truncate. */
        u32 object_id;
        u64 size;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Read(std::addressof(size)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Lock the object as a file. */
        FsFile file;
        R_TRY(m_fs.OpenFile(obj->GetName(), FsOpenMode_Write, std::addressof(file)));

        /* Ensure we maintain a clean state on exit. */
        ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

        /* Truncate the file. */
        R_TRY(m_fs.SetFileSize(std::addressof(file), size));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::BeginEditObject(PtpDataParser &dp) {
        /* Get the object ID we are going to begin editing. */
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* We don't implement transactions, so write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::EndEditObject(PtpDataParser &dp) {
        /* Get the object ID we are going to finish editing. */
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* We don't implement transactions, so write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }


}
