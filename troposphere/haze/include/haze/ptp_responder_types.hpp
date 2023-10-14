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

#include <haze.hpp>

namespace haze {

    constexpr UsbCommsInterfaceInfo MtpInterfaceInfo = {
        .bInterfaceClass    = 0x06,
        .bInterfaceSubClass = 0x01,
        .bInterfaceProtocol = 0x01,
    };

    /* This is a VID:PID recognized by libmtp. */
    constexpr u16 SwitchMtpIdVendor  = 0x057e;
    constexpr u16 SwitchMtpIdProduct = 0x201d;

    /* Constants used for MTP GetDeviceInfo response. */
    constexpr u16 MtpStandardVersion       = 100;
    constexpr u32 MtpVendorExtensionId     = 6;
    constexpr auto MtpVendorExtensionDesc  = "microsoft.com: 1.0;";
    constexpr u16 MtpFunctionalModeDefault = 0;
    constexpr auto MtpDeviceManufacturer   = "Nintendo";
    constexpr auto MtpDeviceModel          = "Nintendo Switch";

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
        PtpOperationCode_MtpGetObjectPropsSupported,
        PtpOperationCode_MtpGetObjectPropDesc,
        PtpOperationCode_MtpGetObjectPropValue,
        PtpOperationCode_MtpSetObjectPropValue,
        PtpOperationCode_MtpGetObjPropList,
        PtpOperationCode_AndroidGetPartialObject64,
        PtpOperationCode_AndroidSendPartialObject,
        PtpOperationCode_AndroidTruncateObject,
        PtpOperationCode_AndroidBeginEditObject,
        PtpOperationCode_AndroidEndEditObject,
    };

    constexpr const PtpEventCode SupportedEventCodes[]                = { /* ... */ };
    constexpr const PtpDevicePropertyCode SupportedDeviceProperties[] = { /* ... */ };
    constexpr const PtpObjectFormatCode SupportedCaptureFormats[]     = { /* ... */ };

    constexpr const PtpObjectFormatCode SupportedPlaybackFormats[] = {
        PtpObjectFormatCode_Undefined,
        PtpObjectFormatCode_Association,
    };

    constexpr const PtpObjectPropertyCode SupportedObjectProperties[] = {
        PtpObjectPropertyCode_StorageId,
        PtpObjectPropertyCode_ObjectFormat,
        PtpObjectPropertyCode_ObjectSize,
        PtpObjectPropertyCode_ObjectFileName,
        PtpObjectPropertyCode_ParentObject,
        PtpObjectPropertyCode_PersistentUniqueObjectIdentifier,
    };

    constexpr bool IsSupportedObjectPropertyCode(PtpObjectPropertyCode c) {
        for (size_t i = 0; i < util::size(SupportedObjectProperties); i++) {
            if (SupportedObjectProperties[i] == c) {
                return true;
            }
        }

        return false;
    }

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

    constexpr u32 UsbBulkPacketBufferSize = 1_MB;
    constexpr u64 FsBufferSize = UsbBulkPacketBufferSize;
    constexpr s64 DirectoryReadSize = 32;

    struct PtpBuffers {
        char filename_string_buffer[PtpStringMaxLength + 1];
        char capture_date_string_buffer[PtpStringMaxLength + 1];
        char modification_date_string_buffer[PtpStringMaxLength + 1];
        char keywords_string_buffer[PtpStringMaxLength + 1];

        FsDirectoryEntry file_system_entry_buffer[DirectoryReadSize];
        u8 file_system_data_buffer[FsBufferSize];

        alignas(4_KB) u8 usb_bulk_write_buffer[UsbBulkPacketBufferSize];
        alignas(4_KB) u8 usb_bulk_read_buffer[UsbBulkPacketBufferSize];
    };

}
