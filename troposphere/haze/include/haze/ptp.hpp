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

    constexpr inline size_t PtpUsbBulkHighSpeedMaxPacketLength = 0x200;
    constexpr inline size_t PtpUsbBulkHeaderLength = 2 * sizeof(u32) + 2 * sizeof(u16);
    constexpr inline size_t PtpStringMaxLength = 255;

    enum PtpUsbBulkContainerType : u16 {
        PtpUsbBulkContainerType_Undefined = 0,
        PtpUsbBulkContainerType_Command   = 1,
        PtpUsbBulkContainerType_Data      = 2,
        PtpUsbBulkContainerType_Response  = 3,
        PtpUsbBulkContainerType_Event     = 4,
    };

    enum PtpOperationCode : u16 {
        PtpOperationCode_Undefined                    = 0x1000,
        PtpOperationCode_GetDeviceInfo                = 0x1001,
        PtpOperationCode_OpenSession                  = 0x1002,
        PtpOperationCode_CloseSession                 = 0x1003,
        PtpOperationCode_GetStorageIds                = 0x1004,
        PtpOperationCode_GetStorageInfo               = 0x1005,
        PtpOperationCode_GetNumObjects                = 0x1006,
        PtpOperationCode_GetObjectHandles             = 0x1007,
        PtpOperationCode_GetObjectInfo                = 0x1008,
        PtpOperationCode_GetObject                    = 0x1009,
        PtpOperationCode_GetThumb                     = 0x100a,
        PtpOperationCode_DeleteObject                 = 0x100b,
        PtpOperationCode_SendObjectInfo               = 0x100c,
        PtpOperationCode_SendObject                   = 0x100d,
        PtpOperationCode_InitiateCapture              = 0x100e,
        PtpOperationCode_FormatStore                  = 0x100f,
        PtpOperationCode_ResetDevice                  = 0x1010,
        PtpOperationCode_SelfTest                     = 0x1011,
        PtpOperationCode_SetObjectProtection          = 0x1012,
        PtpOperationCode_PowerDown                    = 0x1013,
        PtpOperationCode_GetDevicePropDesc            = 0x1014,
        PtpOperationCode_GetDevicePropValue           = 0x1015,
        PtpOperationCode_SetDevicePropValue           = 0x1016,
        PtpOperationCode_ResetDevicePropValue         = 0x1017,
        PtpOperationCode_TerminateOpenCapture         = 0x1018,
        PtpOperationCode_MoveObject                   = 0x1019,
        PtpOperationCode_CopyObject                   = 0x101a,
        PtpOperationCode_GetPartialObject             = 0x101b,
        PtpOperationCode_InitiateOpenCapture          = 0x101c,
        PtpOperationCode_StartEnumHandles             = 0x101d,
        PtpOperationCode_EnumHandles                  = 0x101e,
        PtpOperationCode_StopEnumHandles              = 0x101f,
        PtpOperationCode_GetVendorExtensionMaps       = 0x1020,
        PtpOperationCode_GetVendorDeviceInfo          = 0x1021,
        PtpOperationCode_GetResizedImageObject        = 0x1022,
        PtpOperationCode_GetFilesystemManifest        = 0x1023,
        PtpOperationCode_GetStreamInfo                = 0x1024,
        PtpOperationCode_GetStream                    = 0x1025,
        PtpOperationCode_MtpGetObjectPropsSupported   = 0x9801,
        PtpOperationCode_MtpGetObjectPropDesc         = 0x9802,
        PtpOperationCode_MtpGetObjectPropValue        = 0x9803,
        PtpOperationCode_MtpSetObjectPropValue        = 0x9804,
        PtpOperationCode_MtpGetObjPropList            = 0x9805,
        PtpOperationCode_MtpSetObjPropList            = 0x9806,
        PtpOperationCode_MtpGetInterdependendPropdesc = 0x9807,
        PtpOperationCode_MtpSendObjectPropList        = 0x9808,
        PtpOperationCode_MtpGetObjectReferences       = 0x9810,
        PtpOperationCode_MtpSetObjectReferences       = 0x9811,
        PtpOperationCode_MtpUpdateDeviceFirmware      = 0x9812,
        PtpOperationCode_MtpSkip                      = 0x9820,
    };

    enum PtpResponseCode : u16 {
        PtpResponseCode_Undefined                             = 0x2000,
        PtpResponseCode_Ok                                    = 0x2001,
        PtpResponseCode_GeneralError                          = 0x2002,
        PtpResponseCode_SessionNotOpen                        = 0x2003,
        PtpResponseCode_InvalidTransactionId                  = 0x2004,
        PtpResponseCode_OperationNotSupported                 = 0x2005,
        PtpResponseCode_ParameterNotSupported                 = 0x2006,
        PtpResponseCode_IncompleteTransfer                    = 0x2007,
        PtpResponseCode_InvalidStorageId                      = 0x2008,
        PtpResponseCode_InvalidObjectHandle                   = 0x2009,
        PtpResponseCode_DevicePropNotSupported                = 0x200a,
        PtpResponseCode_InvalidObjectFormatCode               = 0x200b,
        PtpResponseCode_StoreFull                             = 0x200c,
        PtpResponseCode_ObjectWriteProtected                  = 0x200d,
        PtpResponseCode_StoreReadOnly                         = 0x200e,
        PtpResponseCode_AccessDenied                          = 0x200f,
        PtpResponseCode_NoThumbnailPresent                    = 0x2010,
        PtpResponseCode_SelfTestFailed                        = 0x2011,
        PtpResponseCode_PartialDeletion                       = 0x2012,
        PtpResponseCode_StoreNotAvailable                     = 0x2013,
        PtpResponseCode_SpecificationByFormatUnsupported      = 0x2014,
        PtpResponseCode_NoValidObjectInfo                     = 0x2015,
        PtpResponseCode_InvalidCodeFormat                     = 0x2016,
        PtpResponseCode_UnknownVendorCode                     = 0x2017,
        PtpResponseCode_CaptureAlreadyTerminated              = 0x2018,
        PtpResponseCode_DeviceBusy                            = 0x2019,
        PtpResponseCode_InvalidParentObject                   = 0x201a,
        PtpResponseCode_InvalidDevicePropFormat               = 0x201b,
        PtpResponseCode_InvalidDevicePropValue                = 0x201c,
        PtpResponseCode_InvalidParameter                      = 0x201d,
        PtpResponseCode_SessionAlreadyOpened                  = 0x201e,
        PtpResponseCode_TransactionCanceled                   = 0x201f,
        PtpResponseCode_SpecificationOfDestinationUnsupported = 0x2020,
        PtpResponseCode_InvalidEnumHandle                     = 0x2021,
        PtpResponseCode_NoStreamEnabled                       = 0x2022,
        PtpResponseCode_InvalidDataSet                        = 0x2023,
        PtpResponseCode_MtpUndefined                          = 0xa800,
        PtpResponseCode_MtpInvalid_ObjectPropCode             = 0xa801,
        PtpResponseCode_MtpInvalid_ObjectProp_Format          = 0xa802,
        PtpResponseCode_MtpInvalid_ObjectProp_Value           = 0xa803,
        PtpResponseCode_MtpInvalid_ObjectReference            = 0xa804,
        PtpResponseCode_MtpInvalid_Dataset                    = 0xa806,
        PtpResponseCode_MtpSpecification_By_Group_Unsupported = 0xa807,
        PtpResponseCode_MtpSpecification_By_Depth_Unsupported = 0xa808,
        PtpResponseCode_MtpObject_Too_Large                   = 0xa809,
        PtpResponseCode_MtpObjectProp_Not_Supported           = 0xa80A,
    };

    enum PtpEventCode : u16 {
        PtpEventCode_Undefined             = 0x4000,
        PtpEventCode_CancelTransaction     = 0x4001,
        PtpEventCode_ObjectAdded           = 0x4002,
        PtpEventCode_ObjectRemoved         = 0x4003,
        PtpEventCode_StoreAdded            = 0x4004,
        PtpEventCode_StoreRemoved          = 0x4005,
        PtpEventCode_DevicePropChanged     = 0x4006,
        PtpEventCode_ObjectInfoChanged     = 0x4007,
        PtpEventCode_DeviceInfoChanged     = 0x4008,
        PtpEventCode_RequestObjectTransfer = 0x4009,
        PtpEventCode_StoreFull             = 0x400a,
        PtpEventCode_DeviceReset           = 0x400b,
        PtpEventCode_StorageInfoChanged    = 0x400c,
        PtpEventCode_CaptureComplete       = 0x400d,
        PtpEventCode_UnreportedStatus      = 0x400e,
    };

    enum PtpDevicePropertyCode : u16 {
        PtpDevicePropertyCode_Undefined                = 0x5000,
        PtpDevicePropertyCode_BatteryLevel             = 0x5001,
        PtpDevicePropertyCode_FunctionalMode           = 0x5002,
        PtpDevicePropertyCode_ImageSize                = 0x5003,
        PtpDevicePropertyCode_CompressionSetting       = 0x5004,
        PtpDevicePropertyCode_WhiteBalance             = 0x5005,
        PtpDevicePropertyCode_RgbGain                  = 0x5006,
        PtpDevicePropertyCode_FNumber                  = 0x5007,
        PtpDevicePropertyCode_FocalLength              = 0x5008,
        PtpDevicePropertyCode_FocusDistance            = 0x5009,
        PtpDevicePropertyCode_FocusMode                = 0x500a,
        PtpDevicePropertyCode_ExposureMeteringMode     = 0x500b,
        PtpDevicePropertyCode_FlashMode                = 0x500c,
        PtpDevicePropertyCode_ExposureTime             = 0x500d,
        PtpDevicePropertyCode_ExposureProgramMode      = 0x500e,
        PtpDevicePropertyCode_ExposureIndex            = 0x500f,
        PtpDevicePropertyCode_ExposureBiasCompensation = 0x5010,
        PtpDevicePropertyCode_DateTime                 = 0x5011,
        PtpDevicePropertyCode_CaptureDelay             = 0x5012,
        PtpDevicePropertyCode_StillCaptureMode         = 0x5013,
        PtpDevicePropertyCode_Contrast                 = 0x5014,
        PtpDevicePropertyCode_Sharpness                = 0x5015,
        PtpDevicePropertyCode_DigitalZoom              = 0x5016,
        PtpDevicePropertyCode_EffectMode               = 0x5017,
        PtpDevicePropertyCode_BurstNumber              = 0x5018,
        PtpDevicePropertyCode_BurstInterval            = 0x5019,
        PtpDevicePropertyCode_TimelapseNumber          = 0x501a,
        PtpDevicePropertyCode_TimelapseInterval        = 0x501b,
        PtpDevicePropertyCode_FocusMeteringMode        = 0x501c,
        PtpDevicePropertyCode_UploadUrl                = 0x501d,
        PtpDevicePropertyCode_Artist                   = 0x501e,
        PtpDevicePropertyCode_CopyrightInfo            = 0x501f,
        PtpDevicePropertyCode_SupportedStreams         = 0x5020,
        PtpDevicePropertyCode_EnabledStreams           = 0x5021,
        PtpDevicePropertyCode_VideoFormat              = 0x5022,
        PtpDevicePropertyCode_VideoResolution          = 0x5023,
        PtpDevicePropertyCode_VideoQuality             = 0x5024,
        PtpDevicePropertyCode_VideoFrameRate           = 0x5025,
        PtpDevicePropertyCode_VideoContrast            = 0x5026,
        PtpDevicePropertyCode_VideoBrightness          = 0x5027,
        PtpDevicePropertyCode_AudioFormat              = 0x5028,
        PtpDevicePropertyCode_AudioBitrate             = 0x5029,
        PtpDevicePropertyCode_AudioSamplingRate        = 0x502a,
        PtpDevicePropertyCode_AudioBitPerSample        = 0x502b,
        PtpDevicePropertyCode_AudioVolume              = 0x502c,
    };

    enum PtpObjectFormatCode : u16 {
        PtpObjectFormatCode_Undefined    = 0x3000,
        PtpObjectFormatCode_Association  = 0x3001,
        PtpObjectFormatCode_Defined      = 0x3800,
        PtpObjectFormatCode_MtpMediaCard = 0xb211,
    };

    enum PtpAssociationType : u16 {
        PtpAssociationType_Undefined     = 0x0,
        PtpAssociationType_GenericFolder = 0x1,
    };

    enum PtpGetObjectHandles : u32 {
        PtpGetObjectHandles_AllFormats = 0x0,
        PtpGetObjectHandles_AllAssocs  = 0x0,
        PtpGetObjectHandles_AllStorage = 0xffffffff,
        PtpGetObjectHandles_RootParent = 0xffffffff,
    };

    enum PtpStorageType : u16 {
        PtpStorageType_Undefined    = 0x0000,
        PtpStorageType_FixedRom     = 0x0001,
        PtpStorageType_RemovableRom = 0x0002,
        PtpStorageType_FixedRam     = 0x0003,
        PtpStorageType_RemovableRam = 0x0004,
    };

    enum PtpFilesystemType : u16 {
        PtpFilesystemType_Undefined           = 0x0000,
        PtpFilesystemType_GenericFlat         = 0x0001,
        PtpFilesystemType_GenericHierarchical = 0x0002,
        PtpFilesystemType_Dcf                 = 0x0003,
    };

    enum PtpAccessCapability : u16 {
        PtpAccessCapability_ReadWrite                  = 0x0000,
        PtpAccessCapability_ReadOnly                   = 0x0001,
        PtpAccessCapability_ReadOnlyWithObjectDeletion = 0x0002,
    };

    enum PtpProtectionStatus : u16 {
        PtpProtectionStatus_NoProtection           = 0x0000,
        PtpProtectionStatus_ReadOnly               = 0x0001,
        PtpProtectionStatus_MtpReadOnlyData        = 0x8002,
        PtpProtectionStatus_MtpNonTransferableData = 0x8003,
    };

    enum PtpThumbFormat : u16 {
        PtpThumbFormat_Undefined = 0x0000,
    };

    struct PtpUsbBulkContainer {
        u32 length;
        u16 type;
        u16 code;
        u32 trans_id;
    };
    static_assert(sizeof(PtpUsbBulkContainer) == PtpUsbBulkHeaderLength);

    struct PtpNewObjectInfo {
        u32 storage_id;
        u32 parent_object_id;
        u32 object_id;
    };
}
