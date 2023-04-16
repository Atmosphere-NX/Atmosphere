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

namespace haze {

    namespace {

        constexpr UsbCommsInterfaceInfo MtpInterfaceInfo = {
            .bInterfaceClass = 0x06,
            .bInterfaceSubClass = 0x01,
            .bInterfaceProtocol = 0x01,
        };

        /* This is a VID:PID recognized by libmtp. */
        constexpr u16 SwitchMtpIdVendor = 0x057e;
        constexpr u16 SwitchMtpIdProduct = 0x201d;

        /* Constants used for MTP GetDeviceInfo response. */
        constexpr u16 MtpStandardVersion = 100;
        constexpr u32 MtpVendorExtensionId = 6;
        constexpr auto MtpVendorExtensionDesc = "microsoft.com: 1.0;";
        constexpr u16 MtpFunctionalModeDefault = 0;
        constexpr auto MtpDeviceManufacturer = "Nintendo";
        constexpr auto MtpDeviceModel = "Switch";
        constexpr auto MtpDeviceVersion = "1.0.0";
        constexpr auto MtpDeviceSerialNumber = "SerialNumber";

        enum StorageId : u32 {
            StorageId_SdmcFs = 0xffffffffu - 1,
        };

        constexpr PtpOperationCode SupportedOperationCodes[] = {
            PtpOperationCode_GetDeviceInfo,
            PtpOperationCode_OpenSession,
            PtpOperationCode_CloseSession,
            PtpOperationCode_GetStorageIds,
            PtpOperationCode_GetStorageInfo,
            PtpOperationCode_GetObjectHandles,
            PtpOperationCode_GetObjectInfo,
            PtpOperationCode_GetObject,
            PtpOperationCode_SendObjectInfo,
            PtpOperationCode_SendObject,
            PtpOperationCode_DeleteObject,
        };

        constexpr const PtpEventCode SupportedEventCodes[]             = { /* ... */};
        constexpr const PtpDevicePropertyCode SupportedPropertyCodes[] = { /* ...*/ };
        constexpr const PtpObjectFormatCode SupportedCaptureFormats[]  = { /* ...*/ };
        constexpr const PtpObjectFormatCode SupportedPlaybackFormats[] = { /* ...*/ };

        constexpr const StorageId SupportedStorageIds[] = {
            StorageId_SdmcFs,
        };

        struct PtpStorageInfo {
            PtpStorageType storage_type;
            PtpFilesystemType filesystem_type;
            PtpAccessCapability access_capability;
            u64 max_capacity;
            u64 free_space_in_bytes;
            u32 free_space_in_images;
            const char *storage_description;
            const char *volume_label;
        };

        constexpr PtpStorageInfo DefaultStorageInfo = {
            .storage_type         = PtpStorageType_FixedRam,
            .filesystem_type      = PtpFilesystemType_GenericHierarchical,
            .access_capability    = PtpAccessCapability_ReadWrite,
            .max_capacity         = 0,
            .free_space_in_bytes  = 0,
            .free_space_in_images = 0,
            .storage_description  = "",
            .volume_label         = "",
        };

        struct PtpObjectInfo {
            StorageId storage_id;
            PtpObjectFormatCode object_format;
            PtpProtectionStatus protection_status;
            u32 object_compressed_size;
            u16 thumb_format;
            u32 thumb_compressed_size;
            u32 thumb_width;
            u32 thumb_height;
            u32 image_width;
            u32 image_height;
            u32 image_depth;
            u32 parent_object;
            PtpAssociationType association_type;
            u32 association_desc;
            u32 sequence_number;
            const char *filename;
            const char *capture_date;
            const char *modification_date;
            const char *keywords;
        };

