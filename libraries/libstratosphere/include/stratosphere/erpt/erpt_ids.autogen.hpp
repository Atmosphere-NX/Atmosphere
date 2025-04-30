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
#include <vapours.hpp>

/* NOTE: This file is auto-generated. */
/* Do not make edits to this file by hand. */

#define AMS_ERPT_FOREACH_FIELD_TYPE(HANDLER) \
    HANDLER(FieldType_NumericU64, 0 ) \
    HANDLER(FieldType_NumericU32, 1 ) \
    HANDLER(FieldType_NumericI64, 2 ) \
    HANDLER(FieldType_NumericI32, 3 ) \
    HANDLER(FieldType_String,     4 ) \
    HANDLER(FieldType_U8Array,    5 ) \
    HANDLER(FieldType_U32Array,   6 ) \
    HANDLER(FieldType_U64Array,   7 ) \
    HANDLER(FieldType_I32Array,   8 ) \
    HANDLER(FieldType_I64Array,   9 ) \
    HANDLER(FieldType_Bool,       10) \
    HANDLER(FieldType_NumericU16, 11) \
    HANDLER(FieldType_NumericU8,  12) \
    HANDLER(FieldType_NumericI16, 13) \
    HANDLER(FieldType_NumericI8,  14) \
    HANDLER(FieldType_I8Array,    15)

#define AMS_ERPT_FOREACH_CATEGORY(HANDLER) \
    HANDLER(Test,                                0   ) \
    HANDLER(ErrorInfo,                           1   ) \
    HANDLER(ConnectionStatusInfo,                2   ) \
    HANDLER(NetworkInfo,                         3   ) \
    HANDLER(NXMacAddressInfo,                    4   ) \
    HANDLER(StealthNetworkInfo,                  5   ) \
    HANDLER(LimitHighCapacityInfo,               6   ) \
    HANDLER(NATTypeInfo,                         7   ) \
    HANDLER(WirelessAPMacAddressInfo,            8   ) \
    HANDLER(GlobalIPAddressInfo,                 9   ) \
    HANDLER(EnableWirelessInterfaceInfo,         10  ) \
    HANDLER(EnableWifiInfo,                      11  ) \
    HANDLER(EnableBluetoothInfo,                 12  ) \
    HANDLER(EnableNFCInfo,                       13  ) \
    HANDLER(NintendoZoneSSIDListVersionInfo,     14  ) \
    HANDLER(LANAdapterMacAddressInfo,            15  ) \
    HANDLER(ApplicationInfo,                     16  ) \
    HANDLER(OccurrenceInfo,                      17  ) \
    HANDLER(ProductModelInfo,                    18  ) \
    HANDLER(CurrentLanguageInfo,                 19  ) \
    HANDLER(UseNetworkTimeProtocolInfo,          20  ) \
    HANDLER(TimeZoneInfo,                        21  ) \
    HANDLER(ControllerFirmwareInfo,              22  ) \
    HANDLER(VideoOutputInfo,                     23  ) \
    HANDLER(NANDFreeSpaceInfo,                   24  ) \
    HANDLER(SDCardFreeSpaceInfo,                 25  ) \
    HANDLER(ScreenBrightnessInfo,                26  ) \
    HANDLER(AudioFormatInfo,                     27  ) \
    HANDLER(MuteOnHeadsetUnpluggedInfo,          28  ) \
    HANDLER(NumUserRegisteredInfo,               29  ) \
    HANDLER(DataDeletionInfo,                    30  ) \
    HANDLER(ControllerVibrationInfo,             31  ) \
    HANDLER(LockScreenInfo,                      32  ) \
    HANDLER(InternalBatteryLotNumberInfo,        33  ) \
    HANDLER(LeftControllerSerialNumberInfo,      34  ) \
    HANDLER(RightControllerSerialNumberInfo,     35  ) \
    HANDLER(NotificationInfo,                    36  ) \
    HANDLER(TVInfo,                              37  ) \
    HANDLER(SleepInfo,                           38  ) \
    HANDLER(ConnectionInfo,                      39  ) \
    HANDLER(NetworkErrorInfo,                    40  ) \
    HANDLER(FileAccessPathInfo,                  41  ) \
    HANDLER(GameCardCIDInfo,                     42  ) \
    HANDLER(NANDCIDInfoDeprecated,               43  ) \
    HANDLER(MicroSDCIDInfoDeprecated,            44  ) \
    HANDLER(NANDSpeedModeInfo,                   45  ) \
    HANDLER(MicroSDSpeedModeInfo,                46  ) \
    HANDLER(GameCardSpeedModeInfo,               47  ) \
    HANDLER(UserAccountInternalIDInfo,           48  ) \
    HANDLER(NetworkServiceAccountInternalIDInfo, 49  ) \
    HANDLER(NintendoAccountInternalIDInfo,       50  ) \
    HANDLER(USB3AvailableInfo,                   51  ) \
    HANDLER(CallStackInfo,                       52  ) \
    HANDLER(SystemStartupLogInfo,                53  ) \
    HANDLER(RegionSettingInfo,                   54  ) \
    HANDLER(NintendoZoneConnectedInfo,           55  ) \
    HANDLER(ForceSleepInfo,                      56  ) \
    HANDLER(ChargerInfo,                         57  ) \
    HANDLER(RadioStrengthInfo,                   58  ) \
    HANDLER(ErrorInfoAuto,                       59  ) \
    HANDLER(AccessPointInfo,                     60  ) \
    HANDLER(ErrorInfoDefaults,                   61  ) \
    HANDLER(SystemPowerStateInfo,                62  ) \
    HANDLER(PerformanceInfo,                     63  ) \
    HANDLER(ThrottlingInfo,                      64  ) \
    HANDLER(GameCardErrorInfo,                   65  ) \
    HANDLER(EdidInfo,                            66  ) \
    HANDLER(ThermalInfo,                         67  ) \
    HANDLER(CradleFirmwareInfo,                  68  ) \
    HANDLER(RunningApplicationInfo,              69  ) \
    HANDLER(RunningAppletInfo,                   70  ) \
    HANDLER(FocusedAppletHistoryInfo,            71  ) \
    HANDLER(CompositorInfo,                      72  ) \
    HANDLER(BatteryChargeInfo,                   73  ) \
    HANDLER(NANDExtendedCsdDeprecated,           74  ) \
    HANDLER(NANDPatrolInfo,                      75  ) \
    HANDLER(NANDErrorInfo,                       76  ) \
    HANDLER(NANDDriverLog,                       77  ) \
    HANDLER(SdCardSizeSpec,                      78  ) \
    HANDLER(SdCardErrorInfo,                     79  ) \
    HANDLER(SdCardDriverLog ,                    80  ) \
    HANDLER(FsProxyErrorInfo,                    81  ) \
    HANDLER(SystemAppletSceneInfo,               82  ) \
    HANDLER(VideoInfo,                           83  ) \
    HANDLER(GpuErrorInfo,                        84  ) \
    HANDLER(PowerClockInfo,                      85  ) \
    HANDLER(AdspErrorInfo,                       86  ) \
    HANDLER(NvDispDeviceInfo,                    87  ) \
    HANDLER(NvDispDcWindowInfo,                  88  ) \
    HANDLER(NvDispDpModeInfo,                    89  ) \
    HANDLER(NvDispDpLinkSpec,                    90  ) \
    HANDLER(NvDispDpLinkStatus,                  91  ) \
    HANDLER(NvDispDpHdcpInfo,                    92  ) \
    HANDLER(NvDispDpAuxCecInfo,                  93  ) \
    HANDLER(NvDispDcInfo,                        94  ) \
    HANDLER(NvDispDsiInfo,                       95  ) \
    HANDLER(NvDispErrIDInfo,                     96  ) \
    HANDLER(SdCardMountInfo,                     97  ) \
    HANDLER(RetailInteractiveDisplayInfo,        98  ) \
    HANDLER(CompositorStateInfo,                 99  ) \
    HANDLER(CompositorLayerInfo,                 100 ) \
    HANDLER(CompositorDisplayInfo,               101 ) \
    HANDLER(CompositorHWCInfo,                   102 ) \
    HANDLER(MonitorCapability,                   103 ) \
    HANDLER(ErrorReportSharePermissionInfo,      104 ) \
    HANDLER(MultimediaInfo,                      105 ) \
    HANDLER(ConnectedControllerInfo,             106 ) \
    HANDLER(FsMemoryInfo,                        107 ) \
    HANDLER(UserClockContextInfo,                108 ) \
    HANDLER(NetworkClockContextInfo,             109 ) \
    HANDLER(AcpGeneralSettingsInfo,              110 ) \
    HANDLER(AcpPlayLogSettingsInfo,              111 ) \
    HANDLER(AcpAocSettingsInfo,                  112 ) \
    HANDLER(AcpBcatSettingsInfo,                 113 ) \
    HANDLER(AcpStorageSettingsInfo,              114 ) \
    HANDLER(AcpRatingSettingsInfo,               115 ) \
    HANDLER(MonitorSettings,                     116 ) \
    HANDLER(RebootlessSystemUpdateVersionInfo,   117 ) \
    HANDLER(NifmConnectionTestInfo,              118 ) \
    HANDLER(PcieLoggedStateInfo,                 119 ) \
    HANDLER(NetworkSecurityCertificateInfo,      120 ) \
    HANDLER(AcpNeighborDetectionInfo,            121 ) \
    HANDLER(GpuCrashInfo,                        122 ) \
    HANDLER(UsbStateInfo,                        123 ) \
    HANDLER(NvHostErrInfo,                       124 ) \
    HANDLER(RunningUlaInfo,                      125 ) \
    HANDLER(InternalPanelInfo,                   126 ) \
    HANDLER(ResourceLimitLimitInfo,              127 ) \
    HANDLER(ResourceLimitPeakInfo,               128 ) \
    HANDLER(TouchScreenInfo,                     129 ) \
    HANDLER(AcpUserAccountSettingsInfo,          130 ) \
    HANDLER(AudioDeviceInfo,                     131 ) \
    HANDLER(AbnormalWakeInfo,                    132 ) \
    HANDLER(ServiceProfileInfo,                  133 ) \
    HANDLER(BluetoothAudioInfo,                  134 ) \
    HANDLER(BluetoothPairingCountInfo,           135 ) \
    HANDLER(FsProxyErrorInfo2,                   136 ) \
    HANDLER(BuiltInWirelessOUIInfo,              137 ) \
    HANDLER(WirelessAPOUIInfo,                   138 ) \
    HANDLER(EthernetAdapterOUIInfo,              139 ) \
    HANDLER(NANDTypeInfoDeprecated,              140 ) \
    HANDLER(MicroSDTypeInfo,                     141 ) \
    HANDLER(AttachmentFileInfo,                  142 ) \
    HANDLER(WlanInfo,                            143 ) \
    HANDLER(HalfAwakeStateInfo,                  144 ) \
    HANDLER(PctlSettingInfo,                     145 ) \
    HANDLER(GameCardLogInfo,                     146 ) \
    HANDLER(WlanIoctlErrorInfo,                  147 ) \
    HANDLER(SdCardActivationInfo,                148 ) \
    HANDLER(GameCardDetailedErrorInfo,           149 ) \
    HANDLER(TestNx,                              1000) \
    HANDLER(NANDTypeInfo,                        1001) \
    HANDLER(NANDExtendedCsd,                     1002) \

