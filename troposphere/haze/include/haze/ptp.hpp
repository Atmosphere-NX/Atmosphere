/*
 * Copyright (c) Atmosphère-NX
 * Copyright (c) libmtp project
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

#include <haze/common.hpp>

namespace haze {

    constexpr size_t PtpUsbBulkHighSpeedMaxPacketLength = 0x200;
    constexpr size_t PtpUsbBulkHeaderLength = 2 * sizeof(uint32_t) + 2 * sizeof(uint16_t);
    constexpr size_t PtpStringMaxLength = 255;

    enum PtpUsbBulkContainerType : u16 {
        PtpUsbBulkContainerType_Undefined = 0,
        PtpUsbBulkContainerType_Command = 1,
        PtpUsbBulkContainerType_Data = 2,
        PtpUsbBulkContainerType_Response = 3,
        PtpUsbBulkContainerType_Event = 4,
    };

    enum PtpOperationCode : u16 {
        PtpOperationCode_Undefined = 0x1000,
        PtpOperationCode_GetDeviceInfo = 0x1001,
        PtpOperationCode_OpenSession = 0x1002,
        PtpOperationCode_CloseSession = 0x1003,
        PtpOperationCode_GetStorageIds = 0x1004,
        PtpOperationCode_GetStorageInfo = 0x1005,
        PtpOperationCode_GetNumObjects = 0x1006,
        PtpOperationCode_GetObjectHandles = 0x1007,
        PtpOperationCode_GetObjectInfo = 0x1008,
        PtpOperationCode_GetObject = 0x1009,
        PtpOperationCode_GetThumb = 0x100A,
        PtpOperationCode_DeleteObject = 0x100B,
        PtpOperationCode_SendObjectInfo = 0x100C,
        PtpOperationCode_SendObject = 0x100D,
        PtpOperationCode_InitiateCapture = 0x100E,
        PtpOperationCode_FormatStore = 0x100F,
        PtpOperationCode_ResetDevice = 0x1010,
        PtpOperationCode_SelfTest = 0x1011,
        PtpOperationCode_SetObjectProtection = 0x1012,
        PtpOperationCode_PowerDown = 0x1013,
        PtpOperationCode_GetDevicePropDesc = 0x1014,
        PtpOperationCode_GetDevicePropValue = 0x1015,
        PtpOperationCode_SetDevicePropValue = 0x1016,
        PtpOperationCode_ResetDevicePropValue = 0x1017,
        PtpOperationCode_TerminateOpenCapture = 0x1018,
        PtpOperationCode_MoveObject = 0x1019,
        PtpOperationCode_CopyObject = 0x101A,
        PtpOperationCode_GetPartialObject = 0x101B,
        PtpOperationCode_InitiateOpenCapture = 0x101C,
        PtpOperationCode_StartEnumHandles = 0x101D,
        PtpOperationCode_EnumHandles = 0x101E,
        PtpOperationCode_StopEnumHandles = 0x101F,
        PtpOperationCode_GetVendorExtensionMaps = 0x1020,
        PtpOperationCode_GetVendorDeviceInfo = 0x1021,
        PtpOperationCode_GetResizedImageObject = 0x1022,
        PtpOperationCode_GetFilesystemManifest = 0x1023,
        PtpOperationCode_GetStreamInfo = 0x1024,
        PtpOperationCode_GetStream = 0x1025,
        PtpOperationCode_MtpGetObjectPropsSupported = 0x9801,
        PtpOperationCode_MtpGetObjectPropDesc = 0x9802,
        PtpOperationCode_MtpGetObjectPropValue = 0x9803,
        PtpOperationCode_MtpSetObjectPropValue = 0x9804,
        PtpOperationCode_MtpGetObjPropList = 0x9805,
        PtpOperationCode_MtpSetObjPropList = 0x9806,
        PtpOperationCode_MtpGetInterdependendPropdesc = 0x9807,
        PtpOperationCode_MtpSendObjectPropList = 0x9808,
        PtpOperationCode_MtpGetObjectReferences = 0x9810,
        PtpOperationCode_MtpSetObjectReferences = 0x9811,
        PtpOperationCode_MtpUpdateDeviceFirmware = 0x9812,
        PtpOperationCode_MtpSkip = 0x9820,
    };

    enum PtpResponseCode : u16 {
        PtpResponseCode_Undefined = 0x2000,
        PtpResponseCode_Ok = 0x2001,
        PtpResponseCode_GeneralError = 0x2002,
        PtpResponseCode_SessionNotOpen = 0x2003,
        PtpResponseCode_InvalidTransactionId = 0x2004,
        PtpResponseCode_OperationNotSupported = 0x2005,
        PtpResponseCode_ParameterNotSupported = 0x2006,
        PtpResponseCode_IncompleteTransfer = 0x2007,
        PtpResponseCode_InvalidStorageId = 0x2008,
        PtpResponseCode_InvalidObjectHandle = 0x2009,
        PtpResponseCode_DevicePropNotSupported = 0x200A,
        PtpResponseCode_InvalidObjectFormatCode = 0x200B,
        PtpResponseCode_StoreFull = 0x200C,
        PtpResponseCode_ObjectWriteProtected = 0x200D,
        PtpResponseCode_StoreReadOnly = 0x200E,
        PtpResponseCode_AccessDenied = 0x200F,
        PtpResponseCode_NoThumbnailPresent = 0x2010,
        PtpResponseCode_SelfTestFailed = 0x2011,
        PtpResponseCode_PartialDeletion = 0x2012,
        PtpResponseCode_StoreNotAvailable = 0x2013,
        PtpResponseCode_SpecificationByFormatUnsupported = 0x2014,
        PtpResponseCode_NoValidObjectInfo = 0x2015,
        PtpResponseCode_InvalidCodeFormat = 0x2016,
        PtpResponseCode_UnknownVendorCode = 0x2017,
        PtpResponseCode_CaptureAlreadyTerminated = 0x2018,
        PtpResponseCode_DeviceBusy = 0x2019,
        PtpResponseCode_InvalidParentObject = 0x201A,
        PtpResponseCode_InvalidDevicePropFormat = 0x201B,
        PtpResponseCode_InvalidDevicePropValue = 0x201C,
        PtpResponseCode_InvalidParameter = 0x201D,
        PtpResponseCode_SessionAlreadyOpened = 0x201E,
        PtpResponseCode_TransactionCanceled = 0x201F,
        PtpResponseCode_SpecificationOfDestinationUnsupported = 0x2020,
        PtpResponseCode_InvalidEnumHandle = 0x2021,
        PtpResponseCode_NoStreamEnabled = 0x2022,
        PtpResponseCode_InvalidDataSet = 0x2023,
        PtpResponseCode_MtpUndefined = 0xA800,
        PtpResponseCode_MtpInvalid_ObjectPropCode = 0xA801,
        PtpResponseCode_MtpInvalid_ObjectProp_Format = 0xA802,
        PtpResponseCode_MtpInvalid_ObjectProp_Value = 0xA803,
        PtpResponseCode_MtpInvalid_ObjectReference = 0xA804,
        PtpResponseCode_MtpInvalid_Dataset = 0xA806,
        PtpResponseCode_MtpSpecification_By_Group_Unsupported = 0xA807,
        PtpResponseCode_MtpSpecification_By_Depth_Unsupported = 0xA808,
        PtpResponseCode_MtpObject_Too_Large = 0xA809,
        PtpResponseCode_MtpObjectProp_Not_Supported = 0xA80A,
    };

    enum PtpEventCode : u16 {
        PtpEventCode_Undefined = 0x4000,
        PtpEventCode_CancelTransaction = 0x4001,
        PtpEventCode_ObjectAdded = 0x4002,
        PtpEventCode_ObjectRemoved = 0x4003,
        PtpEventCode_StoreAdded = 0x4004,
        PtpEventCode_StoreRemoved = 0x4005,
        PtpEventCode_DevicePropChanged = 0x4006,
        PtpEventCode_ObjectInfoChanged = 0x4007,
        PtpEventCode_DeviceInfoChanged = 0x4008,
        PtpEventCode_RequestObjectTransfer = 0x4009,
        PtpEventCode_StoreFull = 0x400A,
        PtpEventCode_DeviceReset = 0x400B,
        PtpEventCode_StorageInfoChanged = 0x400C,
        PtpEventCode_CaptureComplete = 0x400D,
        PtpEventCode_UnreportedStatus = 0x400E,
    };

    enum PtpDevicePropertyCode : u16 {
        PtpDevicePropertyCode_Undefined = 0x5000,
        PtpDevicePropertyCode_BatteryLevel = 0x5001,
        PtpDevicePropertyCode_FunctionalMode = 0x5002,
        PtpDevicePropertyCode_ImageSize = 0x5003,
        PtpDevicePropertyCode_CompressionSetting = 0x5004,
        PtpDevicePropertyCode_WhiteBalance = 0x5005,
        PtpDevicePropertyCode_RgbGain = 0x5006,
        PtpDevicePropertyCode_FNumber = 0x5007,
        PtpDevicePropertyCode_FocalLength = 0x5008,
        PtpDevicePropertyCode_FocusDistance = 0x5009,
        PtpDevicePropertyCode_FocusMode = 0x500A,
        PtpDevicePropertyCode_ExposureMeteringMode = 0x500B,
        PtpDevicePropertyCode_FlashMode = 0x500C,
        PtpDevicePropertyCode_ExposureTime = 0x500D,
        PtpDevicePropertyCode_ExposureProgramMode = 0x500E,
        PtpDevicePropertyCode_ExposureIndex = 0x500F,
        PtpDevicePropertyCode_ExposureBiasCompensation = 0x5010,
        PtpDevicePropertyCode_DateTime = 0x5011,
        PtpDevicePropertyCode_CaptureDelay = 0x5012,
        PtpDevicePropertyCode_StillCaptureMode = 0x5013,
        PtpDevicePropertyCode_Contrast = 0x5014,
        PtpDevicePropertyCode_Sharpness = 0x5015,
        PtpDevicePropertyCode_DigitalZoom = 0x5016,
        PtpDevicePropertyCode_EffectMode = 0x5017,
        PtpDevicePropertyCode_BurstNumber = 0x5018,
        PtpDevicePropertyCode_BurstInterval = 0x5019,
        PtpDevicePropertyCode_TimelapseNumber = 0x501A,
        PtpDevicePropertyCode_TimelapseInterval = 0x501B,
        PtpDevicePropertyCode_FocusMeteringMode = 0x501C,
        PtpDevicePropertyCode_UploadUrl = 0x501D,
        PtpDevicePropertyCode_Artist = 0x501E,
        PtpDevicePropertyCode_CopyrightInfo = 0x501F,
        PtpDevicePropertyCode_SupportedStreams = 0x5020,
        PtpDevicePropertyCode_EnabledStreams = 0x5021,
        PtpDevicePropertyCode_VideoFormat = 0x5022,
        PtpDevicePropertyCode_VideoResolution = 0x5023,
        PtpDevicePropertyCode_VideoQuality = 0x5024,
        PtpDevicePropertyCode_VideoFrameRate = 0x5025,
        PtpDevicePropertyCode_VideoContrast = 0x5026,
        PtpDevicePropertyCode_VideoBrightness = 0x5027,
        PtpDevicePropertyCode_AudioFormat = 0x5028,
        PtpDevicePropertyCode_AudioBitrate = 0x5029,
        PtpDevicePropertyCode_AudioSamplingRate = 0x502A,
        PtpDevicePropertyCode_AudioBitPerSample = 0x502B,
        PtpDevicePropertyCode_AudioVolume = 0x502C,
    };

    enum PtpObjectFormatCode : u16 {
        PtpObjectFormatCode_Undefined = 0x3000,
        PtpObjectFormatCode_Association = 0x3001,
        PtpObjectFormatCode_Defined = 0x3800,
        PtpObjectFormatCode_MtpMediaCard = 0xB211,
    };

    enum PtpAssociationType : u16 {
        PtpAssociationType_Undefined = 0x0,
        PtpAssociationType_GenericFolder = 0x1,
    };

    enum PtpGetObjectHandles : u32 {
        PtpGetObjectHandles_AllFormats = 0x0,
        PtpGetObjectHandles_AllAssocs = 0x0,
        PtpGetObjectHandles_AllStorage = 0xffffffff,
        PtpGetObjectHandles_RootParent = 0xffffffff,
    };

    enum PtpStorageType : u16 {
        PtpStorageType_Undefined = 0x0000,
        PtpStorageType_FixedRom = 0x0001,
        PtpStorageType_RemovableRom = 0x0002,
        PtpStorageType_FixedRam = 0x0003,
        PtpStorageType_RemovableRam = 0x0004,
    };

    enum PtpFilesystemType : u16 {
        PtpFilesystemType_Undefined = 0x0000,
        PtpFilesystemType_GenericFlat = 0x0001,
        PtpFilesystemType_GenericHierarchical = 0x0002,
        PtpFilesystemType_Dcf = 0x0003,
    };

    enum PtpAccessCapability : u16 {
        PtpAccessCapability_ReadWrite = 0x0000,
        PtpAccessCapability_ReadOnly = 0x0001,
        PtpAccessCapability_ReadOnlyWithObjectDeletion = 0x0002,
    };

    enum PtpProtectionStatus : u16 {
        PtpProtectionStatus_NoProtection = 0x0000,
        PtpProtectionStatus_ReadOnly = 0x0001,
        PtpProtectionStatus_MtpReadOnlyData = 0x8002,
        PtpProtectionStatus_MtpNonTransferableData = 0x8003,
    };

    enum PtpThumbFormat : u16 {
        PtpThumbFormat_Undefined = 0x0000,
    };

    struct PtpUsbBulkContainer {
        uint32_t length;
        uint16_t type;
        uint16_t code;
        uint32_t trans_id;
    };
    static_assert(sizeof(PtpUsbBulkContainer) == PtpUsbBulkHeaderLength);

    struct PtpNewObjectInfo {
        uint32_t storage_id;
        uint32_t parent_object_id;
        uint32_t object_id;
    };
}
