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

    namespace {

        PtpBuffers* GetBuffers() {
            static constinit PtpBuffers buffers = {};
            return std::addressof(buffers);
        }

    }

    Result PtpResponder::Initialize(EventReactor *reactor, PtpObjectHeap *object_heap) {
        m_object_heap = object_heap;
        m_buffers = GetBuffers();

        /* Configure fs proxy. */
        m_fs.Initialize(reactor, fsdevGetDeviceFileSystem("sdmc"));

        R_RETURN(m_usb_server.Initialize(std::addressof(MtpInterfaceInfo), SwitchMtpIdVendor, SwitchMtpIdProduct, reactor));
    }

    void PtpResponder::Finalize() {
        m_usb_server.Finalize();
        m_fs.Finalize();
    }

    Result PtpResponder::LoopProcess() {
        while (true) {
            /* Try to handle a request. */
            R_TRY_CATCH(this->HandleRequest()) {
                R_CATCH(haze::ResultStopRequested, haze::ResultFocusLost) {
                    /* If we encountered a stop condition, we're done.*/
                    R_THROW(R_CURRENT_RESULT);
                }
                R_CATCH_ALL() {
                    /* On other failures, try to handle another request. */
                    continue;
                }
            } R_END_TRY_CATCH;

            /* Otherwise, handle the next request. */
            /* ... */
        }
    }

    Result PtpResponder::HandleRequest() {
        ON_RESULT_FAILURE {
            /* For general failure modes, the failure is unrecoverable. Close the session. */
            this->ForceCloseSession();
        };

        R_TRY_CATCH(this->HandleRequestImpl()) {
            R_CATCH(haze::ResultUnknownRequestType) {
                R_TRY(this->WriteResponse(PtpResponseCode_GeneralError));
            }
            R_CATCH(haze::ResultSessionNotOpen) {
                R_TRY(this->WriteResponse(PtpResponseCode_SessionNotOpen));
            }
            R_CATCH(haze::ResultOperationNotSupported) {
                R_TRY(this->WriteResponse(PtpResponseCode_OperationNotSupported));
            }
            R_CATCH(haze::ResultInvalidStorageId) {
                R_TRY(this->WriteResponse(PtpResponseCode_InvalidStorageId));
            }
            R_CATCH(haze::ResultInvalidObjectId) {
                R_TRY(this->WriteResponse(PtpResponseCode_InvalidObjectHandle));
            }
            R_CATCH(haze::ResultUnknownPropertyCode) {
                R_TRY(this->WriteResponse(PtpResponseCode_MtpObjectPropNotSupported));
            }
            R_CATCH(haze::ResultInvalidPropertyValue) {
                R_TRY(this->WriteResponse(PtpResponseCode_MtpInvalidObjectPropValue));
            }
            R_CATCH_MODULE(fs) {
                /* Errors from fs are typically recoverable. */
                R_TRY(this->WriteResponse(PtpResponseCode_GeneralError));
            }
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result PtpResponder::HandleRequestImpl() {
        PtpDataParser dp(m_buffers->usb_bulk_read_buffer, std::addressof(m_usb_server));
        R_TRY(dp.Read(std::addressof(m_request_header)));

        switch (m_request_header.type) {
            case PtpUsbBulkContainerType_Command: R_RETURN(this->HandleCommandRequest(dp));
            default:                              R_THROW(haze::ResultUnknownRequestType());
        }
    }

    Result PtpResponder::HandleCommandRequest(PtpDataParser &dp) {
        if (!m_session_open && m_request_header.code != PtpOperationCode_OpenSession && m_request_header.code != PtpOperationCode_GetDeviceInfo)  {
            R_THROW(haze::ResultSessionNotOpen());
        }

        switch (m_request_header.code) {
            case PtpOperationCode_GetDeviceInfo:              R_RETURN(this->GetDeviceInfo(dp));           break;
            case PtpOperationCode_OpenSession:                R_RETURN(this->OpenSession(dp));             break;
            case PtpOperationCode_CloseSession:               R_RETURN(this->CloseSession(dp));            break;
            case PtpOperationCode_GetStorageIds:              R_RETURN(this->GetStorageIds(dp));           break;
            case PtpOperationCode_GetStorageInfo:             R_RETURN(this->GetStorageInfo(dp));          break;
            case PtpOperationCode_GetObjectHandles:           R_RETURN(this->GetObjectHandles(dp));        break;
            case PtpOperationCode_GetObjectInfo:              R_RETURN(this->GetObjectInfo(dp));           break;
            case PtpOperationCode_GetObject:                  R_RETURN(this->GetObject(dp));               break;
            case PtpOperationCode_SendObjectInfo:             R_RETURN(this->SendObjectInfo(dp));          break;
            case PtpOperationCode_SendObject:                 R_RETURN(this->SendObject(dp));              break;
            case PtpOperationCode_DeleteObject:               R_RETURN(this->DeleteObject(dp));            break;
            case PtpOperationCode_MtpGetObjectPropsSupported: R_RETURN(this->GetObjectPropsSupported(dp)); break;
            case PtpOperationCode_MtpGetObjectPropDesc:       R_RETURN(this->GetObjectPropDesc(dp));       break;
            case PtpOperationCode_MtpGetObjectPropValue:      R_RETURN(this->GetObjectPropValue(dp));      break;
            case PtpOperationCode_MtpSetObjectPropValue:      R_RETURN(this->SetObjectPropValue(dp));      break;
            default:                                          R_THROW(haze::ResultOperationNotSupported());
        }
    }

    void PtpResponder::ForceCloseSession() {
        if (m_session_open) {
            m_session_open = false;
            m_object_database.Finalize();
        }
    }

    Result PtpResponder::WriteResponse(PtpResponseCode code, const void* data, size_t size) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));
        R_TRY(db.AddResponseHeader(m_request_header, code, size));
        R_TRY(db.AddBuffer(reinterpret_cast<const u8*>(data), size));
        R_RETURN(db.Commit());
    }

    Result PtpResponder::WriteResponse(PtpResponseCode code) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));
        R_TRY(db.AddResponseHeader(m_request_header, code, 0));
        R_RETURN(db.Commit());
    }

    Result PtpResponder::GetDeviceInfo(PtpDataParser &dp) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        /* Write the device info data. */
        R_TRY(db.WriteVariableLengthData(m_request_header, [&] () {
            R_TRY(db.Add(MtpStandardVersion));
            R_TRY(db.Add(MtpVendorExtensionId));
            R_TRY(db.Add(MtpStandardVersion));
            R_TRY(db.AddString(MtpVendorExtensionDesc));
            R_TRY(db.Add(MtpFunctionalModeDefault));
            R_TRY(db.AddArray(SupportedOperationCodes, util::size(SupportedOperationCodes)));
            R_TRY(db.AddArray(SupportedEventCodes, util::size(SupportedEventCodes)));
            R_TRY(db.AddArray(SupportedDeviceProperties, util::size(SupportedDeviceProperties)));
            R_TRY(db.AddArray(SupportedCaptureFormats, util::size(SupportedCaptureFormats)));
            R_TRY(db.AddArray(SupportedPlaybackFormats, util::size(SupportedPlaybackFormats)));
            R_TRY(db.AddString(MtpDeviceManufacturer));
            R_TRY(db.AddString(MtpDeviceModel));
            R_TRY(db.AddString(GetFirmwareVersion()));
            R_TRY(db.AddString(GetSerialNumber()));

            R_SUCCEED();
        }));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::OpenSession(PtpDataParser &dp) {
        R_TRY(dp.Finalize());

        /* Close, if we're already open. */
        this->ForceCloseSession();

        /* Initialize the database. */
        m_session_open = true;
        m_object_database.Initialize(m_object_heap);

        /* Create the root storages. */
        PtpObject *object;
        R_TRY(m_object_database.CreateOrFindObject("", "", PtpGetObjectHandles_RootParent, std::addressof(object)));

        /* Register the root storages. */
        m_object_database.RegisterObject(object, StorageId_SdmcFs);

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::CloseSession(PtpDataParser &dp) {
        R_TRY(dp.Finalize());

        this->ForceCloseSession();

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetStorageIds(PtpDataParser &dp) {
        R_TRY(dp.Finalize());

        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        /* Write the storage ID array. */
        R_TRY(db.WriteVariableLengthData(m_request_header, [&] {
            R_RETURN(db.AddArray(SupportedStorageIds, util::size(SupportedStorageIds)));
        }));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetStorageInfo(PtpDataParser &dp) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));
        PtpStorageInfo storage_info(DefaultStorageInfo);

        /* Get the storage ID the client requested information for. */
        u32 storage_id;
        R_TRY(dp.Read(std::addressof(storage_id)));
        R_TRY(dp.Finalize());

        /* Get the info from fs. */
        switch (storage_id) {
            case StorageId_SdmcFs:
                {
                    s64 total_space, free_space;
                    R_TRY(m_fs.GetTotalSpace("/", std::addressof(total_space)));
                    R_TRY(m_fs.GetFreeSpace("/", std::addressof(free_space)));

                    storage_info.max_capacity         = total_space;
                    storage_info.free_space_in_bytes  = free_space;
                    storage_info.free_space_in_images = 0;
                    storage_info.storage_description  = "SD Card";
                }
                break;
            default:
                R_THROW(haze::ResultInvalidStorageId());
        }

        /* Write the storage info data. */
        R_TRY(db.WriteVariableLengthData(m_request_header, [&] () {
            R_TRY(db.Add(storage_info.storage_type));
            R_TRY(db.Add(storage_info.filesystem_type));
            R_TRY(db.Add(storage_info.access_capability));
            R_TRY(db.Add(storage_info.max_capacity));
            R_TRY(db.Add(storage_info.free_space_in_bytes));
            R_TRY(db.Add(storage_info.free_space_in_images));
            R_TRY(db.AddString(storage_info.storage_description));
            R_TRY(db.AddString(storage_info.volume_label));

            R_SUCCEED();
        }));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObjectHandles(PtpDataParser &dp) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        /* Get the object ID the client requested enumeration for. */
        u32 storage_id, object_format_code, association_object_handle;
        R_TRY(dp.Read(std::addressof(storage_id)));
        R_TRY(dp.Read(std::addressof(object_format_code)));
        R_TRY(dp.Read(std::addressof(association_object_handle)));
        R_TRY(dp.Finalize());

        /* Handle top-level requests. */
        if (storage_id == PtpGetObjectHandles_AllStorage) {
            storage_id = StorageId_SdmcFs;
        }

        /* Rewrite requests for enumerating storage directories. */
        if (association_object_handle == PtpGetObjectHandles_RootParent) {
            association_object_handle = storage_id;
        }

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(association_object_handle);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Try to read the object as a directory. */
        FsDir dir;
        R_TRY(m_fs.OpenDirectory(obj->GetName(), FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, std::addressof(dir)));

        /* Ensure we maintain a clean state on exit. */
        ON_SCOPE_EXIT { m_fs.CloseDirectory(std::addressof(dir)); };

        /* Count how many entries are in the directory. */
        s64 entry_count = 0;
        R_TRY(m_fs.GetDirectoryEntryCount(std::addressof(dir), std::addressof(entry_count)));

        /* Begin writing. */
        R_TRY(db.AddDataHeader(m_request_header, sizeof(u32) + (entry_count * sizeof(u32))));
        R_TRY(db.Add(static_cast<u32>(entry_count)));

        /* Enumerate the directory, writing results to the data builder as we progress. */
        /* TODO: How should we handle the directory contents changing during enumeration? */
        /* Is this even feasible to handle? */
        while (true) {
            /* Get the next batch. */
            s64 read_count = 0;
            R_TRY(m_fs.ReadDirectory(std::addressof(dir), std::addressof(read_count), DirectoryReadSize, m_buffers->file_system_entry_buffer));

            /* Write to output. */
            for (s64 i = 0; i < read_count; i++) {
                const char *name = m_buffers->file_system_entry_buffer[i].name;
                u32 handle;

                R_TRY(m_object_database.CreateAndRegisterObjectId(obj->GetName(), name, obj->GetObjectId(), std::addressof(handle)));
                R_TRY(db.Add(handle));
            }

            /* If we read fewer than the batch size, we're done. */
            if (read_count < DirectoryReadSize) {
                break;
            }
        }

        /* Flush the data response. */
        R_TRY(db.Commit());

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObjectInfo(PtpDataParser &dp) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        /* Get the object ID the client requested info for. */
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Build info about the object. */
        PtpObjectInfo object_info(DefaultObjectInfo);

        if (object_id == StorageId_SdmcFs) {
            /* The SD Card directory has some special properties. */
            object_info.object_format    = PtpObjectFormatCode_Association;
            object_info.association_type = PtpAssociationType_GenericFolder;
            object_info.filename         = "SD Card";
        } else {
            /* Figure out what type of object this is. */
            FsDirEntryType entry_type;
            R_TRY(m_fs.GetEntryType(obj->GetName(), std::addressof(entry_type)));

            /* Get the size, if we are requesting info about a file. */
            s64 size = 0;
            if (entry_type == FsDirEntryType_File) {
                FsFile file;
                R_TRY(m_fs.OpenFile(obj->GetName(), FsOpenMode_Read, std::addressof(file)));

                /* Ensure we maintain a clean state on exit. */
                ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

                R_TRY(m_fs.GetFileSize(std::addressof(file), std::addressof(size)));
            }

            object_info.filename               = std::strrchr(obj->GetName(), '/') + 1;
            object_info.object_compressed_size = size;
            object_info.parent_object          = obj->GetParentId();

            if (entry_type == FsDirEntryType_Dir) {
                object_info.object_format    = PtpObjectFormatCode_Association;
                object_info.association_type = PtpAssociationType_GenericFolder;
            } else {
                object_info.object_format    = PtpObjectFormatCode_Undefined;
                object_info.association_type = PtpAssociationType_Undefined;
            }
        }

        /* Write the object info data. */
        R_TRY(db.WriteVariableLengthData(m_request_header, [&] () {
            R_TRY(db.Add(object_info.storage_id));
            R_TRY(db.Add(object_info.object_format));
            R_TRY(db.Add(object_info.protection_status));
            R_TRY(db.Add(object_info.object_compressed_size));
            R_TRY(db.Add(object_info.thumb_format));
            R_TRY(db.Add(object_info.thumb_compressed_size));
            R_TRY(db.Add(object_info.thumb_width));
            R_TRY(db.Add(object_info.thumb_height));
            R_TRY(db.Add(object_info.image_width));
            R_TRY(db.Add(object_info.image_height));
            R_TRY(db.Add(object_info.image_depth));
            R_TRY(db.Add(object_info.parent_object));
            R_TRY(db.Add(object_info.association_type));
            R_TRY(db.Add(object_info.association_desc));
            R_TRY(db.Add(object_info.sequence_number));
            R_TRY(db.AddString(object_info.filename));
            R_TRY(db.AddString(object_info.capture_date));
            R_TRY(db.AddString(object_info.modification_date));
            R_TRY(db.AddString(object_info.keywords));

            R_SUCCEED();
        }));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObject(PtpDataParser &dp) {
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        /* Get the object ID the client requested. */
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
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
        s64 size = 0;
        R_TRY(m_fs.GetFileSize(std::addressof(file), std::addressof(size)));

        /* Send the header and file size. */
        R_TRY(db.AddDataHeader(m_request_header, size));

        /* Begin reading the file, writing data to the builder as we progress. */
        s64 offset = 0;
        while (true) {
            /* Get the next batch. */
            u64 bytes_read;
            R_TRY(m_fs.ReadFile(std::addressof(file), offset, m_buffers->file_system_data_buffer, FsBufferSize, FsReadOption_None, std::addressof(bytes_read)));

            offset += bytes_read;

            /* Write to output. */
            R_TRY(db.AddBuffer(m_buffers->file_system_data_buffer, bytes_read));

            /* If we read fewer bytes than the batch size, we're done. */
            if (bytes_read < FsBufferSize) {
                break;
            }
        }

        /* Flush the data response. */
        R_TRY(db.Commit());

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::SendObjectInfo(PtpDataParser &rdp) {
        /* Get the storage ID and parent object and flush the request packet. */
        u32 storage_id, parent_object;
        R_TRY(rdp.Read(std::addressof(storage_id)));
        R_TRY(rdp.Read(std::addressof(parent_object)));
        R_TRY(rdp.Finalize());

        PtpDataParser dp(m_buffers->usb_bulk_read_buffer, std::addressof(m_usb_server));
        PtpObjectInfo info(DefaultObjectInfo);

        /* Ensure we have a data header. */
        PtpUsbBulkContainer data_header;
        R_TRY(dp.Read(std::addressof(data_header)));
        R_UNLESS(data_header.type == PtpUsbBulkContainerType_Data,  haze::ResultUnknownRequestType());
        R_UNLESS(data_header.code == m_request_header.code,         haze::ResultOperationNotSupported());
        R_UNLESS(data_header.trans_id == m_request_header.trans_id, haze::ResultOperationNotSupported());

        /* Read in the object info. */
        R_TRY(dp.Read(std::addressof(info.storage_id)));
        R_TRY(dp.Read(std::addressof(info.object_format)));
        R_TRY(dp.Read(std::addressof(info.protection_status)));
        R_TRY(dp.Read(std::addressof(info.object_compressed_size)));
        R_TRY(dp.Read(std::addressof(info.thumb_format)));
        R_TRY(dp.Read(std::addressof(info.thumb_compressed_size)));
        R_TRY(dp.Read(std::addressof(info.thumb_width)));
        R_TRY(dp.Read(std::addressof(info.thumb_height)));
        R_TRY(dp.Read(std::addressof(info.image_width)));
        R_TRY(dp.Read(std::addressof(info.image_height)));
        R_TRY(dp.Read(std::addressof(info.image_depth)));
        R_TRY(dp.Read(std::addressof(info.parent_object)));
        R_TRY(dp.Read(std::addressof(info.association_type)));
        R_TRY(dp.Read(std::addressof(info.association_desc)));
        R_TRY(dp.Read(std::addressof(info.sequence_number)));
        R_TRY(dp.ReadString(m_buffers->filename_string_buffer));
        R_TRY(dp.ReadString(m_buffers->capture_date_string_buffer));
        R_TRY(dp.ReadString(m_buffers->modification_date_string_buffer));
        R_TRY(dp.ReadString(m_buffers->keywords_string_buffer));
        R_TRY(dp.Finalize());

        /* Rewrite requests for creating in storage directories. */
        if (parent_object == PtpGetObjectHandles_RootParent) {
            parent_object = storage_id;
        }

        /* Check if we know about the parent object. If we don't, it's an error. */
        auto * const parentobj = m_object_database.GetObjectById(parent_object);
        R_UNLESS(parentobj != nullptr, haze::ResultInvalidObjectId());

        /* Make a new object with the intended name. */
        PtpNewObjectInfo new_object_info;
        new_object_info.storage_id       = StorageId_SdmcFs;
        new_object_info.parent_object_id = parent_object == storage_id ? 0 : parent_object;

        /* Create the object in the database. */
        PtpObject *obj;
        R_TRY(m_object_database.CreateOrFindObject(parentobj->GetName(), m_buffers->filename_string_buffer, parentobj->GetObjectId(), std::addressof(obj)));

        /* Ensure we maintain a clean state on failure. */
        ON_RESULT_FAILURE { m_object_database.DeleteObject(obj); };

        /* Register the object with a new ID. */
        m_object_database.RegisterObject(obj);
        new_object_info.object_id = obj->GetObjectId();

        /* Create the object on the filesystem. */
        if (info.object_format == PtpObjectFormatCode_Association) {
            R_TRY(m_fs.CreateDirectory(obj->GetName()));
            m_send_object_id = 0;
        } else {
            R_TRY(m_fs.CreateFile(obj->GetName(), 0, 0));
            m_send_object_id = new_object_info.object_id;
        }

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok, new_object_info));
    }

    Result PtpResponder::SendObject(PtpDataParser &rdp) {
        /* Reset SendObject object ID on exit. */
        ON_SCOPE_EXIT { m_send_object_id = 0; };

        R_TRY(rdp.Finalize());

        PtpDataParser dp(m_buffers->usb_bulk_read_buffer, std::addressof(m_usb_server));

        /* Ensure we have a data header. */
        PtpUsbBulkContainer data_header;
        R_TRY(dp.Read(std::addressof(data_header)));
        R_UNLESS(data_header.type == PtpUsbBulkContainerType_Data,  haze::ResultUnknownRequestType());
        R_UNLESS(data_header.code == m_request_header.code,         haze::ResultOperationNotSupported());
        R_UNLESS(data_header.trans_id == m_request_header.trans_id, haze::ResultOperationNotSupported());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(m_send_object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Lock the object as a file. */
        FsFile file;
        R_TRY(m_fs.OpenFile(obj->GetName(), FsOpenMode_Write | FsOpenMode_Append, std::addressof(file)));

        /* Ensure we maintain a clean state on exit. */
        ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

        /* Truncate the file after locking for write. */
        s64 offset = 0;
        R_TRY(m_fs.SetFileSize(std::addressof(file), 0));

        /* Expand to the needed size. */
        if (data_header.length > sizeof(PtpUsbBulkContainer)) {
            R_TRY(m_fs.SetFileSize(std::addressof(file), data_header.length - sizeof(PtpUsbBulkContainer)));
        }

        /* Begin writing to the filesystem. */
        while (true) {
            /* Read as many bytes as we can. */
            u32 bytes_received;
            const Result read_res = dp.ReadBuffer(m_buffers->file_system_data_buffer, FsBufferSize, std::addressof(bytes_received));

            /* Write to the file. */
            R_TRY(m_fs.WriteFile(std::addressof(file), offset, m_buffers->file_system_data_buffer, bytes_received, 0));

            offset += bytes_received;

            /* If we received fewer bytes than the batch size, we're done. */
            if (haze::ResultEndOfTransmission::Includes(read_res)) {
                break;
            }

            R_TRY(read_res);
        }

        /* Truncate the file to the received size. */
        R_TRY(m_fs.SetFileSize(std::addressof(file), offset));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::DeleteObject(PtpDataParser &dp) {
        /* Get the object ID and flush the request packet. */
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Finalize());

        /* Disallow deleting the storage root. */
        R_UNLESS(object_id != StorageId_SdmcFs, haze::ResultInvalidObjectId());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Figure out what type of object this is. */
        FsDirEntryType entry_type;
        R_TRY(m_fs.GetEntryType(obj->GetName(), std::addressof(entry_type)));

        /* Remove the object from the filesystem. */
        if (entry_type == FsDirEntryType_Dir) {
            R_TRY(m_fs.DeleteDirectoryRecursively(obj->GetName()));
        } else {
            R_TRY(m_fs.DeleteFile(obj->GetName()));
        }

        /* Remove the object from the database. */
        m_object_database.DeleteObject(obj);

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObjectPropsSupported(PtpDataParser &dp) {
        R_TRY(dp.Finalize());

        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        /* Write information about all object properties we can support. */
        R_TRY(db.WriteVariableLengthData(m_request_header, [&] {
            R_RETURN(db.AddArray(SupportedObjectProperties, util::size(SupportedObjectProperties)));
        }));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObjectPropDesc(PtpDataParser &dp) {
        PtpObjectPropertyCode property_code;
        u16 object_format;

        R_TRY(dp.Read(std::addressof(property_code)));
        R_TRY(dp.Read(std::addressof(object_format)));
        R_TRY(dp.Finalize());

        /* Ensure we have a valid property code before continuing. */
        R_UNLESS(IsSupportedObjectPropertyCode(property_code), haze::ResultUnknownPropertyCode());

        /* Begin writing information about the property code. */
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        R_TRY(db.WriteVariableLengthData(m_request_header, [&] {
            R_TRY(db.Add(property_code));

            /* Each property code corresponds to a different pattern, which contains the data type, */
            /* whether the property can be set for an object, and the default value of the property. */
            switch (property_code) {
                case PtpObjectPropertyCode_PersistentUniqueObjectIdentifier:
                    {
                        R_TRY(db.Add(PtpDataTypeCode_U128));
                        R_TRY(db.Add(PtpPropertyGetSetFlag_Get));
                        R_TRY(db.Add<u128>(0));
                    }
                case PtpObjectPropertyCode_ObjectSize:
                    {
                        R_TRY(db.Add(PtpDataTypeCode_U64));
                        R_TRY(db.Add(PtpPropertyGetSetFlag_Get));
                        R_TRY(db.Add<u64>(0));
                    }
                    break;
                case PtpObjectPropertyCode_StorageId:
                case PtpObjectPropertyCode_ParentObject:
                    {
                        R_TRY(db.Add(PtpDataTypeCode_U32));
                        R_TRY(db.Add(PtpPropertyGetSetFlag_Get));
                        R_TRY(db.Add(StorageId_SdmcFs));
                    }
                    break;
                case PtpObjectPropertyCode_ObjectFormat:
                    {
                        R_TRY(db.Add(PtpDataTypeCode_U16));
                        R_TRY(db.Add(PtpPropertyGetSetFlag_Get));
                        R_TRY(db.Add(PtpObjectFormatCode_Undefined));
                    }
                    break;
                case PtpObjectPropertyCode_ObjectFileName:
                    {
                        R_TRY(db.Add(PtpDataTypeCode_String));
                        R_TRY(db.Add(PtpPropertyGetSetFlag_GetSet));
                        R_TRY(db.AddString(""));
                    }
                    break;
                HAZE_UNREACHABLE_DEFAULT_CASE();
            }

            /* Group code is a required part of the response, but doesn't seem to be used for anything. */
            R_TRY(db.Add(PtpPropertyGroupCode_Default));

            /* We don't use the form flag. */
            R_TRY(db.Add(PtpPropertyFormFlag_None));

            R_SUCCEED();
        }));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObjectPropValue(PtpDataParser &dp) {
        u32 object_id;
        PtpObjectPropertyCode property_code;

        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Read(std::addressof(property_code)));
        R_TRY(dp.Finalize());

        /* Disallow renaming the storage root. */
        R_UNLESS(object_id != StorageId_SdmcFs, haze::ResultInvalidObjectId());

        /* Ensure we have a valid property code before continuing. */
        R_UNLESS(IsSupportedObjectPropertyCode(property_code), haze::ResultUnknownPropertyCode());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* Define helper for getting the object type. */
        const auto GetObjectType = [&] (FsDirEntryType *out_entry_type) {
            R_RETURN(m_fs.GetEntryType(obj->GetName(), out_entry_type));
        };

        /* Define helper for getting the object size. */
        const auto GetObjectSize = [&] (s64 *out_size) {
            *out_size = 0;

            /* Check if this is a directory. */
            FsDirEntryType entry_type;
            R_TRY(GetObjectType(std::addressof(entry_type)));

            /* If it is, we're done. */
            R_SUCCEED_IF(entry_type == FsDirEntryType_Dir);

            /* Otherwise, open as a file. */
            FsFile file;
            R_TRY(m_fs.OpenFile(obj->GetName(), FsOpenMode_Read, std::addressof(file)));

            /* Ensure we maintain a clean state on exit. */
            ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

            R_RETURN(m_fs.GetFileSize(std::addressof(file), out_size));
        };

        /* Begin writing the requested object property. */
        PtpDataBuilder db(m_buffers->usb_bulk_write_buffer, std::addressof(m_usb_server));

        R_TRY(db.WriteVariableLengthData(m_request_header, [&] {
            switch (property_code) {
                case PtpObjectPropertyCode_PersistentUniqueObjectIdentifier:
                    {
                        R_TRY(db.Add<u128>(object_id));
                    }
                    break;
                case PtpObjectPropertyCode_ObjectSize:
                    {
                        s64 size;
                        R_TRY(GetObjectSize(std::addressof(size)));
                        R_TRY(db.Add<u64>(size));
                    }
                    break;
                case PtpObjectPropertyCode_StorageId:
                    {
                        R_TRY(db.Add(StorageId_SdmcFs));
                    }
                    break;
                case PtpObjectPropertyCode_ParentObject:
                    {
                        R_TRY(db.Add(obj->GetParentId()));
                    }
                    break;
                case PtpObjectPropertyCode_ObjectFormat:
                    {
                        FsDirEntryType entry_type;
                        R_TRY(GetObjectType(std::addressof(entry_type)));
                        R_TRY(db.Add(entry_type == FsDirEntryType_File ? PtpObjectFormatCode_Undefined : PtpObjectFormatCode_Association));
                    }
                    break;
                case PtpObjectPropertyCode_ObjectFileName:
                    {
                        R_TRY(db.AddString(std::strrchr(obj->GetName(), '/') + 1));
                    }
                    break;
                HAZE_UNREACHABLE_DEFAULT_CASE();
            }

            R_SUCCEED();
        }));

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::SetObjectPropValue(PtpDataParser &rdp) {
        u32 object_id;
        PtpObjectPropertyCode property_code;

        R_TRY(rdp.Read(std::addressof(object_id)));
        R_TRY(rdp.Read(std::addressof(property_code)));
        R_TRY(rdp.Finalize());

        PtpDataParser dp(m_buffers->usb_bulk_read_buffer, std::addressof(m_usb_server));

        /* Ensure we have a data header. */
        PtpUsbBulkContainer data_header;
        R_TRY(dp.Read(std::addressof(data_header)));
        R_UNLESS(data_header.type == PtpUsbBulkContainerType_Data,      haze::ResultUnknownRequestType());
        R_UNLESS(data_header.code == m_request_header.code,             haze::ResultOperationNotSupported());
        R_UNLESS(data_header.trans_id == m_request_header.trans_id,     haze::ResultOperationNotSupported());

        /* Ensure we have a valid property code before continuing. */
        R_UNLESS(property_code == PtpObjectPropertyCode_ObjectFileName, haze::ResultUnknownPropertyCode());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const obj = m_object_database.GetObjectById(object_id);
        R_UNLESS(obj != nullptr, haze::ResultInvalidObjectId());

        /* We are reading a file name. */
        R_TRY(dp.ReadString(m_buffers->filename_string_buffer));
        R_TRY(dp.Finalize());

        /* Ensure we can actually process the new name. */
        const bool is_empty         = m_buffers->filename_string_buffer[0] == '\x00';
        const bool contains_slashes = std::strchr(m_buffers->filename_string_buffer, '/') != nullptr;
        R_UNLESS(!is_empty && !contains_slashes, haze::ResultInvalidPropertyValue());

        /* Add a new object in the database with the new name. */
        PtpObject *newobj;
        {
            /* Find the last path separator in the existing object name. */
            char *pathsep = std::strrchr(obj->m_name, '/');
            HAZE_ASSERT(pathsep != nullptr);

            /* Temporarily mark the path separator as null to facilitate processing. */
            *pathsep = '\x00';
            ON_SCOPE_EXIT { *pathsep = '/'; };

            R_TRY(m_object_database.CreateOrFindObject(obj->GetName(), m_buffers->filename_string_buffer, obj->GetParentId(), std::addressof(newobj)));
        }

        {
            /* Ensure we maintain a clean state on failure. */
            ON_RESULT_FAILURE {
                if (!newobj->GetIsRegistered()) {
                    /* Only delete if the object was not registered. */
                    /* Otherwise, we would remove an object that still exists. */
                    m_object_database.DeleteObject(newobj);
                }
            };

            /* Get the old object type. */
            FsDirEntryType entry_type;
            R_TRY(m_fs.GetEntryType(obj->GetName(), std::addressof(entry_type)));

            /* Attempt to rename the object on the filesystem. */
            if (entry_type == FsDirEntryType_Dir) {
                R_TRY(m_fs.RenameDirectory(obj->GetName(), newobj->GetName()));
            } else {
                R_TRY(m_fs.RenameFile(obj->GetName(), newobj->GetName()));
            }
        }

        /* Unregister and free the old object. */
        m_object_database.DeleteObject(obj);

        /* Register the new object. */
        m_object_database.RegisterObject(newobj, object_id);

        /* Write the success response. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }
}