#define AMS_ERPT_FOREACH_FIELD(HANDLER) \
    HANDLER(TestU64,                                                  0,    Test,                                FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(TestU32,                                                  1,    Test,                                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(TestI64,                                                  2,    Test,                                FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(TestI32,                                                  3,    Test,                                FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(TestString,                                               4,    Test,                                FieldType_String,     FieldFlag_None   ) \
    HANDLER(TestU8Array,                                              5,    Test,                                FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(TestU32Array,                                             6,    Test,                                FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(TestU64Array,                                             7,    Test,                                FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(TestI32Array,                                             8,    Test,                                FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(TestI64Array,                                             9,    Test,                                FieldType_I64Array,   FieldFlag_None   ) \
    HANDLER(ErrorCode,                                                10,   ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(ErrorDescription,                                         11,   ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(OccurrenceTimestamp,                                      12,   ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ReportIdentifier,                                         13,   ErrorInfoAuto,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(ConnectionStatus,                                         14,   ConnectionStatusInfo,                FieldType_String,     FieldFlag_None   ) \
    HANDLER(AccessPointSSID,                                          15,   AccessPointInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(AccessPointSecurityType,                                  16,   AccessPointInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(RadioStrength,                                            17,   RadioStrengthInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NXMacAddress,                                             18,   NXMacAddressInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(IPAddressAcquisitionMethod,                               19,   NetworkInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CurrentIPAddress,                                         20,   NetworkInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(SubnetMask,                                               21,   NetworkInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(GatewayIPAddress,                                         22,   NetworkInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(DNSType,                                                  23,   NetworkInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PriorityDNSIPAddress,                                     24,   NetworkInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(AlternateDNSIPAddress,                                    25,   NetworkInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(UseProxyFlag,                                             26,   NetworkInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ProxyIPAddress,                                           27,   NetworkInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(ProxyPort,                                                28,   NetworkInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ProxyAutoAuthenticateFlag,                                29,   NetworkInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(MTU,                                                      30,   NetworkInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ConnectAutomaticallyFlag,                                 31,   NetworkInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(UseStealthNetworkFlag,                                    32,   StealthNetworkInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LimitHighCapacityFlag,                                    33,   LimitHighCapacityInfo,               FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NATType,                                                  34,   NATTypeInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(WirelessAPMacAddress,                                     35,   WirelessAPMacAddressInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(GlobalIPAddress,                                          36,   GlobalIPAddressInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(EnableWirelessInterfaceFlag,                              37,   EnableWirelessInterfaceInfo,         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(EnableWifiFlag,                                           38,   EnableWifiInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(EnableBluetoothFlag,                                      39,   EnableBluetoothInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(EnableNFCFlag,                                            40,   EnableNFCInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NintendoZoneSSIDListVersion,                              41,   NintendoZoneSSIDListVersionInfo,     FieldType_String,     FieldFlag_None   ) \
    HANDLER(LANAdapterMacAddress,                                     42,   LANAdapterMacAddressInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationID,                                            43,   ApplicationInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationTitle,                                         44,   ApplicationInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationVersion,                                       45,   ApplicationInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationStorageLocation,                               46,   ApplicationInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(DownloadContentType,                                      47,   OccurrenceInfo,                      FieldType_String,     FieldFlag_None   ) \
    HANDLER(InstallContentType,                                       48,   OccurrenceInfo,                      FieldType_String,     FieldFlag_None   ) \
    HANDLER(ConsoleStartingUpFlag,                                    49,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(SystemStartingUpFlag,                                     50,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ConsoleFirstInitFlag,                                     51,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(HomeMenuScreenDisplayedFlag,                              52,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(DataManagementScreenDisplayedFlag,                        53,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ConnectionTestingFlag,                                    54,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ApplicationRunningFlag,                                   55,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(DataCorruptionDetectedFlag,                               56,   OccurrenceInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ProductModel,                                             57,   ProductModelInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(CurrentLanguage,                                          58,   CurrentLanguageInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(UseNetworkTimeProtocolFlag,                               59,   UseNetworkTimeProtocolInfo,          FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TimeZone,                                                 60,   TimeZoneInfo,                        FieldType_String,     FieldFlag_None   ) \
    HANDLER(ControllerFirmware,                                       61,   ControllerFirmwareInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(VideoOutputSetting,                                       62,   VideoOutputInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(NANDFreeSpace,                                            63,   NANDFreeSpaceInfo,                   FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SDCardFreeSpace,                                          64,   SDCardFreeSpaceInfo,                 FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SerialNumber,                                             65,   ErrorInfoAuto,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(OsVersion,                                                66,   ErrorInfoAuto,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(ScreenBrightnessAutoAdjustFlag,                           67,   ScreenBrightnessInfo,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(HdmiAudioOutputMode,                                      68,   AudioFormatInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(SpeakerAudioOutputMode,                                   69,   AudioFormatInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(HeadphoneAudioOutputMode,                                 70,   AudioFormatInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(MuteOnHeadsetUnpluggedFlag,                               71,   MuteOnHeadsetUnpluggedInfo,          FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NumUserRegistered,                                        72,   NumUserRegisteredInfo,               FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(StorageAutoOrganizeFlag,                                  73,   DataDeletionInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ControllerVibrationVolume,                                74,   ControllerVibrationInfo,             FieldType_String,     FieldFlag_None   ) \
    HANDLER(LockScreenFlag,                                           75,   LockScreenInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(InternalBatteryLotNumber,                                 76,   InternalBatteryLotNumberInfo,        FieldType_String,     FieldFlag_None   ) \
    HANDLER(LeftControllerSerialNumber,                               77,   LeftControllerSerialNumberInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(RightControllerSerialNumber,                              78,   RightControllerSerialNumberInfo,     FieldType_String,     FieldFlag_None   ) \
    HANDLER(NotifyInGameDownloadCompletionFlag,                       79,   NotificationInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NotificationSoundFlag,                                    80,   NotificationInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TVResolutionSetting,                                      81,   TVInfo,                              FieldType_String,     FieldFlag_None   ) \
    HANDLER(RGBRangeSetting,                                          82,   TVInfo,                              FieldType_String,     FieldFlag_None   ) \
    HANDLER(ReduceScreenBurnFlag,                                     83,   TVInfo,                              FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TVAllowsCecFlag,                                          84,   TVInfo,                              FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(HandheldModeTimeToScreenSleep,                            85,   SleepInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(ConsoleModeTimeToScreenSleep,                             86,   SleepInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(StopAutoSleepDuringContentPlayFlag,                       87,   SleepInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LastConnectionTestDownloadSpeed,                          88,   ConnectionInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(LastConnectionTestUploadSpeed,                            89,   ConnectionInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(DEPRECATED_ServerFQDN,                                    90,   NetworkErrorInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(HTTPRequestContents,                                      91,   NetworkErrorInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(HTTPRequestResponseContents,                              92,   NetworkErrorInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(EdgeServerIPAddress,                                      93,   NetworkErrorInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(CDNContentPath,                                           94,   NetworkErrorInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(FileAccessPath,                                           95,   FileAccessPathInfo,                  FieldType_String,     FieldFlag_None   ) \
    HANDLER(GameCardCID,                                              96,   GameCardCIDInfo,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NANDCIDDeprecated,                                        97,   NANDCIDInfoDeprecated,               FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(MicroSDCIDDeprecated,                                     98,   MicroSDCIDInfoDeprecated,            FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NANDSpeedMode,                                            99,   NANDSpeedModeInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(MicroSDSpeedMode,                                         100,  MicroSDSpeedModeInfo,                FieldType_String,     FieldFlag_None   ) \
    HANDLER(GameCardSpeedMode,                                        101,  GameCardSpeedModeInfo,               FieldType_String,     FieldFlag_None   ) \
    HANDLER(UserAccountInternalID,                                    102,  UserAccountInternalIDInfo,           FieldType_String,     FieldFlag_None   ) \
    HANDLER(NetworkServiceAccountInternalID,                          103,  NetworkServiceAccountInternalIDInfo, FieldType_String,     FieldFlag_None   ) \
    HANDLER(NintendoAccountInternalID,                                104,  NintendoAccountInternalIDInfo,       FieldType_String,     FieldFlag_None   ) \
    HANDLER(USB3AvailableFlag,                                        105,  USB3AvailableInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(CallStack,                                                106,  CallStackInfo,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(SystemStartupLog,                                         107,  SystemStartupLogInfo,                FieldType_String,     FieldFlag_None   ) \
    HANDLER(RegionSetting,                                            108,  RegionSettingInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(NintendoZoneConnectedFlag,                                109,  NintendoZoneConnectedInfo,           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ForcedSleepHighTemperatureReading,                        110,  ForceSleepInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ForcedSleepFanSpeedReading,                               111,  ForceSleepInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ForcedSleepHWInfo,                                        112,  ForceSleepInfo,                      FieldType_String,     FieldFlag_None   ) \
    HANDLER(AbnormalPowerStateInfo,                                   113,  ChargerInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ScreenBrightnessLevel,                                    114,  ScreenBrightnessInfo,                FieldType_String,     FieldFlag_None   ) \
    HANDLER(ProgramId,                                                115,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(AbortFlag,                                                116,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ReportVisibilityFlag,                                     117,  ErrorInfoAuto,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(FatalFlag,                                                118,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(OccurrenceTimestampNet,                                   119,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ResultBacktrace,                                          120,  ErrorInfo,                           FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(GeneralRegisterAarch32,                                   121,  ErrorInfo,                           FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(StackBacktrace32,                                         122,  ErrorInfo,                           FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(ExceptionInfoAarch32,                                     123,  ErrorInfo,                           FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(GeneralRegisterAarch64,                                   124,  ErrorInfo,                           FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(ExceptionInfoAarch64,                                     125,  ErrorInfo,                           FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(StackBacktrace64,                                         126,  ErrorInfo,                           FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(RegisterSetFlag32,                                        127,  ErrorInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(RegisterSetFlag64,                                        128,  ErrorInfo,                           FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ProgramMappedAddr32,                                      129,  ErrorInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ProgramMappedAddr64,                                      130,  ErrorInfo,                           FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AbortType,                                                131,  ErrorInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PrivateOsVersion,                                         132,  ErrorInfoAuto,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(CurrentSystemPowerState,                                  133,  SystemPowerStateInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PreviousSystemPowerState,                                 134,  SystemPowerStateInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(DestinationSystemPowerState,                              135,  SystemPowerStateInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PscTransitionCurrentState,                                136,  ErrorInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PscTransitionPreviousState,                               137,  ErrorInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PscInitializedList,                                       138,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(PscCurrentPmStateList,                                    139,  ErrorInfo,                           FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PscNextPmStateList,                                       140,  ErrorInfo,                           FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PerformanceMode,                                          141,  PerformanceInfo,                     FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PerformanceConfiguration,                                 142,  PerformanceInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(Throttled,                                                143,  ThrottlingInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ThrottlingDuration,                                       144,  ThrottlingInfo,                      FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ThrottlingTimestamp,                                      145,  ThrottlingInfo,                      FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(GameCardCrcErrorCount,                                    146,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardAsicCrcErrorCount,                                147,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardRefreshCount,                                     148,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardReadRetryCount,                                   149,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(EdidBlock,                                                150,  EdidInfo,                            FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EdidExtensionBlock,                                       151,  EdidInfo,                            FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(CreateProcessFailureFlag,                                 152,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TemperaturePcb,                                           153,  ThermalInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(TemperatureSoc,                                           154,  ThermalInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(CurrentFanDuty,                                           155,  ThermalInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(LastDvfsThresholdTrippedDeprecated,                       156,  ThermalInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(CradlePdcHFwVersion,                                      157,  CradleFirmwareInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CradlePdcAFwVersion,                                      158,  CradleFirmwareInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CradleMcuFwVersion,                                       159,  CradleFirmwareInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CradleDp2HdmiFwVersion,                                   160,  CradleFirmwareInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(RunningApplicationId,                                     161,  RunningApplicationInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationTitle,                                  162,  RunningApplicationInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationVersion,                                163,  RunningApplicationInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationStorageLocation,                        164,  RunningApplicationInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningAppletList,                                        165,  RunningAppletInfo,                   FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(FocusedAppletHistory,                                     166,  FocusedAppletHistoryInfo,            FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(CompositorState,                                          167,  CompositorStateInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(CompositorLayerState,                                     168,  CompositorLayerInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(CompositorDisplayState,                                   169,  CompositorDisplayInfo,               FieldType_String,     FieldFlag_None   ) \
    HANDLER(CompositorHWCState,                                       170,  CompositorHWCInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(InputCurrentLimit,                                        171,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BoostModeCurrentLimitDeprecated,                          172,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FastChargeCurrentLimit,                                   173,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ChargeVoltageLimit,                                       174,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ChargeConfigurationDeprecated,                            175,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(HizModeDeprecated,                                        176,  BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ChargeEnabled,                                            177,  BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PowerSupplyPathDeprecated,                                178,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryTemperature,                                       179,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryChargePercent,                                     180,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryChargeVoltage,                                     181,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryAge,                                               182,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerRole,                                                183,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerSupplyType,                                          184,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerSupplyVoltage,                                       185,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerSupplyCurrent,                                       186,  BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FastBatteryChargingEnabled,                               187,  BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ControllerPowerSupplyAcquiredDeprecated,                  188,  BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(OtgRequestedDeprecated,                                   189,  BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NANDPreEolInfoDeprecated,                                 190,  NANDExtendedCsdDeprecated,           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDDeviceLifeTimeEstTypADeprecated,                      191,  NANDExtendedCsdDeprecated,           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDDeviceLifeTimeEstTypBDeprecated,                      192,  NANDExtendedCsdDeprecated,           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDPatrolCount,                                          193,  NANDPatrolInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumActivationFailures,                                194,  NANDErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumActivationErrorCorrections,                        195,  NANDErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumReadWriteFailures,                                 196,  NANDErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumReadWriteErrorCorrections,                         197,  NANDErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDErrorLog,                                             198,  NANDDriverLog,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(SdCardUserAreaSize,                                       199,  SdCardSizeSpec,                      FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SdCardProtectedAreaSize,                                  200,  SdCardSizeSpec,                      FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SdCardNumActivationFailures,                              201,  SdCardErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardNumActivationErrorCorrections,                      202,  SdCardErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardNumReadWriteFailures,                               203,  SdCardErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardNumReadWriteErrorCorrections,                       204,  SdCardErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardErrorLog,                                           205,  SdCardDriverLog ,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(EncryptionKey,                                            206,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EncryptedExceptionInfo,                                   207,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardTimeoutRetryErrorCount,                           208,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsRemountForDataCorruptCount,                             209,  FsProxyErrorInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsRemountForDataCorruptRetryOutCount,                     210,  FsProxyErrorInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardInsertionCount,                                   211,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardRemovalCount,                                     212,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardAsicInitializeCount,                              213,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(TestU16,                                                  214,  Test,                                FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(TestU8,                                                   215,  Test,                                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(TestI16,                                                  216,  Test,                                FieldType_NumericI16, FieldFlag_None   ) \
    HANDLER(TestI8,                                                   217,  Test,                                FieldType_NumericI8,  FieldFlag_None   ) \
    HANDLER(SystemAppletScene,                                        218,  SystemAppletSceneInfo,               FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(CodecType,                                                219,  VideoInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(DecodeBuffers,                                            220,  VideoInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FrameWidth,                                               221,  VideoInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FrameHeight,                                              222,  VideoInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ColorPrimaries,                                           223,  VideoInfo,                           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(TransferCharacteristics,                                  224,  VideoInfo,                           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(MatrixCoefficients,                                       225,  VideoInfo,                           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(DisplayWidth,                                             226,  VideoInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(DisplayHeight,                                            227,  VideoInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(DARWidth,                                                 228,  VideoInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(DARHeight,                                                229,  VideoInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ColorFormat,                                              230,  VideoInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ColorSpace,                                               231,  VideoInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(SurfaceLayout,                                            232,  VideoInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(BitStream,                                                233,  VideoInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(VideoDecState,                                            234,  VideoInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(GpuErrorChannelId,                                        235,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorAruId,                                            236,  GpuErrorInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(GpuErrorType,                                             237,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorFaultInfo,                                        238,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorWriteAccess,                                      239,  GpuErrorInfo,                        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(GpuErrorFaultAddress,                                     240,  GpuErrorInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(GpuErrorFaultUnit,                                        241,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorFaultType,                                        242,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorHwContextPointer,                                 243,  GpuErrorInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(GpuErrorContextStatus,                                    244,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaIntr,                                        245,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaErrorType,                                   246,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaHeaderShadow,                                247,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaHeader,                                      248,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaGpShadow0,                                   249,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaGpShadow1,                                   250,  GpuErrorInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AccessPointChannel,                                       251,  AccessPointInfo,                     FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(ThreadName,                                               252,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(AdspExceptionRegistersDeprecated,                         253,  AdspErrorInfo,                       FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionSpsrDeprecated,                              254,  AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionProgramCounter,                              255,  AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionLinkRegister,                                256,  AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionStackPointer,                                257,  AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionArmModeRegistersDeprecated,                  258,  AdspErrorInfo,                       FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionStackAddressDeprecated,                      259,  AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionStackDumpDeprecated,                         260,  AdspErrorInfo,                       FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionReasonDeprecated,                            261,  AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(OscillatorClockDeprecated,                                262,  PowerClockInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CpuDvfsTableClocksDeprecated,                             263,  PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(CpuDvfsTableVoltagesDeprecated,                           264,  PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(GpuDvfsTableClocksDeprecated,                             265,  PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(GpuDvfsTableVoltagesDeprecated,                           266,  PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(EmcDvfsTableClocksDeprecated,                             267,  PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(EmcDvfsTableVoltagesDeprecated,                           268,  PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(ModuleClockFrequencies,                                   269,  PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(ModuleClockEnableFlagsDeprecated,                         270,  PowerClockInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ModulePowerEnableFlagsDeprecated,                         271,  PowerClockInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ModuleResetAssertFlags,                                   272,  PowerClockInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ModuleMinimumVoltageClockRates,                           273,  PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PowerDomainEnableFlagsDeprecated,                         274,  PowerClockInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(PowerDomainVoltagesDeprecated,                            275,  PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(AccessPointRssi,                                          276,  RadioStrengthInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FuseInfoDeprecated,                                       277,  PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(VideoLog,                                                 278,  VideoInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(GameCardDeviceId,                                         279,  GameCardCIDInfo,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardAsicReinitializeCount,                            280,  GameCardErrorInfo,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardAsicReinitializeFailureCount,                     281,  GameCardErrorInfo,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardAsicReinitializeFailureDetail,                    282,  GameCardErrorInfo,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardRefreshSuccessCount,                              283,  GameCardErrorInfo,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardAwakenCount,                                      284,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardAwakenFailureCount,                               285,  GameCardErrorInfo,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardReadCountFromInsert,                              286,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardReadCountFromAwaken,                              287,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLastReadErrorPageAddress,                         288,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLastReadErrorPageCount,                           289,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AppletManagerContextTrace,                                290,  ErrorInfo,                           FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(NvDispIsRegistered,                                       291,  NvDispDeviceInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispIsSuspend,                                          292,  NvDispDeviceInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDC0SurfaceNum,                                      293,  NvDispDeviceInfo,                    FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(NvDispDC1SurfaceNum,                                      294,  NvDispDeviceInfo,                    FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectX,                                     295,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectY,                                     296,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectWidth,                                 297,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectHeight,                                298,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectX,                                     299,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectY,                                     300,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectWidth,                                 301,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectHeight,                                302,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowIndex,                                        303,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowBlendOperation,                               304,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowAlphaOperation,                               305,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDepth,                                        306,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowAlpha,                                        307,  NvDispDcWindowInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispWindowHFilter,                                      308,  NvDispDcWindowInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispWindowVFilter,                                      309,  NvDispDcWindowInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispWindowOptions,                                      310,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSyncPointId,                                  311,  NvDispDcWindowInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPSorPower,                                         312,  NvDispDpModeInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPClkType,                                          313,  NvDispDpModeInfo,                    FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPEnable,                                           314,  NvDispDpModeInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPState,                                            315,  NvDispDpModeInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPEdid,                                             316,  NvDispDpModeInfo,                    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispDPEdidSize,                                         317,  NvDispDpModeInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPEdidExtSize,                                      318,  NvDispDpModeInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPFakeMode,                                         319,  NvDispDpModeInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPModeNumber,                                       320,  NvDispDpModeInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPPlugInOut,                                        321,  NvDispDpModeInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPAuxIntHandler,                                    322,  NvDispDpModeInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPForceMaxLinkBW,                                   323,  NvDispDpModeInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPIsConnected,                                      324,  NvDispDpModeInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkValid,                                        325,  NvDispDpLinkSpec,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkMaxBW,                                        326,  NvDispDpLinkSpec,                    FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkMaxLaneCount,                                 327,  NvDispDpLinkSpec,                    FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkDownSpread,                                   328,  NvDispDpLinkSpec,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkSupportEnhancedFraming,                       329,  NvDispDpLinkSpec,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkBpp,                                          330,  NvDispDpLinkSpec,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkScaramberCap,                                 331,  NvDispDpLinkSpec,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkBW,                                           332,  NvDispDpLinkStatus,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkLaneCount,                                    333,  NvDispDpLinkStatus,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkEnhancedFraming,                              334,  NvDispDpLinkStatus,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkScrambleEnable,                               335,  NvDispDpLinkStatus,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkActivePolarity,                               336,  NvDispDpLinkStatus,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkActiveCount,                                  337,  NvDispDpLinkStatus,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkTUSize,                                       338,  NvDispDpLinkStatus,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkActiveFrac,                                   339,  NvDispDpLinkStatus,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkWatermark,                                    340,  NvDispDpLinkStatus,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkHBlank,                                       341,  NvDispDpLinkStatus,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkVBlank,                                       342,  NvDispDpLinkStatus,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkOnlyEnhancedFraming,                          343,  NvDispDpLinkStatus,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkOnlyEdpCap,                                   344,  NvDispDpLinkStatus,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkSupportFastLT,                                345,  NvDispDpLinkStatus,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkLTDataValid,                                  346,  NvDispDpLinkStatus,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkTsp3Support,                                  347,  NvDispDpLinkStatus,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkAuxInterval,                                  348,  NvDispDpLinkStatus,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpCreated,                                        349,  NvDispDpHdcpInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispHdcpUserRequest,                                    350,  NvDispDpHdcpInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispHdcpPlugged,                                        351,  NvDispDpHdcpInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispHdcpState,                                          352,  NvDispDpHdcpInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispHdcpFailCount,                                      353,  NvDispDpHdcpInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispHdcpHdcp22,                                         354,  NvDispDpHdcpInfo,                    FieldType_NumericI8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpMaxRetry,                                       355,  NvDispDpHdcpInfo,                    FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpHpd,                                            356,  NvDispDpHdcpInfo,                    FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpRepeater,                                       357,  NvDispDpHdcpInfo,                    FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispCecRxBuf,                                           358,  NvDispDpAuxCecInfo,                  FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispCecRxLength,                                        359,  NvDispDpAuxCecInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispCecTxBuf,                                           360,  NvDispDpAuxCecInfo,                  FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispCecTxLength,                                        361,  NvDispDpAuxCecInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispCecTxRet,                                           362,  NvDispDpAuxCecInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispCecState,                                           363,  NvDispDpAuxCecInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispCecTxInfo,                                          364,  NvDispDpAuxCecInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispCecRxInfo,                                          365,  NvDispDpAuxCecInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDCIndex,                                            366,  NvDispDcInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCInitialize,                                       367,  NvDispDcInfo,                        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCClock,                                            368,  NvDispDcInfo,                        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCFrequency,                                        369,  NvDispDcInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCFailed,                                           370,  NvDispDcInfo,                        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCModeWidth,                                        371,  NvDispDcInfo,                        FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispDCModeHeight,                                       372,  NvDispDcInfo,                        FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispDCModeBpp,                                          373,  NvDispDcInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCPanelFrequency,                                   374,  NvDispDcInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCWinDirty,                                         375,  NvDispDcInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCWinEnable,                                        376,  NvDispDcInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCVrr,                                              377,  NvDispDcInfo,                        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCPanelInitialize,                                  378,  NvDispDcInfo,                        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDsiDataFormat,                                      379,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiVideoMode,                                       380,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiRefreshRate,                                     381,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiLpCmdModeFrequency,                              382,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiHsCmdModeFrequency,                              383,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiPanelResetTimeout,                               384,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiPhyFrequency,                                    385,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiFrequency,                                       386,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiInstance,                                        387,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHostCtrlEnable,                                388,  NvDispDsiInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiInit,                                          389,  NvDispDsiInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiEnable,                                        390,  NvDispDsiInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHsMode,                                        391,  NvDispDsiInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiVendorId,                                      392,  NvDispDsiInfo,                       FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispDcDsiLcdVendorNum,                                  393,  NvDispDsiInfo,                       FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHsClockControl,                                394,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDcDsiEnableHsClockInLpMode,                         395,  NvDispDsiInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiTeFrameUpdate,                                 396,  NvDispDsiInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiGangedType,                                    397,  NvDispDsiInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHbpInPktSeq,                                   398,  NvDispDsiInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispErrID,                                              399,  NvDispErrIDInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispErrData0,                                           400,  NvDispErrIDInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispErrData1,                                           401,  NvDispErrIDInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardMountStatus,                                        402,  SdCardMountInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(SdCardMountUnexpectedResult,                              403,  SdCardMountInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(NANDTotalSize,                                            404,  NANDFreeSpaceInfo,                   FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SdCardTotalSize,                                          405,  SDCardFreeSpaceInfo,                 FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ElapsedTimeSinceInitialLaunch,                            406,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ElapsedTimeSincePowerOn,                                  407,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ElapsedTimeSinceLastAwake,                                408,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(OccurrenceTick,                                           409,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(RetailInteractiveDisplayFlag,                             410,  RetailInteractiveDisplayInfo,        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(FatFsError,                                               411,  FsProxyErrorInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsExtraError,                                          412,  FsProxyErrorInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsErrorDrive,                                          413,  FsProxyErrorInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsErrorName,                                           414,  FsProxyErrorInfo,                    FieldType_String,     FieldFlag_None   ) \
    HANDLER(MonitorManufactureCode,                                   415,  MonitorCapability,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(MonitorProductCode,                                       416,  MonitorCapability,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(MonitorSerialNumber,                                      417,  MonitorCapability,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(MonitorManufactureYear,                                   418,  MonitorCapability,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PhysicalAddress,                                          419,  MonitorCapability,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(Is4k60Hz,                                                 420,  MonitorCapability,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(Is4k30Hz,                                                 421,  MonitorCapability,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(Is1080P60Hz,                                              422,  MonitorCapability,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(Is720P60Hz,                                               423,  MonitorCapability,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PcmChannelMax,                                            424,  MonitorCapability,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(CrashReportHash,                                          425,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ErrorReportSharePermission,                               426,  ErrorReportSharePermissionInfo,      FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(VideoCodecTypeEnum,                                       427,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoBitRate,                                             428,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoFrameRate,                                           429,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoWidth,                                               430,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoHeight,                                              431,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioCodecTypeEnum,                                       432,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioSampleRate,                                          433,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioChannelCount,                                        434,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioBitRate,                                             435,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaContainerType,                                  436,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaProfileType,                                    437,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaLevelType,                                      438,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaCacheSizeEnum,                                  439,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaErrorStatusEnum,                                440,  MultimediaInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaErrorLog,                                       441,  MultimediaInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ServerFqdn,                                               442,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerIpAddress,                                          443,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(TestStringEncrypt,                                        444,  Test,                                FieldType_String,     FieldFlag_Encrypt) \
    HANDLER(TestU8ArrayEncrypt,                                       445,  Test,                                FieldType_U8Array,    FieldFlag_Encrypt) \
    HANDLER(TestU32ArrayEncrypt,                                      446,  Test,                                FieldType_U32Array,   FieldFlag_Encrypt) \
    HANDLER(TestU64ArrayEncrypt,                                      447,  Test,                                FieldType_U64Array,   FieldFlag_Encrypt) \
    HANDLER(TestI32ArrayEncrypt,                                      448,  Test,                                FieldType_I32Array,   FieldFlag_Encrypt) \
    HANDLER(TestI64ArrayEncrypt,                                      449,  Test,                                FieldType_I64Array,   FieldFlag_Encrypt) \
    HANDLER(CipherKey,                                                450,  ErrorInfoAuto,                       FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(FileSystemPath,                                           451,  ErrorInfo,                           FieldType_String,     FieldFlag_Encrypt) \
    HANDLER(WebMediaPlayerOpenUrl,                                    452,  ErrorInfo,                           FieldType_String,     FieldFlag_Encrypt) \
    HANDLER(WebMediaPlayerLastSocketErrors,                           453,  ErrorInfo,                           FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(UnknownControllerCount,                                   454,  ConnectedControllerInfo,             FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AttachedControllerCount,                                  455,  ConnectedControllerInfo,             FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothControllerCount,                                 456,  ConnectedControllerInfo,             FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(UsbControllerCount,                                       457,  ConnectedControllerInfo,             FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ControllerTypeList,                                       458,  ConnectedControllerInfo,             FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ControllerInterfaceList,                                  459,  ConnectedControllerInfo,             FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ControllerStyleList,                                      460,  ConnectedControllerInfo,             FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(FsPooledBufferPeakFreeSize,                               461,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPooledBufferRetriedCount,                               462,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPooledBufferReduceAllocationCount,                      463,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsBufferManagerPeakFreeSize,                              464,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsBufferManagerRetriedCount,                              465,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsExpHeapPeakFreeSize,                                    466,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsBufferPoolPeakFreeSize,                                 467,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPatrolReadAllocateBufferSuccessCount,                   468,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPatrolReadAllocateBufferFailureCount,                   469,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SteadyClockInternalOffset,                                470,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SteadyClockCurrentTimePointValue,                         471,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(UserClockContextOffset,                                   472,  UserClockContextInfo,                FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(UserClockContextTimeStampValue,                           473,  UserClockContextInfo,                FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(NetworkClockContextOffset,                                474,  NetworkClockContextInfo,             FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(NetworkClockContextTimeStampValue,                        475,  NetworkClockContextInfo,             FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemAbortFlag,                                          476,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ApplicationAbortFlag,                                     477,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NifmErrorCode,                                            478,  ConnectionStatusInfo,                FieldType_String,     FieldFlag_None   ) \
    HANDLER(LcsApplicationId,                                         479,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyIdList,                                  480,  ErrorInfo,                           FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyVersionList,                             481,  ErrorInfo,                           FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyTypeList,                                482,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyInstallTypeList,                         483,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(LcsSenderFlag,                                            484,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LcsApplicationRequestFlag,                                485,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LcsHasExFatDriverFlag,                                    486,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LcsIpAddress,                                             487,  ErrorInfo,                           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpStartupUserAccount,                                    488,  AcpUserAccountSettingsInfo,          FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpAocRegistrationType,                                   489,  AcpAocSettingsInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpAttributeFlag,                                         490,  AcpGeneralSettingsInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpSupportedLanguageFlag,                                 491,  AcpGeneralSettingsInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpParentalControlFlag,                                   492,  AcpGeneralSettingsInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpScreenShot,                                            493,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpVideoCapture,                                          494,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpDataLossConfirmation,                                  495,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpPlayLogPolicy,                                         496,  AcpPlayLogSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpPresenceGroupId,                                       497,  AcpGeneralSettingsInfo,              FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpRatingAge,                                             498,  AcpRatingSettingsInfo,               FieldType_I8Array,    FieldFlag_None   ) \
    HANDLER(AcpAocBaseId,                                             499,  AcpAocSettingsInfo,                  FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpSaveDataOwnerId,                                       500,  AcpStorageSettingsInfo,              FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataSize,                               501,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataJournalSize,                        502,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataSize,                                    503,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataJournalSize,                             504,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpBcatDeliveryCacheStorageSize,                          505,  AcpBcatSettingsInfo,                 FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpApplicationErrorCodeCategory,                          506,  AcpGeneralSettingsInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(AcpLocalCommunicationId,                                  507,  AcpGeneralSettingsInfo,              FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(AcpLogoType,                                              508,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpLogoHandling,                                          509,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpRuntimeAocInstall,                                     510,  AcpAocSettingsInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpCrashReport,                                           511,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpHdcp,                                                  512,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpSeedForPseudoDeviceId,                                 513,  AcpGeneralSettingsInfo,              FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpBcatPassphrase,                                        514,  AcpBcatSettingsInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataSizeMax,                            515,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataJournalSizeMax,                     516,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataSizeMax,                                 517,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataJournalSizeMax,                          518,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpTemporaryStorageSize,                                  519,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageSize,                                      520,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageJournalSize,                               521,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageDataAndJournalSizeMax,                     522,  AcpStorageSettingsInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageIndexMax,                                  523,  AcpStorageSettingsInfo,              FieldType_NumericI16, FieldFlag_None   ) \
    HANDLER(AcpPlayLogQueryableApplicationId,                         524,  AcpPlayLogSettingsInfo,              FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(AcpPlayLogQueryCapability,                                525,  AcpPlayLogSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpRepairFlag,                                            526,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(RunningApplicationPatchStorageLocation,                   527,  RunningApplicationInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationVersionNumber,                          528,  RunningApplicationInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsRecoveredByInvalidateCacheCount,                        529,  FsProxyErrorInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsSaveDataIndexCount,                                     530,  FsProxyErrorInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsBufferManagerPeakTotalAllocatableSize,                  531,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(MonitorCurrentWidth,                                      532,  MonitorSettings,                     FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(MonitorCurrentHeight,                                     533,  MonitorSettings,                     FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(MonitorCurrentRefreshRate,                                534,  MonitorSettings,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(RebootlessSystemUpdateVersion,                            535,  RebootlessSystemUpdateVersionInfo,   FieldType_String,     FieldFlag_None   ) \
    HANDLER(EncryptedExceptionInfo1,                                  536,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EncryptedExceptionInfo2,                                  537,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EncryptedExceptionInfo3,                                  538,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EncryptedDyingMessage,                                    539,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(DramIdDeprecated,                                         540,  PowerClockInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NifmConnectionTestRedirectUrl,                            541,  NifmConnectionTestInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(AcpRequiredNetworkServiceLicenseOnLaunchFlag,             542,  AcpUserAccountSettingsInfo,          FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PciePort0Flags,                                           543,  PcieLoggedStateInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort0Speed,                                           544,  PcieLoggedStateInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PciePort0ResetTimeInUs,                                   545,  PcieLoggedStateInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort0IrqCount,                                        546,  PcieLoggedStateInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort0Statistics,                                      547,  PcieLoggedStateInfo,                 FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PciePort1Flags,                                           548,  PcieLoggedStateInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort1Speed,                                           549,  PcieLoggedStateInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PciePort1ResetTimeInUs,                                   550,  PcieLoggedStateInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort1IrqCount,                                        551,  PcieLoggedStateInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort1Statistics,                                      552,  PcieLoggedStateInfo,                 FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PcieFunction0VendorId,                                    553,  PcieLoggedStateInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction0DeviceId,                                    554,  PcieLoggedStateInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction0PmState,                                     555,  PcieLoggedStateInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PcieFunction0IsAcquired,                                  556,  PcieLoggedStateInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PcieFunction1VendorId,                                    557,  PcieLoggedStateInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction1DeviceId,                                    558,  PcieLoggedStateInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction1PmState,                                     559,  PcieLoggedStateInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PcieFunction1IsAcquired,                                  560,  PcieLoggedStateInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PcieGlobalRootComplexStatistics,                          561,  PcieLoggedStateInfo,                 FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PciePllResistorCalibrationValue,                          562,  PcieLoggedStateInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(CertificateRequestedHostName,                             563,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(CertificateCommonName,                                    564,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(CertificateSANCount,                                      565,  NetworkSecurityCertificateInfo,      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CertificateSANs,                                          566,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(FsBufferPoolMaxAllocateSize,                              567,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(CertificateIssuerName,                                    568,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationAliveTime,                                     569,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ApplicationInFocusTime,                                   570,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ApplicationOutOfFocusTime,                                571,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ApplicationBackgroundFocusTime,                           572,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSwitchLock,                                 573,  AcpUserAccountSettingsInfo,          FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(USB3HostAvailableFlag,                                    574,  USB3AvailableInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(USB3DeviceAvailableFlag,                                  575,  USB3AvailableInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(AcpNeighborDetectionClientConfigurationSendDataId,        576,  AcpNeighborDetectionInfo,            FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpNeighborDetectionClientConfigurationReceivableDataIds, 577,  AcpNeighborDetectionInfo,            FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(AcpStartupUserAccountOptionFlag,                          578,  AcpUserAccountSettingsInfo,          FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ServerErrorCode,                                          579,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(AppletManagerMetaLogTrace,                                580,  ErrorInfo,                           FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(ServerCertificateSerialNumber,                            581,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerCertificatePublicKeyAlgorithm,                      582,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerCertificateSignatureAlgorithm,                      583,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerCertificateNotBefore,                               584,  NetworkSecurityCertificateInfo,      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ServerCertificateNotAfter,                                585,  NetworkSecurityCertificateInfo,      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(CertificateAlgorithmInfoBits,                             586,  NetworkSecurityCertificateInfo,      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(TlsConnectionPeerIpAddress,                               587,  NetworkSecurityCertificateInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(TlsConnectionLastHandshakeState,                          588,  NetworkSecurityCertificateInfo,      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(TlsConnectionInfoBits,                                    589,  NetworkSecurityCertificateInfo,      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SslStateBits,                                             590,  NetworkSecurityCertificateInfo,      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SslProcessInfoBits,                                       591,  NetworkSecurityCertificateInfo,      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SslProcessHeapSize,                                       592,  NetworkSecurityCertificateInfo,      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SslBaseErrorCode,                                         593,  NetworkSecurityCertificateInfo,      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(GpuCrashDumpSize,                                         594,  GpuCrashInfo,                        FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuCrashDump,                                             595,  GpuCrashInfo,                        FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(RunningApplicationProgramIndex,                           596,  RunningApplicationInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(UsbTopology,                                              597,  UsbStateInfo,                        FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(AkamaiReferenceId,                                        598,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(NvHostErrID,                                              599,  NvHostErrInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvHostErrDataArrayU32,                                    600,  NvHostErrInfo,                       FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(HasSyslogFlag,                                            601,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(AcpRuntimeParameterDelivery,                              602,  AcpGeneralSettingsInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PlatformRegion,                                           603,  RegionSettingInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningUlaApplicationId,                                  604,  RunningUlaInfo,                      FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningUlaAppletId,                                       605,  RunningUlaInfo,                      FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(RunningUlaVersion,                                        606,  RunningUlaInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(RunningUlaApplicationStorageLocation,                     607,  RunningUlaInfo,                      FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningUlaPatchStorageLocation,                           608,  RunningUlaInfo,                      FieldType_String,     FieldFlag_None   ) \
    HANDLER(NANDTotalSizeOfSystem,                                    609,  NANDFreeSpaceInfo,                   FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(NANDFreeSpaceOfSystem,                                    610,  NANDFreeSpaceInfo,                   FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AccessPointSSIDAsHex,                                     611,  AccessPointInfo,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(PanelVendorId,                                            612,  InternalPanelInfo,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PanelRevisionId,                                          613,  InternalPanelInfo,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PanelModelId,                                             614,  InternalPanelInfo,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ErrorContext,                                             615,  ErrorInfoAuto,                       FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ErrorContextSize,                                         616,  ErrorInfoAuto,                       FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ErrorContextTotalSize,                                    617,  ErrorInfoAuto,                       FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SystemPhysicalMemoryLimit,                                618,  ResourceLimitLimitInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemThreadCountLimit,                                   619,  ResourceLimitLimitInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemEventCountLimit,                                    620,  ResourceLimitLimitInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemTransferMemoryCountLimit,                           621,  ResourceLimitLimitInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemSessionCountLimit,                                  622,  ResourceLimitLimitInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemPhysicalMemoryPeak,                                 623,  ResourceLimitPeakInfo,               FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemThreadCountPeak,                                    624,  ResourceLimitPeakInfo,               FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemEventCountPeak,                                     625,  ResourceLimitPeakInfo,               FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemTransferMemoryCountPeak,                            626,  ResourceLimitPeakInfo,               FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemSessionCountPeak,                                   627,  ResourceLimitPeakInfo,               FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(GpuCrashHash,                                             628,  GpuCrashInfo,                        FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(TouchScreenPanelGpioValue,                                629,  TouchScreenInfo,                     FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BrowserCertificateHostName,                               630,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(BrowserCertificateCommonName,                             631,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(BrowserCertificateOrganizationalUnitName,                 632,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(FsPooledBufferFailedIdealAllocationCountOnAsyncAccess,    633,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AudioOutputTarget,                                        634,  AudioDeviceInfo,                     FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AudioOutputChannelCount,                                  635,  AudioDeviceInfo,                     FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AppletTotalActiveTime,                                    636,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(WakeCount,                                                637,  AbnormalWakeInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PredominantWakeReason,                                    638,  AbnormalWakeInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(EdidExtensionBlock2,                                      639,  EdidInfo,                            FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EdidExtensionBlock3,                                      640,  EdidInfo,                            FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(LumenRequestId,                                           641,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(LlnwLlid,                                                 642,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(SupportingLimitedApplicationLicenses,                     643,  RunningApplicationInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(RuntimeLimitedApplicationLicenseUpgrade,                  644,  RunningApplicationInfo,              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ServiceProfileRevisionKey,                                645,  ServiceProfileInfo,                  FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(BluetoothAudioConnectionCount,                            646,  BluetoothAudioInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothHidPairingInfoCountDeprecated,                   647,  BluetoothPairingCountInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothAudioPairingInfoCountDeprecated,                 648,  BluetoothPairingCountInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothLePairingInfoCountDeprecated,                    649,  BluetoothPairingCountInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFilePeakOpenCount,                          650,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisSystemDirectoryPeakOpenCount,                     651,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserFilePeakOpenCount,                            652,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserDirectoryPeakOpenCount,                       653,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardFilePeakOpenCount,                             654,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardDirectoryPeakOpenCount,                        655,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(SslAlertInfo,                                             656,  NetworkSecurityCertificateInfo,      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(SslVersionInfo,                                           657,  NetworkSecurityCertificateInfo,      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(FatFsBisSystemUniqueFileEntryPeakOpenCount,               658,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisSystemUniqueDirectoryEntryPeakOpenCount,          659,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserUniqueFileEntryPeakOpenCount,                 660,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserUniqueDirectoryEntryPeakOpenCount,            661,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardUniqueFileEntryPeakOpenCount,                  662,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardUniqueDirectoryEntryPeakOpenCount,             663,  FsProxyErrorInfo,                    FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(ServerErrorIsRetryable,                                   664,  ErrorInfo,                           FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(FsDeepRetryStartCount,                                    665,  FsProxyErrorInfo2,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsUnrecoverableByGameCardAccessFailedCount,               666,  FsProxyErrorInfo2,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(BuiltInWirelessOUI,                                       667,  BuiltInWirelessOUIInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(WirelessAPOUI,                                            668,  WirelessAPOUIInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(EthernetAdapterOUI,                                       669,  EthernetAdapterOUIInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFatSafeControlResult,                       670,  FsProxyErrorInfo2,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFatErrorNumber,                             671,  FsProxyErrorInfo2,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFatSafeErrorNumber,                         672,  FsProxyErrorInfo2,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsBisUserFatSafeControlResult,                         673,  FsProxyErrorInfo2,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(FatFsBisUserFatErrorNumber,                               674,  FsProxyErrorInfo2,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsBisUserFatSafeErrorNumber,                           675,  FsProxyErrorInfo2,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(GpuCrashDump2,                                            676,  GpuCrashInfo,                        FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NANDTypeDeprecated,                                       677,  NANDTypeInfoDeprecated,              FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(MicroSDType,                                              678,  MicroSDTypeInfo,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardLastDeactivateReasonResult,                       679,  GameCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLastDeactivateReason,                             680,  GameCardErrorInfo,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(InvalidErrorCode,                                         681,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(AppletId,                                                 682,  ApplicationInfo,                     FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PrevReportIdentifier,                                     683,  ErrorInfoAuto,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(SyslogStartupTimeBase,                                    684,  ErrorInfoAuto,                       FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(NxdmpIsAttached,                                          685,  AttachmentFileInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ScreenshotIsAttached,                                     686,  AttachmentFileInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(SyslogIsAttached,                                         687,  AttachmentFileInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(SaveSyslogResult,                                         688,  AttachmentFileInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(EncryptionKeyGeneration,                                  689,  ErrorInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FsBufferManagerNonBlockingRetriedCount,                   690,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPooledBufferNonBlockingRetriedCount,                    691,  FsMemoryInfo,                        FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(LastConnectionTestDownloadSpeed64,                        692,  ConnectionInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(LastConnectionTestUploadSpeed64,                          693,  ConnectionInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(EncryptionKeyV1,                                          694,  ErrorInfo,                           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GpuCrashDumpAttachmentId,                                 695,  GpuCrashInfo,                        FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GpuCrashDumpIsAttached,                                   696,  AttachmentFileInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(CallerIdentifier,                                         697,  ErrorInfo,                           FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(WlanMainState,                                            698,  WlanInfo,                            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(WlanSubState,                                             699,  WlanInfo,                            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AdspSyslogIsAttached,                                     702,  AttachmentFileInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LastHalfAwakeTime,                                        703,  HalfAwakeStateInfo,                  FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(LastHalfAwakeTimeAfterBackgroundTaskDone,                 704,  HalfAwakeStateInfo,                  FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(LastHalfAwakeTimeAfterStateUnlocked,                      705,  HalfAwakeStateInfo,                  FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(LastHalfAwakePowerStateMessage,                           706,  HalfAwakeStateInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FastlyRequestId,                                          707,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(CloudflareCfRay,                                          708,  ErrorInfo,                           FieldType_String,     FieldFlag_None   ) \
    HANDLER(WlanCommandEventHistory,                                  709,  WlanInfo,                            FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(FsSaveDataAttributeCheckFailureCount,                     710,  FsProxyErrorInfo2,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PctlIsRestrictionEnabled,                                 711,  PctlSettingInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PctlIsPairingActive,                                      712,  PctlSettingInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PctlSafetyLevel,                                          713,  PctlSettingInfo,                     FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PctlRatingAge,                                            714,  PctlSettingInfo,                     FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PctlRatingOrganization,                                   715,  PctlSettingInfo,                     FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PctlIsSnsPostRestricted,                                  716,  PctlSettingInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PctlIsFreeCommunicationRestrictedByDefault,               717,  PctlSettingInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PctlIsStereoVisionRestricted,                             718,  PctlSettingInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PctlRestrictedFreeCommunicationApplicationIdList,         719,  PctlSettingInfo,                     FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(PctlExemptApplicationIdList,                              720,  PctlSettingInfo,                     FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(GameCardLogEncryptionKeyIndex,                            721,  GameCardLogInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLogEncryptedKey,                                  722,  GameCardLogInfo,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardAsicHandlerLogLength,                             723,  GameCardLogInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardWorkerLogLength,                                  724,  GameCardLogInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardAsicHandlerLogTimeStamp,                          725,  GameCardLogInfo,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(GameCardWorkerLogTimeStamp,                               726,  GameCardLogInfo,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(GameCardEncryptedAsicHandlerLog,                          727,  GameCardLogInfo,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardEncryptedWorkerLog,                               728,  GameCardLogInfo,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(WlanIoctlErrno,                                           729,  ErrorInfo,                           FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FsSaveDataCertificateVerificationFailureCount,            730,  FsProxyErrorInfo2,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(SdCardActivationMilliSeconds,                             731,  SdCardActivationInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLastAwakenFailureResult,                          732,  GameCardDetailedErrorInfo,           FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardInsertedTimestamp,                                733,  GameCardDetailedErrorInfo,           FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(GameCardPreviousInsertedTimestamp,                        734,  GameCardDetailedErrorInfo,           FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(TestStringNx,                                             1000, TestNx,                              FieldType_String,     FieldFlag_None   ) \
    HANDLER(BoostModeCurrentLimit,                                    1001, BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ChargeConfiguration,                                      1002, BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(HizMode,                                                  1003, BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PowerSupplyPath,                                          1004, BatteryChargeInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ControllerPowerSupplyAcquired,                            1005, BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(OtgRequested,                                             1006, BatteryChargeInfo,                   FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(AdspExceptionRegisters,                                   1007, AdspErrorInfo,                       FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionSpsr,                                        1008, AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionArmModeRegisters,                            1009, AdspErrorInfo,                       FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionStackAddress,                                1010, AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionStackDump,                                   1011, AdspErrorInfo,                       FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionReason,                                      1012, AdspErrorInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CpuDvfsTableClocks,                                       1013, PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(CpuDvfsTableVoltages,                                     1014, PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(GpuDvfsTableClocks,                                       1015, PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(GpuDvfsTableVoltages,                                     1016, PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(EmcDvfsTableClocks,                                       1017, PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(EmcDvfsTableVoltages,                                     1018, PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(PowerDomainEnableFlags,                                   1019, PowerClockInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(PowerDomainVoltages,                                      1020, PowerClockInfo,                      FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(FuseInfo,                                                 1021, PowerClockInfo,                      FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(NANDType,                                                 1022, NANDTypeInfo,                        FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(BluetoothHidPairingInfoCount,                             1023, BluetoothPairingCountInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothAudioPairingInfoCount,                           1024, BluetoothPairingCountInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothLePairingInfoCount,                              1025, BluetoothPairingCountInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NANDPreEolInfo,                                           1026, NANDExtendedCsd,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDDeviceLifeTimeEstTypA,                                1027, NANDExtendedCsd,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDDeviceLifeTimeEstTypB,                                1028, NANDExtendedCsd,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(OscillatorClock,                                          1029, PowerClockInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(DramId,                                                   1030, PowerClockInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(LastDvfsThresholdTripped,                                 1031, ThermalInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ModuleClockEnableFlags,                                   1032, PowerClockInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ModulePowerEnableFlags,                                   1033, PowerClockInfo,                      FieldType_U8Array,    FieldFlag_None   ) \