        constexpr PtpObjectInfo DefaultObjectInfo = {
            .storage_id             = StorageId_SdmcFs,
            .object_format          = {},
            .protection_status      = PtpProtectionStatus_NoProtection,
            .object_compressed_size = 0,
            .thumb_format           = 0,
            .thumb_compressed_size  = 0,
            .thumb_width            = 0,
            .thumb_height           = 0,
            .image_width            = 0,
            .image_height           = 0,
            .image_depth            = 0,
            .parent_object          = PtpGetObjectHandles_RootParent,
            .association_type       = PtpAssociationType_Undefined,
            .association_desc       = 0,
            .sequence_number        = 0,
            .filename               = nullptr,
            .capture_date           = "",
            .modification_date      = "",
            .keywords               = "",
        };

        constexpr s64 DirectoryReadSize = 32;
        constexpr u64 FsBufferSize = haze::UsbBulkPacketBufferSize;

        constinit char g_filename_str[PtpStringMaxLength + 1];
        constinit char g_capture_date_str[PtpStringMaxLength + 1];
        constinit char g_modification_date_str[PtpStringMaxLength + 1];
        constinit char g_keywords_str[PtpStringMaxLength + 1];

        constinit FsDirectoryEntry g_dir_entries[DirectoryReadSize] = {};
        constinit u8 g_fs_buffer[FsBufferSize] = {};

        alignas(4_KB) constinit u8 g_bulk_write_buffer[haze::UsbBulkPacketBufferSize] = {};
        alignas(4_KB) constinit u8 g_bulk_read_buffer[haze::UsbBulkPacketBufferSize] = {};

    }

    Result PtpResponder::Initialize(EventReactor *reactor, PtpObjectHeap *object_heap) {
        m_object_heap = object_heap;

        /* Configure fs proxy. */
        m_fs.Initialize(reactor, fsdevGetDeviceFileSystem("sdmc"));

        R_RETURN(m_usb_server.Initialize(std::addressof(MtpInterfaceInfo), SwitchMtpIdVendor, SwitchMtpIdProduct, reactor));
    }

    void PtpResponder::Finalize() {
        m_usb_server.Finalize();
        m_fs.Finalize();
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
            R_CATCH(haze::ResultStorageNotFound) {
                R_TRY(this->WriteResponse(PtpResponseCode_InvalidStorageId));
            }
            R_CATCH(haze::ResultObjectNotFound) {
                R_TRY(this->WriteResponse(PtpResponseCode_InvalidObjectHandle));
            }
            R_CATCH(fs::ResultPathNotFound, fs::ResultPathAlreadyExists, fs::ResultTargetLocked, fs::ResultDirectoryNotEmpty, fs::ResultNotEnoughFreeSpaceSdCard) {
                R_TRY(this->WriteResponse(PtpResponseCode_GeneralError));
            }
            R_CATCH_ALL() {
                R_THROW(haze::ResultGeneralFailure());
            }
        } R_END_TRY_CATCH;

