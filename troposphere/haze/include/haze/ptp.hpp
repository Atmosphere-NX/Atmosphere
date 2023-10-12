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

    constexpr inline u32 PtpUsbBulkHighSpeedMaxPacketLength  = 0x200;
    constexpr inline u32 PtpUsbBulkSuperSpeedMaxPacketLength = 0x400;
    constexpr inline u32 PtpUsbBulkHeaderLength = 2 * sizeof(u32) + 2 * sizeof(u16);
    constexpr inline u32 PtpStringMaxLength = 255;

    enum PtpUsbBulkContainerType : u16 {
        PtpUsbBulkContainerType_Undefined = 0x0000,
        PtpUsbBulkContainerType_Command   = 0x0001,
        PtpUsbBulkContainerType_Data      = 0x0002,
        PtpUsbBulkContainerType_Response  = 0x0003,
        PtpUsbBulkContainerType_Event     = 0x0004,
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
        PtpOperationCode_AndroidGetPartialObject64    = 0x95c1,
        PtpOperationCode_AndroidSendPartialObject     = 0x95c2,
        PtpOperationCode_AndroidTruncateObject        = 0x95c3,
        PtpOperationCode_AndroidBeginEditObject       = 0x95c4,
        PtpOperationCode_AndroidEndEditObject         = 0x95c5,
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
        PtpResponseCode_MtpInvalidObjectPropCode              = 0xa801,
        PtpResponseCode_MtpInvalidObjectPropFormat            = 0xa802,
        PtpResponseCode_MtpInvalidObjectPropValue             = 0xa803,
        PtpResponseCode_MtpInvalidObjectReference             = 0xa804,
        PtpResponseCode_MtpInvalidDataset                     = 0xa806,
        PtpResponseCode_MtpSpecificationByGroupUnsupported    = 0xa807,
        PtpResponseCode_MtpSpecificationByDepthUnsupported    = 0xa808,
        PtpResponseCode_MtpObjectTooLarge                     = 0xa809,
        PtpResponseCode_MtpObjectPropNotSupported             = 0xa80a,
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

    enum PtpDataTypeCode : u16 {
        PtpDataTypeCode_Undefined = 0x0000,
        PtpDataTypeCode_S8        = 0x0001,
        PtpDataTypeCode_U8        = 0x0002,
        PtpDataTypeCode_S16       = 0x0003,
        PtpDataTypeCode_U16       = 0x0004,
        PtpDataTypeCode_S32       = 0x0005,
        PtpDataTypeCode_U32       = 0x0006,
        PtpDataTypeCode_S64       = 0x0007,
        PtpDataTypeCode_U64       = 0x0008,
        PtpDataTypeCode_S128      = 0x0009,
        PtpDataTypeCode_U128      = 0x000a,

        PtpDataTypeCode_ArrayMask = (1u << 14),

        PtpDataTypeCode_S8Array   = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_S8,
        PtpDataTypeCode_U8Array   = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_U8,
        PtpDataTypeCode_S16Array  = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_S16,
        PtpDataTypeCode_U16Array  = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_U16,
        PtpDataTypeCode_S32Array  = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_S32,
        PtpDataTypeCode_U32Array  = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_U32,
        PtpDataTypeCode_S64Array  = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_S64,
        PtpDataTypeCode_U64Array  = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_U64,
        PtpDataTypeCode_S128Array = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_S128,
        PtpDataTypeCode_U128Array = PtpDataTypeCode_ArrayMask | PtpDataTypeCode_U128,

        PtpDataTypeCode_String    = 0xffff,
    };

    enum PtpPropertyGetSetFlag : u8 {
        PtpPropertyGetSetFlag_Get    = 0x00,
        PtpPropertyGetSetFlag_GetSet = 0x01,
    };

    enum PtpPropertyGroupCode : u32 {
        PtpPropertyGroupCode_Default = 0x00000000,
    };

    enum PtpPropertyFormFlag : u8 {
        PtpPropertyFormFlag_None              = 0x00,
        PtpPropertyFormFlag_Range             = 0x01,
        PtpPropertyFormFlag_Enumeration       = 0x02,
        PtpPropertyFormFlag_DateTime          = 0x03,
        PtpPropertyFormFlag_FixedLengthArray  = 0x04,
        PtpPropertyFormFlag_RegularExpression = 0x05,
        PtpPropertyFormFlag_ByteArray         = 0x06,
        PtpPropertyFormFlag_LongString        = 0xff,
    };

    enum PtpObjectPropertyCode : u16 {
        PtpObjectPropertyCode_StorageId                           = 0xdc01,
        PtpObjectPropertyCode_ObjectFormat                        = 0xdc02,
        PtpObjectPropertyCode_ProtectionStatus                    = 0xdc03,
        PtpObjectPropertyCode_ObjectSize                          = 0xdc04,
        PtpObjectPropertyCode_AssociationType                     = 0xdc05,
        PtpObjectPropertyCode_AssociationDesc                     = 0xdc06,
        PtpObjectPropertyCode_ObjectFileName                      = 0xdc07,
        PtpObjectPropertyCode_DateCreated                         = 0xdc08,
        PtpObjectPropertyCode_DateModified                        = 0xdc09,
        PtpObjectPropertyCode_Keywords                            = 0xdc0a,
        PtpObjectPropertyCode_ParentObject                        = 0xdc0b,
        PtpObjectPropertyCode_AllowedFolderContents               = 0xdc0c,
        PtpObjectPropertyCode_Hidden                              = 0xdc0d,
        PtpObjectPropertyCode_SystemObject                        = 0xdc0e,
        PtpObjectPropertyCode_PersistentUniqueObjectIdentifier    = 0xdc41,
        PtpObjectPropertyCode_SyncId                              = 0xdc42,
        PtpObjectPropertyCode_PropertyBag                         = 0xdc43,
        PtpObjectPropertyCode_Name                                = 0xdc44,
        PtpObjectPropertyCode_CreatedBy                           = 0xdc45,
        PtpObjectPropertyCode_Artist                              = 0xdc46,
        PtpObjectPropertyCode_DateAuthored                        = 0xdc47,
        PtpObjectPropertyCode_Description                         = 0xdc48,
        PtpObjectPropertyCode_UrlReference                        = 0xdc49,
        PtpObjectPropertyCode_LanguageLocale                      = 0xdc4a,
        PtpObjectPropertyCode_CopyrightInformation                = 0xdc4b,
        PtpObjectPropertyCode_Source                              = 0xdc4c,
        PtpObjectPropertyCode_OriginLocation                      = 0xdc4d,
        PtpObjectPropertyCode_DateAdded                           = 0xdc4e,
        PtpObjectPropertyCode_NonConsumable                       = 0xdc4f,
        PtpObjectPropertyCode_CorruptOrUnplayable                 = 0xdc50,
        PtpObjectPropertyCode_ProducerSerialNumber                = 0xdc51,
        PtpObjectPropertyCode_RepresentativeSampleFormat          = 0xdc81,
        PtpObjectPropertyCode_RepresentativeSampleSize            = 0xdc82,
        PtpObjectPropertyCode_RepresentativeSampleHeight          = 0xdc83,
        PtpObjectPropertyCode_RepresentativeSampleWidth           = 0xdc84,
        PtpObjectPropertyCode_RepresentativeSampleDuration        = 0xdc85,
        PtpObjectPropertyCode_RepresentativeSampleData            = 0xdc86,
        PtpObjectPropertyCode_Width                               = 0xdc87,
        PtpObjectPropertyCode_Height                              = 0xdc88,
        PtpObjectPropertyCode_Duration                            = 0xdc89,
        PtpObjectPropertyCode_Rating                              = 0xdc8a,
        PtpObjectPropertyCode_Track                               = 0xdc8b,
        PtpObjectPropertyCode_Genre                               = 0xdc8c,
        PtpObjectPropertyCode_Credits                             = 0xdc8d,
        PtpObjectPropertyCode_Lyrics                              = 0xdc8e,
        PtpObjectPropertyCode_SubscriptionContentId               = 0xdc8f,
        PtpObjectPropertyCode_ProducedBy                          = 0xdc90,
        PtpObjectPropertyCode_UseCount                            = 0xdc91,
        PtpObjectPropertyCode_SkipCount                           = 0xdc92,
        PtpObjectPropertyCode_LastAccessed                        = 0xdc93,
        PtpObjectPropertyCode_ParentalRating                      = 0xdc94,
        PtpObjectPropertyCode_MetaGenre                           = 0xdc95,
        PtpObjectPropertyCode_Composer                            = 0xdc96,
        PtpObjectPropertyCode_EffectiveRating                     = 0xdc97,
        PtpObjectPropertyCode_Subtitle                            = 0xdc98,
        PtpObjectPropertyCode_OriginalReleaseDate                 = 0xdc99,
        PtpObjectPropertyCode_AlbumName                           = 0xdc9a,
        PtpObjectPropertyCode_AlbumArtist                         = 0xdc9b,
        PtpObjectPropertyCode_Mood                                = 0xdc9c,
        PtpObjectPropertyCode_DrmStatus                           = 0xdc9d,
        PtpObjectPropertyCode_SubDescription                      = 0xdc9e,
        PtpObjectPropertyCode_IsCropped                           = 0xdcd1,
        PtpObjectPropertyCode_IsColorCorrected                    = 0xdcd2,
        PtpObjectPropertyCode_ImageBitDepth                       = 0xdcd3,
        PtpObjectPropertyCode_Fnumber                             = 0xdcd4,
        PtpObjectPropertyCode_ExposureTime                        = 0xdcd5,
        PtpObjectPropertyCode_ExposureIndex                       = 0xdcd6,
        PtpObjectPropertyCode_DisplayName                         = 0xdce0,
        PtpObjectPropertyCode_BodyText                            = 0xdce1,
        PtpObjectPropertyCode_Subject                             = 0xdce2,
        PtpObjectPropertyCode_Priority                            = 0xdce3,
        PtpObjectPropertyCode_GivenName                           = 0xdd00,
        PtpObjectPropertyCode_MiddleNames                         = 0xdd01,
        PtpObjectPropertyCode_FamilyName                          = 0xdd02,
        PtpObjectPropertyCode_Prefix                              = 0xdd03,
        PtpObjectPropertyCode_Suffix                              = 0xdd04,
        PtpObjectPropertyCode_PhoneticGivenName                   = 0xdd05,
        PtpObjectPropertyCode_PhoneticFamilyName                  = 0xdd06,
        PtpObjectPropertyCode_EmailPrimary                        = 0xdd07,
        PtpObjectPropertyCode_EmailPersonal1                      = 0xdd08,
        PtpObjectPropertyCode_EmailPersonal2                      = 0xdd09,
        PtpObjectPropertyCode_EmailBusiness1                      = 0xdd0a,
        PtpObjectPropertyCode_EmailBusiness2                      = 0xdd0b,
        PtpObjectPropertyCode_EmailOthers                         = 0xdd0c,
        PtpObjectPropertyCode_PhoneNumberPrimary                  = 0xdd0d,
        PtpObjectPropertyCode_PhoneNumberPersonal                 = 0xdd0e,
        PtpObjectPropertyCode_PhoneNumberPersonal2                = 0xdd0f,
        PtpObjectPropertyCode_PhoneNumberBusiness                 = 0xdd10,
        PtpObjectPropertyCode_PhoneNumberBusiness2                = 0xdd11,
        PtpObjectPropertyCode_PhoneNumberMobile                   = 0xdd12,
        PtpObjectPropertyCode_PhoneNumberMobile2                  = 0xdd13,
        PtpObjectPropertyCode_FaxNumberPrimary                    = 0xdd14,
        PtpObjectPropertyCode_FaxNumberPersonal                   = 0xdd15,
        PtpObjectPropertyCode_FaxNumberBusiness                   = 0xdd16,
        PtpObjectPropertyCode_PagerNumber                         = 0xdd17,
        PtpObjectPropertyCode_PhoneNumberOthers                   = 0xdd18,
        PtpObjectPropertyCode_PrimaryWebAddress                   = 0xdd19,
        PtpObjectPropertyCode_PersonalWebAddress                  = 0xdd1a,
        PtpObjectPropertyCode_BusinessWebAddress                  = 0xdd1b,
        PtpObjectPropertyCode_InstantMessengerAddress             = 0xdd1c,
        PtpObjectPropertyCode_InstantMessengerAddress2            = 0xdd1d,
        PtpObjectPropertyCode_InstantMessengerAddress3            = 0xdd1e,
        PtpObjectPropertyCode_PostalAddressPersonalFull           = 0xdd1f,
        PtpObjectPropertyCode_PostalAddressPersonalFullLine1      = 0xdd20,
        PtpObjectPropertyCode_PostalAddressPersonalFullLine2      = 0xdd21,
        PtpObjectPropertyCode_PostalAddressPersonalFullCity       = 0xdd22,
        PtpObjectPropertyCode_PostalAddressPersonalFullRegion     = 0xdd23,
        PtpObjectPropertyCode_PostalAddressPersonalFullPostalCode = 0xdd24,
        PtpObjectPropertyCode_PostalAddressPersonalFullCountry    = 0xdd25,
        PtpObjectPropertyCode_PostalAddressBusinessFull           = 0xdd26,
        PtpObjectPropertyCode_PostalAddressBusinessLine1          = 0xdd27,
        PtpObjectPropertyCode_PostalAddressBusinessLine2          = 0xdd28,
        PtpObjectPropertyCode_PostalAddressBusinessCity           = 0xdd29,
        PtpObjectPropertyCode_PostalAddressBusinessRegion         = 0xdd2a,
        PtpObjectPropertyCode_PostalAddressBusinessPostalCode     = 0xdd2b,
        PtpObjectPropertyCode_PostalAddressBusinessCountry        = 0xdd2c,
        PtpObjectPropertyCode_PostalAddressOtherFull              = 0xdd2d,
        PtpObjectPropertyCode_PostalAddressOtherLine1             = 0xdd2e,
        PtpObjectPropertyCode_PostalAddressOtherLine2             = 0xdd2f,
        PtpObjectPropertyCode_PostalAddressOtherCity              = 0xdd30,
        PtpObjectPropertyCode_PostalAddressOtherRegion            = 0xdd31,
        PtpObjectPropertyCode_PostalAddressOtherPostalCode        = 0xdd32,
        PtpObjectPropertyCode_PostalAddressOtherCountry           = 0xdd33,
        PtpObjectPropertyCode_OrganizationName                    = 0xdd34,
        PtpObjectPropertyCode_PhoneticOrganizationName            = 0xdd35,
        PtpObjectPropertyCode_Role                                = 0xdd36,
        PtpObjectPropertyCode_Birthdate                           = 0xdd37,
        PtpObjectPropertyCode_MessageTo                           = 0xdd40,
        PtpObjectPropertyCode_MessageCC                           = 0xdd41,
        PtpObjectPropertyCode_MessageBCC                          = 0xdd42,
        PtpObjectPropertyCode_MessageRead                         = 0xdd43,
        PtpObjectPropertyCode_MessageReceivedTime                 = 0xdd44,
        PtpObjectPropertyCode_MessageSender                       = 0xdd45,
        PtpObjectPropertyCode_ActivityBeginTime                   = 0xdd50,
        PtpObjectPropertyCode_ActivityEndTime                     = 0xdd51,
        PtpObjectPropertyCode_ActivityLocation                    = 0xdd52,
        PtpObjectPropertyCode_ActivityRequiredAttendees           = 0xdd54,
        PtpObjectPropertyCode_ActivityOptionalAttendees           = 0xdd55,
        PtpObjectPropertyCode_ActivityResources                   = 0xdd56,
        PtpObjectPropertyCode_ActivityAccepted                    = 0xdd57,
        PtpObjectPropertyCode_Owner                               = 0xdd5d,
        PtpObjectPropertyCode_Editor                              = 0xdd5e,
        PtpObjectPropertyCode_Webmaster                           = 0xdd5f,
        PtpObjectPropertyCode_UrlSource                           = 0xdd60,
        PtpObjectPropertyCode_UrlDestination                      = 0xdd61,
        PtpObjectPropertyCode_TimeBookmark                        = 0xdd62,
        PtpObjectPropertyCode_ObjectBookmark                      = 0xdd63,
        PtpObjectPropertyCode_ByteBookmark                        = 0xdd64,
        PtpObjectPropertyCode_LastBuildDate                       = 0xdd70,
        PtpObjectPropertyCode_TimetoLive                          = 0xdd71,
        PtpObjectPropertyCode_MediaGuid                           = 0xdd72,
        PtpObjectPropertyCode_TotalBitRate                        = 0xde91,
        PtpObjectPropertyCode_BitRateType                         = 0xde92,
        PtpObjectPropertyCode_SampleRate                          = 0xde93,
        PtpObjectPropertyCode_NumberOfChannels                    = 0xde94,
        PtpObjectPropertyCode_AudioBitDepth                       = 0xde95,
        PtpObjectPropertyCode_ScanDepth                           = 0xde97,
        PtpObjectPropertyCode_AudioWaveCodec                      = 0xde99,
        PtpObjectPropertyCode_AudioBitRate                        = 0xde9a,
        PtpObjectPropertyCode_VideoFourCcCodec                    = 0xde9b,
        PtpObjectPropertyCode_VideoBitRate                        = 0xde9c,
        PtpObjectPropertyCode_FramesPerThousandSeconds            = 0xde9d,
        PtpObjectPropertyCode_KeyFrameDistance                    = 0xde9e,
        PtpObjectPropertyCode_BufferSize                          = 0xde9f,
        PtpObjectPropertyCode_EncodingQuality                     = 0xdea0,
        PtpObjectPropertyCode_EncodingProfile                     = 0xdea1,
        PtpObjectPropertyCode_BuyFlag                             = 0xd901,
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
        PtpAssociationType_Undefined     = 0x0000,
        PtpAssociationType_GenericFolder = 0x0001,
    };

    enum PtpGetObjectHandles : u32 {
        PtpGetObjectHandles_AllFormats = 0x00000000,
        PtpGetObjectHandles_AllAssocs  = 0x00000000,
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