        R_SUCCEED();
    }

    Result PtpResponder::HandleRequestImpl() {
        PtpDataParser dp(g_bulk_read_buffer, std::addressof(m_usb_server));
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
            case PtpOperationCode_GetDeviceInfo:    R_RETURN(this->GetDeviceInfo(dp));    break;
            case PtpOperationCode_OpenSession:      R_RETURN(this->OpenSession(dp));      break;
            case PtpOperationCode_CloseSession:     R_RETURN(this->CloseSession(dp));     break;
            case PtpOperationCode_GetStorageIds:    R_RETURN(this->GetStorageIds(dp));    break;
            case PtpOperationCode_GetStorageInfo:   R_RETURN(this->GetStorageInfo(dp));   break;
            case PtpOperationCode_GetObjectHandles: R_RETURN(this->GetObjectHandles(dp)); break;
            case PtpOperationCode_GetObjectInfo:    R_RETURN(this->GetObjectInfo(dp));    break;
            case PtpOperationCode_GetObject:        R_RETURN(this->GetObject(dp));        break;
            case PtpOperationCode_SendObjectInfo:   R_RETURN(this->SendObjectInfo(dp));   break;
            case PtpOperationCode_SendObject:       R_RETURN(this->SendObject(dp));       break;
            case PtpOperationCode_DeleteObject:     R_RETURN(this->DeleteObject(dp));     break;
            default:                                R_THROW(haze::ResultOperationNotSupported());
        }
    }

    void PtpResponder::ForceCloseSession() {
        if (m_session_open) {
            m_session_open = false;
            m_object_database.Finalize();
        }
    }

    template <typename Data>
    Result PtpResponder::WriteResponse(PtpResponseCode code, Data &&data) {
        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));
        R_TRY(db.AddResponseHeader(m_request_header, code, sizeof(Data)));
        R_TRY(db.Add(data));
        R_RETURN(db.Commit());
    }

    Result PtpResponder::WriteResponse(PtpResponseCode code) {
        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));
        R_TRY(db.AddResponseHeader(m_request_header, code, 0));
        R_RETURN(db.Commit());
    }

    Result PtpResponder::GetDeviceInfo(PtpDataParser &dp) {
        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));

        R_TRY(db.WriteVariableLengthData(m_request_header, [&] () {
            R_TRY(db.Add(MtpStandardVersion));
            R_TRY(db.Add(MtpVendorExtensionId));
            R_TRY(db.Add(MtpStandardVersion));
            R_TRY(db.AddString(MtpVendorExtensionDesc));
            R_TRY(db.Add(MtpFunctionalModeDefault));
            R_TRY(db.AddArray(SupportedOperationCodes, util::size(SupportedOperationCodes)));
            R_TRY(db.AddArray(SupportedEventCodes, util::size(SupportedEventCodes)));
            R_TRY(db.AddArray(SupportedPropertyCodes, util::size(SupportedPropertyCodes)));
            R_TRY(db.AddArray(SupportedCaptureFormats, util::size(SupportedCaptureFormats)));
            R_TRY(db.AddArray(SupportedPlaybackFormats, util::size(SupportedPlaybackFormats)));
            R_TRY(db.AddString(MtpDeviceManufacturer));
            R_TRY(db.AddString(MtpDeviceModel));
            R_TRY(db.AddString(MtpDeviceVersion));
            R_TRY(db.AddString(MtpDeviceSerialNumber));

            R_SUCCEED();
        }));

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::OpenSession(PtpDataParser &dp) {
        R_TRY(dp.Finalize());

        /* Close, if we're already open. */
        this->ForceCloseSession();

        /* Initialize the database with hardcoded storage IDs. */
        m_session_open = true;
        m_object_database.Initialize(m_object_heap);
        R_TRY(m_object_database.AddObjectId("", "", nullptr, PtpGetObjectHandles_RootParent, StorageId_SdmcFs));

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::CloseSession(PtpDataParser &dp) {
        R_TRY(dp.Finalize());

        this->ForceCloseSession();

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetStorageIds(PtpDataParser &dp) {
        R_TRY(dp.Finalize());

        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));

        R_TRY(db.WriteVariableLengthData(m_request_header, [&] {
            R_RETURN(db.AddArray(SupportedStorageIds, util::size(SupportedStorageIds)));
        }));

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetStorageInfo(PtpDataParser &dp) {
        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));
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

                    storage_info.max_capacity = total_space;
                    storage_info.free_space_in_bytes = free_space;
                    storage_info.free_space_in_images = 0;
                    storage_info.storage_description = "SD Card";
                }
                break;
            default:
                R_THROW(haze::ResultStorageNotFound());
        }

        /* Write the result. */
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

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObjectHandles(PtpDataParser &dp) {
        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));

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
        auto * const fileobj = m_object_database.GetObject(association_object_handle);
        R_UNLESS(fileobj != nullptr, haze::ResultObjectNotFound());

        /* Try to read the object as a directory. */
        FsDir dir;
        R_TRY(m_fs.OpenDirectory(fileobj->GetName(), FsDirOpenMode_ReadDirs | FsDirOpenMode_ReadFiles, std::addressof(dir)));

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
            R_TRY(m_fs.ReadDirectory(std::addressof(dir), std::addressof(read_count), DirectoryReadSize, g_dir_entries));

            /* Write to output. */
            for (s64 i = 0; i < read_count; i++) {
                u32 handle;
                R_TRY(m_object_database.AddObjectId(fileobj->GetName(), g_dir_entries[i].name, std::addressof(handle), fileobj->GetObjectId()));
                R_TRY(db.Add(handle));
            }

            /* If we read fewer than the batch size, we're done. */
            if (read_count < DirectoryReadSize) {
                break;
            }
        }

        R_TRY(db.Commit());

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObjectInfo(PtpDataParser &dp) {
        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));

        /* Get the object ID the client requested info for. */
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const fileobj = m_object_database.GetObject(object_id);
        R_UNLESS(fileobj != nullptr, haze::ResultObjectNotFound());

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
            R_TRY(m_fs.GetEntryType(fileobj->GetName(), std::addressof(entry_type)));

            /* Get the size, if we are requesting info about a file. */
            s64 size = 0;
            if (entry_type == FsDirEntryType_File) {
                FsFile file;
                R_TRY(m_fs.OpenFile(fileobj->GetName(), FsOpenMode_Read, std::addressof(file)));

                /* Ensure we maintain a clean state on exit. */
                ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

                R_TRY(m_fs.GetFileSize(std::addressof(file), std::addressof(size)));
            }

            object_info.filename               = std::strrchr(fileobj->GetName(), '/') + 1;
            object_info.object_compressed_size = size;
            object_info.parent_object          = fileobj->GetParentId();

            if (entry_type == FsDirEntryType_Dir) {
                object_info.object_format    = PtpObjectFormatCode_Association;
                object_info.association_type = PtpAssociationType_GenericFolder;
            } else {
                object_info.object_format    = PtpObjectFormatCode_Undefined;
                object_info.association_type = PtpAssociationType_Undefined;
            }
        }

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

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::GetObject(PtpDataParser &dp) {
        PtpDataBuilder db(g_bulk_write_buffer, std::addressof(m_usb_server));

        /* Get the object ID the client requested. */
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const fileobj = m_object_database.GetObject(object_id);
        R_UNLESS(fileobj != nullptr, haze::ResultObjectNotFound());

        /* Lock the object as a file. */
        FsFile file;
        R_TRY(m_fs.OpenFile(fileobj->GetName(), FsOpenMode_Read, std::addressof(file)));

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
            size_t bytes_read;
            R_TRY(m_fs.ReadFile(std::addressof(file), offset, g_fs_buffer, FsBufferSize, FsReadOption_None, std::addressof(bytes_read)));

            offset += bytes_read;

            /* Write to output. */
            R_TRY(db.AddBuffer(g_fs_buffer, bytes_read));

            /* If we read fewer bytes than the batch size, we're done. */
            if (bytes_read < FsBufferSize) {
                break;
            }
        }

        R_TRY(db.Commit());

        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::SendObjectInfo(PtpDataParser &rdp) {
        u32 storage_id, parent_object;
        R_TRY(rdp.Read(std::addressof(storage_id)));
        R_TRY(rdp.Read(std::addressof(parent_object)));
        R_TRY(rdp.Finalize());

        PtpDataParser dp(g_bulk_read_buffer, std::addressof(m_usb_server));
        PtpObjectInfo info(DefaultObjectInfo);

        /* Ensure that we have a data header. */
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
        R_TRY(dp.ReadString(g_filename_str));
        R_TRY(dp.ReadString(g_capture_date_str));
        R_TRY(dp.ReadString(g_modification_date_str));
        R_TRY(dp.ReadString(g_keywords_str));
        R_TRY(dp.Finalize());

        /* Rewrite requests for creating in storage directories. */
        if (parent_object == PtpGetObjectHandles_RootParent) {
            parent_object = storage_id;
        }

        /* Check if we know about the parent object. If we don't, it's an error. */
        auto * const parentobj = m_object_database.GetObject(parent_object);
        R_UNLESS(parentobj != nullptr, haze::ResultObjectNotFound());

        /* Make a new object with the intended name. */
        PtpNewObjectInfo new_object_info;
        new_object_info.storage_id = StorageId_SdmcFs;
        new_object_info.parent_object_id = parent_object;

        R_TRY(m_object_database.AddObjectId(parentobj->GetName(), g_filename_str, std::addressof(new_object_info.object_id), parentobj->GetObjectId()));

        /* Ensure we maintain a clean state on failure. */
        ON_RESULT_FAILURE { m_object_database.RemoveObjectId(new_object_info.object_id); };

        /* Get the object name we just built. */
        auto * const fileobj = m_object_database.GetObject(new_object_info.object_id);
        R_UNLESS(fileobj != nullptr, haze::ResultGeneralFailure());

        /* Create the object on the filesystem. */
        if (info.association_type == PtpAssociationType_GenericFolder) {
            R_TRY(m_fs.CreateDirectory(fileobj->GetName()));
            m_send_object_id = 0;
        } else {
            R_TRY(m_fs.CreateFile(fileobj->GetName(), 0, 0));
            m_send_object_id = new_object_info.object_id;
        }

        /* We succeeded. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok, new_object_info));
    }

    Result PtpResponder::SendObject(PtpDataParser &rdp) {
        /* Reset SendObject object ID on exit. */
        ON_SCOPE_EXIT { m_send_object_id = 0; };

        R_TRY(rdp.Finalize());

        PtpDataParser dp(g_bulk_read_buffer, std::addressof(m_usb_server));

        /* Ensure that we have a data header. */
        PtpUsbBulkContainer data_header;
        R_TRY(dp.Read(std::addressof(data_header)));
        R_UNLESS(data_header.type == PtpUsbBulkContainerType_Data,  haze::ResultUnknownRequestType());
        R_UNLESS(data_header.code == m_request_header.code,         haze::ResultOperationNotSupported());
        R_UNLESS(data_header.trans_id == m_request_header.trans_id, haze::ResultOperationNotSupported());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const fileobj = m_object_database.GetObject(m_send_object_id);
        R_UNLESS(fileobj != nullptr, haze::ResultObjectNotFound());

        /* Lock the object as a file. */
        FsFile file;
        R_TRY(m_fs.OpenFile(fileobj->GetName(), FsOpenMode_Write | FsOpenMode_Append, std::addressof(file)));

        /* Ensure we maintain a clean state on exit. */
        ON_SCOPE_EXIT { m_fs.CloseFile(std::addressof(file)); };

        /* Truncate the file after locking for write. */
        s64 offset = 0;
        R_TRY(m_fs.SetFileSize(std::addressof(file), 0));

        /* Begin writing. */
        while (true) {
            /* Read as many bytes as we can. */
            size_t bytes_received;
            const Result read_res = dp.ReadBuffer(g_fs_buffer, FsBufferSize, std::addressof(bytes_received));

            /* Write to the file. */
            R_TRY(m_fs.WriteFile(std::addressof(file), offset, g_fs_buffer, bytes_received, 0));

            offset += bytes_received;

            /* If we received fewer bytes than the batch size, we're done. */
            if (haze::ResultEndOfTransmission::Includes(read_res)) {
                break;
            }

            R_TRY(read_res);
        }

        /* We succeeded. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }

    Result PtpResponder::DeleteObject(PtpDataParser &dp) {
        u32 object_id;
        R_TRY(dp.Read(std::addressof(object_id)));
        R_TRY(dp.Finalize());

        /* Check if we know about the object. If we don't, it's an error. */
        auto * const fileobj = m_object_database.GetObject(object_id);
        R_UNLESS(fileobj != nullptr, haze::ResultObjectNotFound());

        /* Figure out what type of object this is. */
        FsDirEntryType entry_type;
        R_TRY(m_fs.GetEntryType(fileobj->GetName(), std::addressof(entry_type)));

        /* Remove the object from the filesystem. */
        if (entry_type == FsDirEntryType_Dir) {
            R_TRY(m_fs.DeleteDirectory(fileobj->GetName()));
        } else {
            R_TRY(m_fs.DeleteFile(fileobj->GetName()));
        }

        /* Remove the object from tracking. */
        m_object_database.RemoveObjectId(fileobj->GetObjectId());

        /* We succeeded. */
        R_RETURN(this->WriteResponse(PtpResponseCode_Ok));
    }
}
