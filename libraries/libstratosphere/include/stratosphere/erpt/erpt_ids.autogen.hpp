/*
 * Copyright (c) 2018-2020 Atmosphère-NX
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
    HANDLER(FieldType_I8Array,    15) \

#define AMS_ERPT_FOREACH_CATEGORY(HANDLER) \
    HANDLER(Test,                                0  ) \
    HANDLER(ErrorInfo,                           1  ) \
    HANDLER(ConnectionStatusInfo,                2  ) \
    HANDLER(NetworkInfo,                         3  ) \
    HANDLER(NXMacAddressInfo,                    4  ) \
    HANDLER(StealthNetworkInfo,                  5  ) \
    HANDLER(LimitHighCapacityInfo,               6  ) \
    HANDLER(NATTypeInfo,                         7  ) \
    HANDLER(WirelessAPMacAddressInfo,            8  ) \
    HANDLER(GlobalIPAddressInfo,                 9  ) \
    HANDLER(EnableWirelessInterfaceInfo,         10 ) \
    HANDLER(EnableWifiInfo,                      11 ) \
    HANDLER(EnableBluetoothInfo,                 12 ) \
    HANDLER(EnableNFCInfo,                       13 ) \
    HANDLER(NintendoZoneSSIDListVersionInfo,     14 ) \
    HANDLER(LANAdapterMacAddressInfo,            15 ) \
    HANDLER(ApplicationInfo,                     16 ) \
    HANDLER(OccurrenceInfo,                      17 ) \
    HANDLER(ProductModelInfo,                    18 ) \
    HANDLER(CurrentLanguageInfo,                 19 ) \
    HANDLER(UseNetworkTimeProtocolInfo,          20 ) \
    HANDLER(TimeZoneInfo,                        21 ) \
    HANDLER(ControllerFirmwareInfo,              22 ) \
    HANDLER(VideoOutputInfo,                     23 ) \
    HANDLER(NANDFreeSpaceInfo,                   24 ) \
    HANDLER(SDCardFreeSpaceInfo,                 25 ) \
    HANDLER(ScreenBrightnessInfo,                26 ) \
    HANDLER(AudioFormatInfo,                     27 ) \
    HANDLER(MuteOnHeadsetUnpluggedInfo,          28 ) \
    HANDLER(NumUserRegisteredInfo,               29 ) \
    HANDLER(DataDeletionInfo,                    30 ) \
    HANDLER(ControllerVibrationInfo,             31 ) \
    HANDLER(LockScreenInfo,                      32 ) \
    HANDLER(InternalBatteryLotNumberInfo,        33 ) \
    HANDLER(LeftControllerSerialNumberInfo,      34 ) \
    HANDLER(RightControllerSerialNumberInfo,     35 ) \
    HANDLER(NotificationInfo,                    36 ) \
    HANDLER(TVInfo,                              37 ) \
    HANDLER(SleepInfo,                           38 ) \
    HANDLER(ConnectionInfo,                      39 ) \
    HANDLER(NetworkErrorInfo,                    40 ) \
    HANDLER(FileAccessPathInfo,                  41 ) \
    HANDLER(GameCardCIDInfo,                     42 ) \
    HANDLER(NANDCIDInfo,                         43 ) \
    HANDLER(MicroSDCIDInfo,                      44 ) \
    HANDLER(NANDSpeedModeInfo,                   45 ) \
    HANDLER(MicroSDSpeedModeInfo,                46 ) \
    HANDLER(GameCardSpeedModeInfo,               47 ) \
    HANDLER(UserAccountInternalIDInfo,           48 ) \
    HANDLER(NetworkServiceAccountInternalIDInfo, 49 ) \
    HANDLER(NintendoAccountInternalIDInfo,       50 ) \
    HANDLER(USB3AvailableInfo,                   51 ) \
    HANDLER(CallStackInfo,                       52 ) \
    HANDLER(SystemStartupLogInfo,                53 ) \
    HANDLER(RegionSettingInfo,                   54 ) \
    HANDLER(NintendoZoneConnectedInfo,           55 ) \
    HANDLER(ForceSleepInfo,                      56 ) \
    HANDLER(ChargerInfo,                         57 ) \
    HANDLER(RadioStrengthInfo,                   58 ) \
    HANDLER(ErrorInfoAuto,                       59 ) \
    HANDLER(AccessPointInfo,                     60 ) \
    HANDLER(SystemPowerStateInfo,                62 ) \
    HANDLER(PerformanceInfo,                     63 ) \
    HANDLER(ThrottlingInfo,                      64 ) \
    HANDLER(GameCardErrorInfo,                   65 ) \
    HANDLER(EdidInfo,                            66 ) \
    HANDLER(ThermalInfo,                         67 ) \
    HANDLER(CradleFirmwareInfo,                  68 ) \
    HANDLER(RunningApplicationInfo,              69 ) \
    HANDLER(RunningAppletInfo,                   70 ) \
    HANDLER(FocusedAppletHistoryInfo,            71 ) \
    HANDLER(BatteryChargeInfo,                   73 ) \
    HANDLER(NANDExtendedCsd,                     74 ) \
    HANDLER(NANDPatrolInfo,                      75 ) \
    HANDLER(NANDErrorInfo,                       76 ) \
    HANDLER(NANDDriverLog,                       77 ) \
    HANDLER(SdCardSizeSpec,                      78 ) \
    HANDLER(SdCardErrorInfo,                     79 ) \
    HANDLER(SdCardDriverLog ,                    80 ) \
    HANDLER(FsProxyErrorInfo,                    81 ) \
    HANDLER(SystemAppletSceneInfo,               82 ) \
    HANDLER(VideoInfo,                           83 ) \
    HANDLER(GpuErrorInfo,                        84 ) \
    HANDLER(PowerClockInfo,                      85 ) \
    HANDLER(AdspErrorInfo,                       86 ) \
    HANDLER(NvDispDeviceInfo,                    87 ) \
    HANDLER(NvDispDcWindowInfo,                  88 ) \
    HANDLER(NvDispDpModeInfo,                    89 ) \
    HANDLER(NvDispDpLinkSpec,                    90 ) \
    HANDLER(NvDispDpLinkStatus,                  91 ) \
    HANDLER(NvDispDpHdcpInfo,                    92 ) \
    HANDLER(NvDispDpAuxCecInfo,                  93 ) \
    HANDLER(NvDispDcInfo,                        94 ) \
    HANDLER(NvDispDsiInfo,                       95 ) \
    HANDLER(NvDispErrIDInfo,                     96 ) \
    HANDLER(SdCardMountInfo,                     97 ) \
    HANDLER(RetailInteractiveDisplayInfo,        98 ) \
    HANDLER(CompositorStateInfo,                 99 ) \
    HANDLER(CompositorLayerInfo,                 100) \
    HANDLER(CompositorDisplayInfo,               101) \
    HANDLER(CompositorHWCInfo,                   102) \
    HANDLER(MonitorCapability,                   103) \
    HANDLER(ErrorReportSharePermissionInfo,      104) \
    HANDLER(MultimediaInfo,                      105) \
    HANDLER(ConnectedControllerInfo,             106) \
    HANDLER(FsMemoryInfo,                        107) \
    HANDLER(UserClockContextInfo,                108) \
    HANDLER(NetworkClockContextInfo,             109) \
    HANDLER(AcpGeneralSettingsInfo,              110) \
    HANDLER(AcpPlayLogSettingsInfo,              111) \
    HANDLER(AcpAocSettingsInfo,                  112) \
    HANDLER(AcpBcatSettingsInfo,                 113) \
    HANDLER(AcpStorageSettingsInfo,              114) \
    HANDLER(AcpRatingSettingsInfo,               115) \
    HANDLER(MonitorSettings,                     116) \
    HANDLER(RebootlessSystemUpdateVersionInfo,   117) \
    HANDLER(NifmConnectionTestInfo,              118) \
    HANDLER(PcieLoggedStateInfo,                 119) \
    HANDLER(NetworkSecurityCertificateInfo,      120) \
    HANDLER(AcpNeighborDetectionInfo,            121) \
    HANDLER(GpuCrashInfo,                        122) \
    HANDLER(UsbStateInfo,                        123) \
    HANDLER(NvHostErrInfo,                       124) \
    HANDLER(RunningUlaInfo,                      125) \

#define AMS_ERPT_FOREACH_FIELD(HANDLER) \
    HANDLER(TestU64,                                                  0,   Test,                                FieldType_NumericU64) \
    HANDLER(TestU32,                                                  1,   Test,                                FieldType_NumericU32) \
    HANDLER(TestI64,                                                  2,   Test,                                FieldType_NumericI64) \
    HANDLER(TestI32,                                                  3,   Test,                                FieldType_NumericI32) \
    HANDLER(TestString,                                               4,   Test,                                FieldType_String    ) \
    HANDLER(TestU8Array,                                              5,   Test,                                FieldType_U8Array   ) \
    HANDLER(TestU32Array,                                             6,   Test,                                FieldType_U32Array  ) \
    HANDLER(TestU64Array,                                             7,   Test,                                FieldType_U64Array  ) \
    HANDLER(TestI32Array,                                             8,   Test,                                FieldType_I32Array  ) \
    HANDLER(TestI64Array,                                             9,   Test,                                FieldType_I64Array  ) \
    HANDLER(ErrorCode,                                                10,  ErrorInfo,                           FieldType_String    ) \
    HANDLER(ErrorDescription,                                         11,  ErrorInfo,                           FieldType_String    ) \
    HANDLER(OccurrenceTimestamp,                                      12,  ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(ReportIdentifier,                                         13,  ErrorInfoAuto,                       FieldType_String    ) \
    HANDLER(ConnectionStatus,                                         14,  ConnectionStatusInfo,                FieldType_String    ) \
    HANDLER(AccessPointSSID,                                          15,  AccessPointInfo,                     FieldType_String    ) \
    HANDLER(AccessPointSecurityType,                                  16,  AccessPointInfo,                     FieldType_String    ) \
    HANDLER(RadioStrength,                                            17,  RadioStrengthInfo,                   FieldType_NumericU32) \
    HANDLER(NXMacAddress,                                             18,  NXMacAddressInfo,                    FieldType_String    ) \
    HANDLER(IPAddressAcquisitionMethod,                               19,  NetworkInfo,                         FieldType_NumericU32) \
    HANDLER(CurrentIPAddress,                                         20,  NetworkInfo,                         FieldType_String    ) \
    HANDLER(SubnetMask,                                               21,  NetworkInfo,                         FieldType_String    ) \
    HANDLER(GatewayIPAddress,                                         22,  NetworkInfo,                         FieldType_String    ) \
    HANDLER(DNSType,                                                  23,  NetworkInfo,                         FieldType_NumericU32) \
    HANDLER(PriorityDNSIPAddress,                                     24,  NetworkInfo,                         FieldType_String    ) \
    HANDLER(AlternateDNSIPAddress,                                    25,  NetworkInfo,                         FieldType_String    ) \
    HANDLER(UseProxyFlag,                                             26,  NetworkInfo,                         FieldType_Bool      ) \
    HANDLER(ProxyIPAddress,                                           27,  NetworkInfo,                         FieldType_String    ) \
    HANDLER(ProxyPort,                                                28,  NetworkInfo,                         FieldType_NumericU32) \
    HANDLER(ProxyAutoAuthenticateFlag,                                29,  NetworkInfo,                         FieldType_Bool      ) \
    HANDLER(MTU,                                                      30,  NetworkInfo,                         FieldType_NumericU32) \
    HANDLER(ConnectAutomaticallyFlag,                                 31,  NetworkInfo,                         FieldType_Bool      ) \
    HANDLER(UseStealthNetworkFlag,                                    32,  StealthNetworkInfo,                  FieldType_Bool      ) \
    HANDLER(LimitHighCapacityFlag,                                    33,  LimitHighCapacityInfo,               FieldType_Bool      ) \
    HANDLER(NATType,                                                  34,  NATTypeInfo,                         FieldType_String    ) \
    HANDLER(WirelessAPMacAddress,                                     35,  WirelessAPMacAddressInfo,            FieldType_String    ) \
    HANDLER(GlobalIPAddress,                                          36,  GlobalIPAddressInfo,                 FieldType_String    ) \
    HANDLER(EnableWirelessInterfaceFlag,                              37,  EnableWirelessInterfaceInfo,         FieldType_Bool      ) \
    HANDLER(EnableWifiFlag,                                           38,  EnableWifiInfo,                      FieldType_Bool      ) \
    HANDLER(EnableBluetoothFlag,                                      39,  EnableBluetoothInfo,                 FieldType_Bool      ) \
    HANDLER(EnableNFCFlag,                                            40,  EnableNFCInfo,                       FieldType_Bool      ) \
    HANDLER(NintendoZoneSSIDListVersion,                              41,  NintendoZoneSSIDListVersionInfo,     FieldType_String    ) \
    HANDLER(LANAdapterMacAddress,                                     42,  LANAdapterMacAddressInfo,            FieldType_String    ) \
    HANDLER(ApplicationID,                                            43,  ApplicationInfo,                     FieldType_String    ) \
    HANDLER(ApplicationTitle,                                         44,  ApplicationInfo,                     FieldType_String    ) \
    HANDLER(ApplicationVersion,                                       45,  ApplicationInfo,                     FieldType_String    ) \
    HANDLER(ApplicationStorageLocation,                               46,  ApplicationInfo,                     FieldType_String    ) \
    HANDLER(DownloadContentType,                                      47,  OccurrenceInfo,                      FieldType_String    ) \
    HANDLER(InstallContentType,                                       48,  OccurrenceInfo,                      FieldType_String    ) \
    HANDLER(ConsoleStartingUpFlag,                                    49,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(SystemStartingUpFlag,                                     50,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(ConsoleFirstInitFlag,                                     51,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(HomeMenuScreenDisplayedFlag,                              52,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(DataManagementScreenDisplayedFlag,                        53,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(ConnectionTestingFlag,                                    54,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(ApplicationRunningFlag,                                   55,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(DataCorruptionDetectedFlag,                               56,  OccurrenceInfo,                      FieldType_Bool      ) \
    HANDLER(ProductModel,                                             57,  ProductModelInfo,                    FieldType_String    ) \
    HANDLER(CurrentLanguage,                                          58,  CurrentLanguageInfo,                 FieldType_String    ) \
    HANDLER(UseNetworkTimeProtocolFlag,                               59,  UseNetworkTimeProtocolInfo,          FieldType_Bool      ) \
    HANDLER(TimeZone,                                                 60,  TimeZoneInfo,                        FieldType_String    ) \
    HANDLER(ControllerFirmware,                                       61,  ControllerFirmwareInfo,              FieldType_String    ) \
    HANDLER(VideoOutputSetting,                                       62,  VideoOutputInfo,                     FieldType_String    ) \
    HANDLER(NANDFreeSpace,                                            63,  NANDFreeSpaceInfo,                   FieldType_NumericU64) \
    HANDLER(SDCardFreeSpace,                                          64,  SDCardFreeSpaceInfo,                 FieldType_NumericU64) \
    HANDLER(SerialNumber,                                             65,  ErrorInfoAuto,                       FieldType_String    ) \
    HANDLER(OsVersion,                                                66,  ErrorInfoAuto,                       FieldType_String    ) \
    HANDLER(ScreenBrightnessAutoAdjustFlag,                           67,  ScreenBrightnessInfo,                FieldType_Bool      ) \
    HANDLER(HdmiAudioOutputMode,                                      68,  AudioFormatInfo,                     FieldType_String    ) \
    HANDLER(SpeakerAudioOutputMode,                                   69,  AudioFormatInfo,                     FieldType_String    ) \
    HANDLER(HeadphoneAudioOutputMode,                                 70,  AudioFormatInfo,                     FieldType_String    ) \
    HANDLER(MuteOnHeadsetUnpluggedFlag,                               71,  MuteOnHeadsetUnpluggedInfo,          FieldType_Bool      ) \
    HANDLER(NumUserRegistered,                                        72,  NumUserRegisteredInfo,               FieldType_NumericI32) \
    HANDLER(StorageAutoOrganizeFlag,                                  73,  DataDeletionInfo,                    FieldType_Bool      ) \
    HANDLER(ControllerVibrationVolume,                                74,  ControllerVibrationInfo,             FieldType_String    ) \
    HANDLER(LockScreenFlag,                                           75,  LockScreenInfo,                      FieldType_Bool      ) \
    HANDLER(InternalBatteryLotNumber,                                 76,  InternalBatteryLotNumberInfo,        FieldType_String    ) \
    HANDLER(LeftControllerSerialNumber,                               77,  LeftControllerSerialNumberInfo,      FieldType_String    ) \
    HANDLER(RightControllerSerialNumber,                              78,  RightControllerSerialNumberInfo,     FieldType_String    ) \
    HANDLER(NotifyInGameDownloadCompletionFlag,                       79,  NotificationInfo,                    FieldType_Bool      ) \
    HANDLER(NotificationSoundFlag,                                    80,  NotificationInfo,                    FieldType_Bool      ) \
    HANDLER(TVResolutionSetting,                                      81,  TVInfo,                              FieldType_String    ) \
    HANDLER(RGBRangeSetting,                                          82,  TVInfo,                              FieldType_String    ) \
    HANDLER(ReduceScreenBurnFlag,                                     83,  TVInfo,                              FieldType_Bool      ) \
    HANDLER(TVAllowsCecFlag,                                          84,  TVInfo,                              FieldType_Bool      ) \
    HANDLER(HandheldModeTimeToScreenSleep,                            85,  SleepInfo,                           FieldType_String    ) \
    HANDLER(ConsoleModeTimeToScreenSleep,                             86,  SleepInfo,                           FieldType_String    ) \
    HANDLER(StopAutoSleepDuringContentPlayFlag,                       87,  SleepInfo,                           FieldType_Bool      ) \
    HANDLER(LastConnectionTestDownloadSpeed,                          88,  ConnectionInfo,                      FieldType_NumericU32) \
    HANDLER(LastConnectionTestUploadSpeed,                            89,  ConnectionInfo,                      FieldType_NumericU32) \
    HANDLER(DEPRECATED_ServerFQDN,                                    90,  NetworkErrorInfo,                    FieldType_String    ) \
    HANDLER(HTTPRequestContents,                                      91,  NetworkErrorInfo,                    FieldType_String    ) \
    HANDLER(HTTPRequestResponseContents,                              92,  NetworkErrorInfo,                    FieldType_String    ) \
    HANDLER(EdgeServerIPAddress,                                      93,  NetworkErrorInfo,                    FieldType_String    ) \
    HANDLER(CDNContentPath,                                           94,  NetworkErrorInfo,                    FieldType_String    ) \
    HANDLER(FileAccessPath,                                           95,  FileAccessPathInfo,                  FieldType_String    ) \
    HANDLER(GameCardCID,                                              96,  GameCardCIDInfo,                     FieldType_U8Array   ) \
    HANDLER(NANDCID,                                                  97,  NANDCIDInfo,                         FieldType_U8Array   ) \
    HANDLER(MicroSDCID,                                               98,  MicroSDCIDInfo,                      FieldType_U8Array   ) \
    HANDLER(NANDSpeedMode,                                            99,  NANDSpeedModeInfo,                   FieldType_String    ) \
    HANDLER(MicroSDSpeedMode,                                         100, MicroSDSpeedModeInfo,                FieldType_String    ) \
    HANDLER(GameCardSpeedMode,                                        101, GameCardSpeedModeInfo,               FieldType_String    ) \
    HANDLER(UserAccountInternalID,                                    102, UserAccountInternalIDInfo,           FieldType_String    ) \
    HANDLER(NetworkServiceAccountInternalID,                          103, NetworkServiceAccountInternalIDInfo, FieldType_String    ) \
    HANDLER(NintendoAccountInternalID,                                104, NintendoAccountInternalIDInfo,       FieldType_String    ) \
    HANDLER(USB3AvailableFlag,                                        105, USB3AvailableInfo,                   FieldType_Bool      ) \
    HANDLER(CallStack,                                                106, CallStackInfo,                       FieldType_String    ) \
    HANDLER(SystemStartupLog,                                         107, SystemStartupLogInfo,                FieldType_String    ) \
    HANDLER(RegionSetting,                                            108, RegionSettingInfo,                   FieldType_String    ) \
    HANDLER(NintendoZoneConnectedFlag,                                109, NintendoZoneConnectedInfo,           FieldType_Bool      ) \
    HANDLER(ForcedSleepHighTemperatureReading,                        110, ForceSleepInfo,                      FieldType_NumericU32) \
    HANDLER(ForcedSleepFanSpeedReading,                               111, ForceSleepInfo,                      FieldType_NumericU32) \
    HANDLER(ForcedSleepHWInfo,                                        112, ForceSleepInfo,                      FieldType_String    ) \
    HANDLER(AbnormalPowerStateInfo,                                   113, ChargerInfo,                         FieldType_NumericU32) \
    HANDLER(ScreenBrightnessLevel,                                    114, ScreenBrightnessInfo,                FieldType_String    ) \
    HANDLER(ProgramId,                                                115, ErrorInfo,                           FieldType_String    ) \
    HANDLER(AbortFlag,                                                116, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(ReportVisibilityFlag,                                     117, ErrorInfoAuto,                       FieldType_Bool      ) \
    HANDLER(FatalFlag,                                                118, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(OccurrenceTimestampNet,                                   119, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(ResultBacktrace,                                          120, ErrorInfo,                           FieldType_U32Array  ) \
    HANDLER(GeneralRegisterAarch32,                                   121, ErrorInfo,                           FieldType_U32Array  ) \
    HANDLER(StackBacktrace32,                                         122, ErrorInfo,                           FieldType_U32Array  ) \
    HANDLER(ExceptionInfoAarch32,                                     123, ErrorInfo,                           FieldType_U32Array  ) \
    HANDLER(GeneralRegisterAarch64,                                   124, ErrorInfo,                           FieldType_U64Array  ) \
    HANDLER(ExceptionInfoAarch64,                                     125, ErrorInfo,                           FieldType_U64Array  ) \
    HANDLER(StackBacktrace64,                                         126, ErrorInfo,                           FieldType_U64Array  ) \
    HANDLER(RegisterSetFlag32,                                        127, ErrorInfo,                           FieldType_NumericU32) \
    HANDLER(RegisterSetFlag64,                                        128, ErrorInfo,                           FieldType_NumericU64) \
    HANDLER(ProgramMappedAddr32,                                      129, ErrorInfo,                           FieldType_NumericU32) \
    HANDLER(ProgramMappedAddr64,                                      130, ErrorInfo,                           FieldType_NumericU64) \
    HANDLER(AbortType,                                                131, ErrorInfo,                           FieldType_NumericU32) \
    HANDLER(PrivateOsVersion,                                         132, ErrorInfoAuto,                       FieldType_String    ) \
    HANDLER(CurrentSystemPowerState,                                  133, SystemPowerStateInfo,                FieldType_NumericU32) \
    HANDLER(PreviousSystemPowerState,                                 134, SystemPowerStateInfo,                FieldType_NumericU32) \
    HANDLER(DestinationSystemPowerState,                              135, SystemPowerStateInfo,                FieldType_NumericU32) \
    HANDLER(PscTransitionCurrentState,                                136, ErrorInfo,                           FieldType_NumericU32) \
    HANDLER(PscTransitionPreviousState,                               137, ErrorInfo,                           FieldType_NumericU32) \
    HANDLER(PscInitializedList,                                       138, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(PscCurrentPmStateList,                                    139, ErrorInfo,                           FieldType_U32Array  ) \
    HANDLER(PscNextPmStateList,                                       140, ErrorInfo,                           FieldType_U32Array  ) \
    HANDLER(PerformanceMode,                                          141, PerformanceInfo,                     FieldType_NumericI32) \
    HANDLER(PerformanceConfiguration,                                 142, PerformanceInfo,                     FieldType_NumericU32) \
    HANDLER(Throttled,                                                143, ThrottlingInfo,                      FieldType_Bool      ) \
    HANDLER(ThrottlingDuration,                                       144, ThrottlingInfo,                      FieldType_NumericI64) \
    HANDLER(ThrottlingTimestamp,                                      145, ThrottlingInfo,                      FieldType_NumericI64) \
    HANDLER(GameCardCrcErrorCount,                                    146, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardAsicCrcErrorCount,                                147, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardRefreshCount,                                     148, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardReadRetryCount,                                   149, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(EdidBlock,                                                150, EdidInfo,                            FieldType_U8Array   ) \
    HANDLER(EdidExtensionBlock,                                       151, EdidInfo,                            FieldType_U8Array   ) \
    HANDLER(CreateProcessFailureFlag,                                 152, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(TemperaturePcb,                                           153, ThermalInfo,                         FieldType_NumericI32) \
    HANDLER(TemperatureSoc,                                           154, ThermalInfo,                         FieldType_NumericI32) \
    HANDLER(CurrentFanDuty,                                           155, ThermalInfo,                         FieldType_NumericI32) \
    HANDLER(LastDvfsThresholdTripped,                                 156, ThermalInfo,                         FieldType_NumericI32) \
    HANDLER(CradlePdcHFwVersion,                                      157, CradleFirmwareInfo,                  FieldType_NumericU32) \
    HANDLER(CradlePdcAFwVersion,                                      158, CradleFirmwareInfo,                  FieldType_NumericU32) \
    HANDLER(CradleMcuFwVersion,                                       159, CradleFirmwareInfo,                  FieldType_NumericU32) \
    HANDLER(CradleDp2HdmiFwVersion,                                   160, CradleFirmwareInfo,                  FieldType_NumericU32) \
    HANDLER(RunningApplicationId,                                     161, RunningApplicationInfo,              FieldType_String    ) \
    HANDLER(RunningApplicationTitle,                                  162, RunningApplicationInfo,              FieldType_String    ) \
    HANDLER(RunningApplicationVersion,                                163, RunningApplicationInfo,              FieldType_String    ) \
    HANDLER(RunningApplicationStorageLocation,                        164, RunningApplicationInfo,              FieldType_String    ) \
    HANDLER(RunningAppletList,                                        165, RunningAppletInfo,                   FieldType_U64Array  ) \
    HANDLER(FocusedAppletHistory,                                     166, FocusedAppletHistoryInfo,            FieldType_U64Array  ) \
    HANDLER(CompositorState,                                          167, CompositorStateInfo,                 FieldType_String    ) \
    HANDLER(CompositorLayerState,                                     168, CompositorLayerInfo,                 FieldType_String    ) \
    HANDLER(CompositorDisplayState,                                   169, CompositorDisplayInfo,               FieldType_String    ) \
    HANDLER(CompositorHWCState,                                       170, CompositorHWCInfo,                   FieldType_String    ) \
    HANDLER(InputCurrentLimit,                                        171, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(BoostModeCurrentLimit,                                    172, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(FastChargeCurrentLimit,                                   173, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(ChargeVoltageLimit,                                       174, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(ChargeConfiguration,                                      175, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(HizMode,                                                  176, BatteryChargeInfo,                   FieldType_Bool      ) \
    HANDLER(ChargeEnabled,                                            177, BatteryChargeInfo,                   FieldType_Bool      ) \
    HANDLER(PowerSupplyPath,                                          178, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(BatteryTemperature,                                       179, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(BatteryChargePercent,                                     180, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(BatteryChargeVoltage,                                     181, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(BatteryAge,                                               182, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(PowerRole,                                                183, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(PowerSupplyType,                                          184, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(PowerSupplyVoltage,                                       185, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(PowerSupplyCurrent,                                       186, BatteryChargeInfo,                   FieldType_NumericI32) \
    HANDLER(FastBatteryChargingEnabled,                               187, BatteryChargeInfo,                   FieldType_Bool      ) \
    HANDLER(ControllerPowerSupplyAcquired,                            188, BatteryChargeInfo,                   FieldType_Bool      ) \
    HANDLER(OtgRequested,                                             189, BatteryChargeInfo,                   FieldType_Bool      ) \
    HANDLER(NANDPreEolInfo,                                           190, NANDExtendedCsd,                     FieldType_NumericU32) \
    HANDLER(NANDDeviceLifeTimeEstTypA,                                191, NANDExtendedCsd,                     FieldType_NumericU32) \
    HANDLER(NANDDeviceLifeTimeEstTypB,                                192, NANDExtendedCsd,                     FieldType_NumericU32) \
    HANDLER(NANDPatrolCount,                                          193, NANDPatrolInfo,                      FieldType_NumericU32) \
    HANDLER(NANDNumActivationFailures,                                194, NANDErrorInfo,                       FieldType_NumericU32) \
    HANDLER(NANDNumActivationErrorCorrections,                        195, NANDErrorInfo,                       FieldType_NumericU32) \
    HANDLER(NANDNumReadWriteFailures,                                 196, NANDErrorInfo,                       FieldType_NumericU32) \
    HANDLER(NANDNumReadWriteErrorCorrections,                         197, NANDErrorInfo,                       FieldType_NumericU32) \
    HANDLER(NANDErrorLog,                                             198, NANDDriverLog,                       FieldType_String    ) \
    HANDLER(SdCardUserAreaSize,                                       199, SdCardSizeSpec,                      FieldType_NumericI64) \
    HANDLER(SdCardProtectedAreaSize,                                  200, SdCardSizeSpec,                      FieldType_NumericI64) \
    HANDLER(SdCardNumActivationFailures,                              201, SdCardErrorInfo,                     FieldType_NumericU32) \
    HANDLER(SdCardNumActivationErrorCorrections,                      202, SdCardErrorInfo,                     FieldType_NumericU32) \
    HANDLER(SdCardNumReadWriteFailures,                               203, SdCardErrorInfo,                     FieldType_NumericU32) \
    HANDLER(SdCardNumReadWriteErrorCorrections,                       204, SdCardErrorInfo,                     FieldType_NumericU32) \
    HANDLER(SdCardErrorLog,                                           205, SdCardDriverLog ,                    FieldType_String    ) \
    HANDLER(EncryptionKey,                                            206, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(EncryptedExceptionInfo,                                   207, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(GameCardTimeoutRetryErrorCount,                           208, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(FsRemountForDataCorruptCount,                             209, FsProxyErrorInfo,                    FieldType_NumericU32) \
    HANDLER(FsRemountForDataCorruptRetryOutCount,                     210, FsProxyErrorInfo,                    FieldType_NumericU32) \
    HANDLER(GameCardInsertionCount,                                   211, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardRemovalCount,                                     212, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardAsicInitializeCount,                              213, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(TestU16,                                                  214, Test,                                FieldType_NumericU16) \
    HANDLER(TestU8,                                                   215, Test,                                FieldType_NumericU8 ) \
    HANDLER(TestI16,                                                  216, Test,                                FieldType_NumericI16) \
    HANDLER(TestI8,                                                   217, Test,                                FieldType_NumericI8 ) \
    HANDLER(SystemAppletScene,                                        218, SystemAppletSceneInfo,               FieldType_NumericU8 ) \
    HANDLER(CodecType,                                                219, VideoInfo,                           FieldType_NumericU32) \
    HANDLER(DecodeBuffers,                                            220, VideoInfo,                           FieldType_NumericU32) \
    HANDLER(FrameWidth,                                               221, VideoInfo,                           FieldType_NumericI32) \
    HANDLER(FrameHeight,                                              222, VideoInfo,                           FieldType_NumericI32) \
    HANDLER(ColorPrimaries,                                           223, VideoInfo,                           FieldType_NumericU8 ) \
    HANDLER(TransferCharacteristics,                                  224, VideoInfo,                           FieldType_NumericU8 ) \
    HANDLER(MatrixCoefficients,                                       225, VideoInfo,                           FieldType_NumericU8 ) \
    HANDLER(DisplayWidth,                                             226, VideoInfo,                           FieldType_NumericI32) \
    HANDLER(DisplayHeight,                                            227, VideoInfo,                           FieldType_NumericI32) \
    HANDLER(DARWidth,                                                 228, VideoInfo,                           FieldType_NumericI32) \
    HANDLER(DARHeight,                                                229, VideoInfo,                           FieldType_NumericI32) \
    HANDLER(ColorFormat,                                              230, VideoInfo,                           FieldType_NumericU32) \
    HANDLER(ColorSpace,                                               231, VideoInfo,                           FieldType_String    ) \
    HANDLER(SurfaceLayout,                                            232, VideoInfo,                           FieldType_String    ) \
    HANDLER(BitStream,                                                233, VideoInfo,                           FieldType_U8Array   ) \
    HANDLER(VideoDecState,                                            234, VideoInfo,                           FieldType_String    ) \
    HANDLER(GpuErrorChannelId,                                        235, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorAruId,                                            236, GpuErrorInfo,                        FieldType_NumericU64) \
    HANDLER(GpuErrorType,                                             237, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorFaultInfo,                                        238, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorWriteAccess,                                      239, GpuErrorInfo,                        FieldType_Bool      ) \
    HANDLER(GpuErrorFaultAddress,                                     240, GpuErrorInfo,                        FieldType_NumericU64) \
    HANDLER(GpuErrorFaultUnit,                                        241, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorFaultType,                                        242, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorHwContextPointer,                                 243, GpuErrorInfo,                        FieldType_NumericU64) \
    HANDLER(GpuErrorContextStatus,                                    244, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorPbdmaIntr,                                        245, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorPbdmaErrorType,                                   246, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorPbdmaHeaderShadow,                                247, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorPbdmaHeader,                                      248, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorPbdmaGpShadow0,                                   249, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(GpuErrorPbdmaGpShadow1,                                   250, GpuErrorInfo,                        FieldType_NumericU32) \
    HANDLER(AccessPointChannel,                                       251, AccessPointInfo,                     FieldType_NumericU16) \
    HANDLER(ThreadName,                                               252, ErrorInfo,                           FieldType_String    ) \
    HANDLER(AdspExceptionRegisters,                                   253, AdspErrorInfo,                       FieldType_U32Array  ) \
    HANDLER(AdspExceptionSpsr,                                        254, AdspErrorInfo,                       FieldType_NumericU32) \
    HANDLER(AdspExceptionProgramCounter,                              255, AdspErrorInfo,                       FieldType_NumericU32) \
    HANDLER(AdspExceptionLinkRegister,                                256, AdspErrorInfo,                       FieldType_NumericU32) \
    HANDLER(AdspExceptionStackPointer,                                257, AdspErrorInfo,                       FieldType_NumericU32) \
    HANDLER(AdspExceptionArmModeRegisters,                            258, AdspErrorInfo,                       FieldType_U32Array  ) \
    HANDLER(AdspExceptionStackAddress,                                259, AdspErrorInfo,                       FieldType_NumericU32) \
    HANDLER(AdspExceptionStackDump,                                   260, AdspErrorInfo,                       FieldType_U32Array  ) \
    HANDLER(AdspExceptionReason,                                      261, AdspErrorInfo,                       FieldType_NumericU32) \
    HANDLER(OscillatorClock,                                          262, PowerClockInfo,                      FieldType_NumericU32) \
    HANDLER(CpuDvfsTableClocks,                                       263, PowerClockInfo,                      FieldType_U32Array  ) \
    HANDLER(CpuDvfsTableVoltages,                                     264, PowerClockInfo,                      FieldType_I32Array  ) \
    HANDLER(GpuDvfsTableClocks,                                       265, PowerClockInfo,                      FieldType_U32Array  ) \
    HANDLER(GpuDvfsTableVoltages,                                     266, PowerClockInfo,                      FieldType_I32Array  ) \
    HANDLER(EmcDvfsTableClocks,                                       267, PowerClockInfo,                      FieldType_U32Array  ) \
    HANDLER(EmcDvfsTableVoltages,                                     268, PowerClockInfo,                      FieldType_I32Array  ) \
    HANDLER(ModuleClockFrequencies,                                   269, PowerClockInfo,                      FieldType_U32Array  ) \
    HANDLER(ModuleClockEnableFlags,                                   270, PowerClockInfo,                      FieldType_U8Array   ) \
    HANDLER(ModulePowerEnableFlags,                                   271, PowerClockInfo,                      FieldType_U8Array   ) \
    HANDLER(ModuleResetAssertFlags,                                   272, PowerClockInfo,                      FieldType_U8Array   ) \
    HANDLER(ModuleMinimumVoltageClockRates,                           273, PowerClockInfo,                      FieldType_U32Array  ) \
    HANDLER(PowerDomainEnableFlags,                                   274, PowerClockInfo,                      FieldType_U8Array   ) \
    HANDLER(PowerDomainVoltages,                                      275, PowerClockInfo,                      FieldType_I32Array  ) \
    HANDLER(AccessPointRssi,                                          276, RadioStrengthInfo,                   FieldType_NumericI32) \
    HANDLER(FuseInfo,                                                 277, PowerClockInfo,                      FieldType_U32Array  ) \
    HANDLER(VideoLog,                                                 278, VideoInfo,                           FieldType_String    ) \
    HANDLER(GameCardDeviceId,                                         279, GameCardCIDInfo,                     FieldType_U8Array   ) \
    HANDLER(GameCardAsicReinitializeCount,                            280, GameCardErrorInfo,                   FieldType_NumericU16) \
    HANDLER(GameCardAsicReinitializeFailureCount,                     281, GameCardErrorInfo,                   FieldType_NumericU16) \
    HANDLER(GameCardAsicReinitializeFailureDetail,                    282, GameCardErrorInfo,                   FieldType_NumericU16) \
    HANDLER(GameCardRefreshSuccessCount,                              283, GameCardErrorInfo,                   FieldType_NumericU16) \
    HANDLER(GameCardAwakenCount,                                      284, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardAwakenFailureCount,                               285, GameCardErrorInfo,                   FieldType_NumericU16) \
    HANDLER(GameCardReadCountFromInsert,                              286, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardReadCountFromAwaken,                              287, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardLastReadErrorPageAddress,                         288, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(GameCardLastReadErrorPageCount,                           289, GameCardErrorInfo,                   FieldType_NumericU32) \
    HANDLER(AppletManagerContextTrace,                                290, ErrorInfo,                           FieldType_I32Array  ) \
    HANDLER(NvDispIsRegistered,                                       291, NvDispDeviceInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispIsSuspend,                                          292, NvDispDeviceInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispDC0SurfaceNum,                                      293, NvDispDeviceInfo,                    FieldType_I32Array  ) \
    HANDLER(NvDispDC1SurfaceNum,                                      294, NvDispDeviceInfo,                    FieldType_I32Array  ) \
    HANDLER(NvDispWindowSrcRectX,                                     295, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowSrcRectY,                                     296, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowSrcRectWidth,                                 297, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowSrcRectHeight,                                298, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowDstRectX,                                     299, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowDstRectY,                                     300, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowDstRectWidth,                                 301, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowDstRectHeight,                                302, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowIndex,                                        303, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowBlendOperation,                               304, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowAlphaOperation,                               305, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowDepth,                                        306, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowAlpha,                                        307, NvDispDcWindowInfo,                  FieldType_NumericU8 ) \
    HANDLER(NvDispWindowHFilter,                                      308, NvDispDcWindowInfo,                  FieldType_Bool      ) \
    HANDLER(NvDispWindowVFilter,                                      309, NvDispDcWindowInfo,                  FieldType_Bool      ) \
    HANDLER(NvDispWindowOptions,                                      310, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispWindowSyncPointId,                                  311, NvDispDcWindowInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispDPSorPower,                                         312, NvDispDpModeInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispDPClkType,                                          313, NvDispDpModeInfo,                    FieldType_NumericU8 ) \
    HANDLER(NvDispDPEnable,                                           314, NvDispDpModeInfo,                    FieldType_NumericU32) \
    HANDLER(NvDispDPState,                                            315, NvDispDpModeInfo,                    FieldType_NumericU32) \
    HANDLER(NvDispDPEdid,                                             316, NvDispDpModeInfo,                    FieldType_U8Array   ) \
    HANDLER(NvDispDPEdidSize,                                         317, NvDispDpModeInfo,                    FieldType_NumericU32) \
    HANDLER(NvDispDPEdidExtSize,                                      318, NvDispDpModeInfo,                    FieldType_NumericU32) \
    HANDLER(NvDispDPFakeMode,                                         319, NvDispDpModeInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispDPModeNumber,                                       320, NvDispDpModeInfo,                    FieldType_NumericU32) \
    HANDLER(NvDispDPPlugInOut,                                        321, NvDispDpModeInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispDPAuxIntHandler,                                    322, NvDispDpModeInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispDPForceMaxLinkBW,                                   323, NvDispDpModeInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispDPIsConnected,                                      324, NvDispDpModeInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispDPLinkValid,                                        325, NvDispDpLinkSpec,                    FieldType_Bool      ) \
    HANDLER(NvDispDPLinkMaxBW,                                        326, NvDispDpLinkSpec,                    FieldType_NumericU8 ) \
    HANDLER(NvDispDPLinkMaxLaneCount,                                 327, NvDispDpLinkSpec,                    FieldType_NumericU8 ) \
    HANDLER(NvDispDPLinkDownSpread,                                   328, NvDispDpLinkSpec,                    FieldType_Bool      ) \
    HANDLER(NvDispDPLinkSupportEnhancedFraming,                       329, NvDispDpLinkSpec,                    FieldType_Bool      ) \
    HANDLER(NvDispDPLinkBpp,                                          330, NvDispDpLinkSpec,                    FieldType_NumericU32) \
    HANDLER(NvDispDPLinkScaramberCap,                                 331, NvDispDpLinkSpec,                    FieldType_Bool      ) \
    HANDLER(NvDispDPLinkBW,                                           332, NvDispDpLinkStatus,                  FieldType_NumericU8 ) \
    HANDLER(NvDispDPLinkLaneCount,                                    333, NvDispDpLinkStatus,                  FieldType_NumericU8 ) \
    HANDLER(NvDispDPLinkEnhancedFraming,                              334, NvDispDpLinkStatus,                  FieldType_Bool      ) \
    HANDLER(NvDispDPLinkScrambleEnable,                               335, NvDispDpLinkStatus,                  FieldType_Bool      ) \
    HANDLER(NvDispDPLinkActivePolarity,                               336, NvDispDpLinkStatus,                  FieldType_NumericU32) \
    HANDLER(NvDispDPLinkActiveCount,                                  337, NvDispDpLinkStatus,                  FieldType_NumericU32) \
    HANDLER(NvDispDPLinkTUSize,                                       338, NvDispDpLinkStatus,                  FieldType_NumericU32) \
    HANDLER(NvDispDPLinkActiveFrac,                                   339, NvDispDpLinkStatus,                  FieldType_NumericU32) \
    HANDLER(NvDispDPLinkWatermark,                                    340, NvDispDpLinkStatus,                  FieldType_NumericU32) \
    HANDLER(NvDispDPLinkHBlank,                                       341, NvDispDpLinkStatus,                  FieldType_NumericU32) \
    HANDLER(NvDispDPLinkVBlank,                                       342, NvDispDpLinkStatus,                  FieldType_NumericU32) \
    HANDLER(NvDispDPLinkOnlyEnhancedFraming,                          343, NvDispDpLinkStatus,                  FieldType_Bool      ) \
    HANDLER(NvDispDPLinkOnlyEdpCap,                                   344, NvDispDpLinkStatus,                  FieldType_Bool      ) \
    HANDLER(NvDispDPLinkSupportFastLT,                                345, NvDispDpLinkStatus,                  FieldType_Bool      ) \
    HANDLER(NvDispDPLinkLTDataValid,                                  346, NvDispDpLinkStatus,                  FieldType_Bool      ) \
    HANDLER(NvDispDPLinkTsp3Support,                                  347, NvDispDpLinkStatus,                  FieldType_Bool      ) \
    HANDLER(NvDispDPLinkAuxInterval,                                  348, NvDispDpLinkStatus,                  FieldType_NumericU8 ) \
    HANDLER(NvDispHdcpCreated,                                        349, NvDispDpHdcpInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispHdcpUserRequest,                                    350, NvDispDpHdcpInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispHdcpPlugged,                                        351, NvDispDpHdcpInfo,                    FieldType_Bool      ) \
    HANDLER(NvDispHdcpState,                                          352, NvDispDpHdcpInfo,                    FieldType_NumericU32) \
    HANDLER(NvDispHdcpFailCount,                                      353, NvDispDpHdcpInfo,                    FieldType_NumericI32) \
    HANDLER(NvDispHdcpHdcp22,                                         354, NvDispDpHdcpInfo,                    FieldType_NumericI8 ) \
    HANDLER(NvDispHdcpMaxRetry,                                       355, NvDispDpHdcpInfo,                    FieldType_NumericU8 ) \
    HANDLER(NvDispHdcpHpd,                                            356, NvDispDpHdcpInfo,                    FieldType_NumericU8 ) \
    HANDLER(NvDispHdcpRepeater,                                       357, NvDispDpHdcpInfo,                    FieldType_NumericU8 ) \
    HANDLER(NvDispCecRxBuf,                                           358, NvDispDpAuxCecInfo,                  FieldType_U8Array   ) \
    HANDLER(NvDispCecRxLength,                                        359, NvDispDpAuxCecInfo,                  FieldType_NumericI32) \
    HANDLER(NvDispCecTxBuf,                                           360, NvDispDpAuxCecInfo,                  FieldType_U8Array   ) \
    HANDLER(NvDispCecTxLength,                                        361, NvDispDpAuxCecInfo,                  FieldType_NumericI32) \
    HANDLER(NvDispCecTxRet,                                           362, NvDispDpAuxCecInfo,                  FieldType_NumericI32) \
    HANDLER(NvDispCecState,                                           363, NvDispDpAuxCecInfo,                  FieldType_NumericU32) \
    HANDLER(NvDispCecTxInfo,                                          364, NvDispDpAuxCecInfo,                  FieldType_NumericU8 ) \
    HANDLER(NvDispCecRxInfo,                                          365, NvDispDpAuxCecInfo,                  FieldType_NumericU8 ) \
    HANDLER(NvDispDCIndex,                                            366, NvDispDcInfo,                        FieldType_NumericU32) \
    HANDLER(NvDispDCInitialize,                                       367, NvDispDcInfo,                        FieldType_Bool      ) \
    HANDLER(NvDispDCClock,                                            368, NvDispDcInfo,                        FieldType_Bool      ) \
    HANDLER(NvDispDCFrequency,                                        369, NvDispDcInfo,                        FieldType_NumericU32) \
    HANDLER(NvDispDCFailed,                                           370, NvDispDcInfo,                        FieldType_Bool      ) \
    HANDLER(NvDispDCModeWidth,                                        371, NvDispDcInfo,                        FieldType_NumericI32) \
    HANDLER(NvDispDCModeHeight,                                       372, NvDispDcInfo,                        FieldType_NumericI32) \
    HANDLER(NvDispDCModeBpp,                                          373, NvDispDcInfo,                        FieldType_NumericU32) \
    HANDLER(NvDispDCPanelFrequency,                                   374, NvDispDcInfo,                        FieldType_NumericU32) \
    HANDLER(NvDispDCWinDirty,                                         375, NvDispDcInfo,                        FieldType_NumericU32) \
    HANDLER(NvDispDCWinEnable,                                        376, NvDispDcInfo,                        FieldType_NumericU32) \
    HANDLER(NvDispDCVrr,                                              377, NvDispDcInfo,                        FieldType_Bool      ) \
    HANDLER(NvDispDCPanelInitialize,                                  378, NvDispDcInfo,                        FieldType_Bool      ) \
    HANDLER(NvDispDsiDataFormat,                                      379, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiVideoMode,                                       380, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiRefreshRate,                                     381, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiLpCmdModeFrequency,                              382, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiHsCmdModeFrequency,                              383, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiPanelResetTimeout,                               384, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiPhyFrequency,                                    385, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiFrequency,                                       386, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDsiInstance,                                        387, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDcDsiHostCtrlEnable,                                388, NvDispDsiInfo,                       FieldType_Bool      ) \
    HANDLER(NvDispDcDsiInit,                                          389, NvDispDsiInfo,                       FieldType_Bool      ) \
    HANDLER(NvDispDcDsiEnable,                                        390, NvDispDsiInfo,                       FieldType_Bool      ) \
    HANDLER(NvDispDcDsiHsMode,                                        391, NvDispDsiInfo,                       FieldType_Bool      ) \
    HANDLER(NvDispDcDsiVendorId,                                      392, NvDispDsiInfo,                       FieldType_U8Array   ) \
    HANDLER(NvDispDcDsiLcdVendorNum,                                  393, NvDispDsiInfo,                       FieldType_NumericU8 ) \
    HANDLER(NvDispDcDsiHsClockControl,                                394, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDcDsiEnableHsClockInLpMode,                         395, NvDispDsiInfo,                       FieldType_Bool      ) \
    HANDLER(NvDispDcDsiTeFrameUpdate,                                 396, NvDispDsiInfo,                       FieldType_Bool      ) \
    HANDLER(NvDispDcDsiGangedType,                                    397, NvDispDsiInfo,                       FieldType_NumericU32) \
    HANDLER(NvDispDcDsiHbpInPktSeq,                                   398, NvDispDsiInfo,                       FieldType_Bool      ) \
    HANDLER(NvDispErrID,                                              399, NvDispErrIDInfo,                     FieldType_NumericU32) \
    HANDLER(NvDispErrData0,                                           400, NvDispErrIDInfo,                     FieldType_NumericU32) \
    HANDLER(NvDispErrData1,                                           401, NvDispErrIDInfo,                     FieldType_NumericU32) \
    HANDLER(SdCardMountStatus,                                        402, SdCardMountInfo,                     FieldType_String    ) \
    HANDLER(SdCardMountUnexpectedResult,                              403, SdCardMountInfo,                     FieldType_String    ) \
    HANDLER(NANDTotalSize,                                            404, NANDFreeSpaceInfo,                   FieldType_NumericU64) \
    HANDLER(SdCardTotalSize,                                          405, SDCardFreeSpaceInfo,                 FieldType_NumericU64) \
    HANDLER(ElapsedTimeSinceInitialLaunch,                            406, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(ElapsedTimeSincePowerOn,                                  407, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(ElapsedTimeSinceLastAwake,                                408, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(OccurrenceTick,                                           409, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(RetailInteractiveDisplayFlag,                             410, RetailInteractiveDisplayInfo,        FieldType_Bool      ) \
    HANDLER(FatFsError,                                               411, FsProxyErrorInfo,                    FieldType_NumericI32) \
    HANDLER(FatFsExtraError,                                          412, FsProxyErrorInfo,                    FieldType_NumericI32) \
    HANDLER(FatFsErrorDrive,                                          413, FsProxyErrorInfo,                    FieldType_NumericI32) \
    HANDLER(FatFsErrorName,                                           414, FsProxyErrorInfo,                    FieldType_String    ) \
    HANDLER(MonitorManufactureCode,                                   415, MonitorCapability,                   FieldType_String    ) \
    HANDLER(MonitorProductCode,                                       416, MonitorCapability,                   FieldType_NumericU16) \
    HANDLER(MonitorSerialNumber,                                      417, MonitorCapability,                   FieldType_NumericU32) \
    HANDLER(MonitorManufactureYear,                                   418, MonitorCapability,                   FieldType_NumericI32) \
    HANDLER(PhysicalAddress,                                          419, MonitorCapability,                   FieldType_NumericU16) \
    HANDLER(Is4k60Hz,                                                 420, MonitorCapability,                   FieldType_Bool      ) \
    HANDLER(Is4k30Hz,                                                 421, MonitorCapability,                   FieldType_Bool      ) \
    HANDLER(Is1080P60Hz,                                              422, MonitorCapability,                   FieldType_Bool      ) \
    HANDLER(Is720P60Hz,                                               423, MonitorCapability,                   FieldType_Bool      ) \
    HANDLER(PcmChannelMax,                                            424, MonitorCapability,                   FieldType_NumericI32) \
    HANDLER(CrashReportHash,                                          425, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(ErrorReportSharePermission,                               426, ErrorReportSharePermissionInfo,      FieldType_NumericU8 ) \
    HANDLER(VideoCodecTypeEnum,                                       427, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(VideoBitRate,                                             428, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(VideoFrameRate,                                           429, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(VideoWidth,                                               430, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(VideoHeight,                                              431, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(AudioCodecTypeEnum,                                       432, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(AudioSampleRate,                                          433, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(AudioChannelCount,                                        434, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(AudioBitRate,                                             435, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(MultimediaContainerType,                                  436, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(MultimediaProfileType,                                    437, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(MultimediaLevelType,                                      438, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(MultimediaCacheSizeEnum,                                  439, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(MultimediaErrorStatusEnum,                                440, MultimediaInfo,                      FieldType_NumericI32) \
    HANDLER(MultimediaErrorLog,                                       441, MultimediaInfo,                      FieldType_U8Array   ) \
    HANDLER(ServerFqdn,                                               442, ErrorInfo,                           FieldType_String    ) \
    HANDLER(ServerIpAddress,                                          443, ErrorInfo,                           FieldType_String    ) \
    HANDLER(TestStringEncrypt,                                        444, Test,                                FieldType_String    ) \
    HANDLER(TestU8ArrayEncrypt,                                       445, Test,                                FieldType_U8Array   ) \
    HANDLER(TestU32ArrayEncrypt,                                      446, Test,                                FieldType_U32Array  ) \
    HANDLER(TestU64ArrayEncrypt,                                      447, Test,                                FieldType_U64Array  ) \
    HANDLER(TestI32ArrayEncrypt,                                      448, Test,                                FieldType_I32Array  ) \
    HANDLER(TestI64ArrayEncrypt,                                      449, Test,                                FieldType_I64Array  ) \
    HANDLER(CipherKey,                                                450, ErrorInfoAuto,                       FieldType_U8Array   ) \
    HANDLER(FileSystemPath,                                           451, ErrorInfo,                           FieldType_String    ) \
    HANDLER(WebMediaPlayerOpenUrl,                                    452, ErrorInfo,                           FieldType_String    ) \
    HANDLER(WebMediaPlayerLastSocketErrors,                           453, ErrorInfo,                           FieldType_I32Array  ) \
    HANDLER(UnknownControllerCount,                                   454, ConnectedControllerInfo,             FieldType_NumericU8 ) \
    HANDLER(AttachedControllerCount,                                  455, ConnectedControllerInfo,             FieldType_NumericU8 ) \
    HANDLER(BluetoothControllerCount,                                 456, ConnectedControllerInfo,             FieldType_NumericU8 ) \
    HANDLER(UsbControllerCount,                                       457, ConnectedControllerInfo,             FieldType_NumericU8 ) \
    HANDLER(ControllerTypeList,                                       458, ConnectedControllerInfo,             FieldType_U8Array   ) \
    HANDLER(ControllerInterfaceList,                                  459, ConnectedControllerInfo,             FieldType_U8Array   ) \
    HANDLER(ControllerStyleList,                                      460, ConnectedControllerInfo,             FieldType_U8Array   ) \
    HANDLER(FsPooledBufferPeakFreeSize,                               461, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsPooledBufferRetriedCount,                               462, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsPooledBufferReduceAllocationCount,                      463, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsBufferManagerPeakFreeSize,                              464, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsBufferManagerRetriedCount,                              465, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsExpHeapPeakFreeSize,                                    466, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsBufferPoolPeakFreeSize,                                 467, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsPatrolReadAllocateBufferSuccessCount,                   468, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(FsPatrolReadAllocateBufferFailureCount,                   469, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(SteadyClockInternalOffset,                                470, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(SteadyClockCurrentTimePointValue,                         471, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(UserClockContextOffset,                                   472, UserClockContextInfo,                FieldType_NumericI64) \
    HANDLER(UserClockContextTimeStampValue,                           473, UserClockContextInfo,                FieldType_NumericI64) \
    HANDLER(NetworkClockContextOffset,                                474, NetworkClockContextInfo,             FieldType_NumericI64) \
    HANDLER(NetworkClockContextTimeStampValue,                        475, NetworkClockContextInfo,             FieldType_NumericI64) \
    HANDLER(SystemAbortFlag,                                          476, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(ApplicationAbortFlag,                                     477, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(NifmErrorCode,                                            478, ConnectionStatusInfo,                FieldType_String    ) \
    HANDLER(LcsApplicationId,                                         479, ErrorInfo,                           FieldType_String    ) \
    HANDLER(LcsContentMetaKeyIdList,                                  480, ErrorInfo,                           FieldType_U64Array  ) \
    HANDLER(LcsContentMetaKeyVersionList,                             481, ErrorInfo,                           FieldType_U32Array  ) \
    HANDLER(LcsContentMetaKeyTypeList,                                482, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(LcsContentMetaKeyInstallTypeList,                         483, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(LcsSenderFlag,                                            484, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(LcsApplicationRequestFlag,                                485, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(LcsHasExFatDriverFlag,                                    486, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(LcsIpAddress,                                             487, ErrorInfo,                           FieldType_NumericU32) \
    HANDLER(AcpStartupUserAccount,                                    488, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpAocRegistrationType,                                   489, AcpAocSettingsInfo,                  FieldType_NumericU8 ) \
    HANDLER(AcpAttributeFlag,                                         490, AcpGeneralSettingsInfo,              FieldType_NumericU32) \
    HANDLER(AcpSupportedLanguageFlag,                                 491, AcpGeneralSettingsInfo,              FieldType_NumericU32) \
    HANDLER(AcpParentalControlFlag,                                   492, AcpGeneralSettingsInfo,              FieldType_NumericU32) \
    HANDLER(AcpScreenShot,                                            493, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpVideoCapture,                                          494, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpDataLossConfirmation,                                  495, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpPlayLogPolicy,                                         496, AcpPlayLogSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpPresenceGroupId,                                       497, AcpGeneralSettingsInfo,              FieldType_NumericU64) \
    HANDLER(AcpRatingAge,                                             498, AcpRatingSettingsInfo,               FieldType_I8Array   ) \
    HANDLER(AcpAocBaseId,                                             499, AcpAocSettingsInfo,                  FieldType_NumericU64) \
    HANDLER(AcpSaveDataOwnerId,                                       500, AcpStorageSettingsInfo,              FieldType_NumericU64) \
    HANDLER(AcpUserAccountSaveDataSize,                               501, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpUserAccountSaveDataJournalSize,                        502, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpDeviceSaveDataSize,                                    503, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpDeviceSaveDataJournalSize,                             504, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpBcatDeliveryCacheStorageSize,                          505, AcpBcatSettingsInfo,                 FieldType_NumericI64) \
    HANDLER(AcpApplicationErrorCodeCategory,                          506, AcpGeneralSettingsInfo,              FieldType_String    ) \
    HANDLER(AcpLocalCommunicationId,                                  507, AcpGeneralSettingsInfo,              FieldType_U64Array  ) \
    HANDLER(AcpLogoType,                                              508, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpLogoHandling,                                          509, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpRuntimeAocInstall,                                     510, AcpAocSettingsInfo,                  FieldType_NumericU8 ) \
    HANDLER(AcpCrashReport,                                           511, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpHdcp,                                                  512, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpSeedForPseudoDeviceId,                                 513, AcpGeneralSettingsInfo,              FieldType_NumericU64) \
    HANDLER(AcpBcatPassphrase,                                        514, AcpBcatSettingsInfo,                 FieldType_String    ) \
    HANDLER(AcpUserAccountSaveDataSizeMax,                            515, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpUserAccountSaveDataJournalSizeMax,                     516, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpDeviceSaveDataSizeMax,                                 517, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpDeviceSaveDataJournalSizeMax,                          518, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpTemporaryStorageSize,                                  519, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpCacheStorageSize,                                      520, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpCacheStorageJournalSize,                               521, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpCacheStorageDataAndJournalSizeMax,                     522, AcpStorageSettingsInfo,              FieldType_NumericI64) \
    HANDLER(AcpCacheStorageIndexMax,                                  523, AcpStorageSettingsInfo,              FieldType_NumericI16) \
    HANDLER(AcpPlayLogQueryableApplicationId,                         524, AcpPlayLogSettingsInfo,              FieldType_U64Array  ) \
    HANDLER(AcpPlayLogQueryCapability,                                525, AcpPlayLogSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(AcpRepairFlag,                                            526, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(RunningApplicationPatchStorageLocation,                   527, RunningApplicationInfo,              FieldType_String    ) \
    HANDLER(RunningApplicationVersionNumber,                          528, RunningApplicationInfo,              FieldType_NumericU32) \
    HANDLER(FsRecoveredByInvalidateCacheCount,                        529, FsProxyErrorInfo,                    FieldType_NumericU32) \
    HANDLER(FsSaveDataIndexCount,                                     530, FsProxyErrorInfo,                    FieldType_NumericU32) \
    HANDLER(FsBufferManagerPeakTotalAllocatableSize,                  531, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(MonitorCurrentWidth,                                      532, MonitorSettings,                     FieldType_NumericU16) \
    HANDLER(MonitorCurrentHeight,                                     533, MonitorSettings,                     FieldType_NumericU16) \
    HANDLER(MonitorCurrentRefreshRate,                                534, MonitorSettings,                     FieldType_String    ) \
    HANDLER(RebootlessSystemUpdateVersion,                            535, RebootlessSystemUpdateVersionInfo,   FieldType_String    ) \
    HANDLER(EncryptedExceptionInfo1,                                  536, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(EncryptedExceptionInfo2,                                  537, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(EncryptedExceptionInfo3,                                  538, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(EncryptedDyingMessage,                                    539, ErrorInfo,                           FieldType_U8Array   ) \
    HANDLER(DramId,                                                   540, PowerClockInfo,                      FieldType_NumericU32) \
    HANDLER(NifmConnectionTestRedirectUrl,                            541, NifmConnectionTestInfo,              FieldType_String    ) \
    HANDLER(AcpRequiredNetworkServiceLicenseOnLaunchFlag,             542, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(PciePort0Flags,                                           543, PcieLoggedStateInfo,                 FieldType_NumericU32) \
    HANDLER(PciePort0Speed,                                           544, PcieLoggedStateInfo,                 FieldType_NumericU8 ) \
    HANDLER(PciePort0ResetTimeInUs,                                   545, PcieLoggedStateInfo,                 FieldType_NumericU32) \
    HANDLER(PciePort0IrqCount,                                        546, PcieLoggedStateInfo,                 FieldType_NumericU32) \
    HANDLER(PciePort0Statistics,                                      547, PcieLoggedStateInfo,                 FieldType_U32Array  ) \
    HANDLER(PciePort1Flags,                                           548, PcieLoggedStateInfo,                 FieldType_NumericU32) \
    HANDLER(PciePort1Speed,                                           549, PcieLoggedStateInfo,                 FieldType_NumericU8 ) \
    HANDLER(PciePort1ResetTimeInUs,                                   550, PcieLoggedStateInfo,                 FieldType_NumericU32) \
    HANDLER(PciePort1IrqCount,                                        551, PcieLoggedStateInfo,                 FieldType_NumericU32) \
    HANDLER(PciePort1Statistics,                                      552, PcieLoggedStateInfo,                 FieldType_U32Array  ) \
    HANDLER(PcieFunction0VendorId,                                    553, PcieLoggedStateInfo,                 FieldType_NumericU16) \
    HANDLER(PcieFunction0DeviceId,                                    554, PcieLoggedStateInfo,                 FieldType_NumericU16) \
    HANDLER(PcieFunction0PmState,                                     555, PcieLoggedStateInfo,                 FieldType_NumericU8 ) \
    HANDLER(PcieFunction0IsAcquired,                                  556, PcieLoggedStateInfo,                 FieldType_Bool      ) \
    HANDLER(PcieFunction1VendorId,                                    557, PcieLoggedStateInfo,                 FieldType_NumericU16) \
    HANDLER(PcieFunction1DeviceId,                                    558, PcieLoggedStateInfo,                 FieldType_NumericU16) \
    HANDLER(PcieFunction1PmState,                                     559, PcieLoggedStateInfo,                 FieldType_NumericU8 ) \
    HANDLER(PcieFunction1IsAcquired,                                  560, PcieLoggedStateInfo,                 FieldType_Bool      ) \
    HANDLER(PcieGlobalRootComplexStatistics,                          561, PcieLoggedStateInfo,                 FieldType_U32Array  ) \
    HANDLER(PciePllResistorCalibrationValue,                          562, PcieLoggedStateInfo,                 FieldType_NumericU8 ) \
    HANDLER(CertificateRequestedHostName,                             563, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(CertificateCommonName,                                    564, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(CertificateSANCount,                                      565, NetworkSecurityCertificateInfo,      FieldType_NumericU32) \
    HANDLER(CertificateSANs,                                          566, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(FsBufferPoolMaxAllocateSize,                              567, FsMemoryInfo,                        FieldType_NumericU64) \
    HANDLER(CertificateIssuerName,                                    568, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(ApplicationAliveTime,                                     569, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(ApplicationInFocusTime,                                   570, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(ApplicationOutOfFocusTime,                                571, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(ApplicationBackgroundFocusTime,                           572, ErrorInfoAuto,                       FieldType_NumericI64) \
    HANDLER(AcpUserAccountSwitchLock,                                 573, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(USB3HostAvailableFlag,                                    574, USB3AvailableInfo,                   FieldType_Bool      ) \
    HANDLER(USB3DeviceAvailableFlag,                                  575, USB3AvailableInfo,                   FieldType_Bool      ) \
    HANDLER(AcpNeighborDetectionClientConfigurationSendDataId,        576, AcpNeighborDetectionInfo,            FieldType_NumericU64) \
    HANDLER(AcpNeighborDetectionClientConfigurationReceivableDataIds, 577, AcpNeighborDetectionInfo,            FieldType_U64Array  ) \
    HANDLER(AcpStartupUserAccountOptionFlag,                          578, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(ServerErrorCode,                                          579, ErrorInfo,                           FieldType_String    ) \
    HANDLER(AppletManagerMetaLogTrace,                                580, ErrorInfo,                           FieldType_U64Array  ) \
    HANDLER(ServerCertificateSerialNumber,                            581, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(ServerCertificatePublicKeyAlgorithm,                      582, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(ServerCertificateSignatureAlgorithm,                      583, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(ServerCertificateNotBefore,                               584, NetworkSecurityCertificateInfo,      FieldType_NumericU64) \
    HANDLER(ServerCertificateNotAfter,                                585, NetworkSecurityCertificateInfo,      FieldType_NumericU64) \
    HANDLER(CertificateAlgorithmInfoBits,                             586, NetworkSecurityCertificateInfo,      FieldType_NumericU64) \
    HANDLER(TlsConnectionPeerIpAddress,                               587, NetworkSecurityCertificateInfo,      FieldType_String    ) \
    HANDLER(TlsConnectionLastHandshakeState,                          588, NetworkSecurityCertificateInfo,      FieldType_NumericU32) \
    HANDLER(TlsConnectionInfoBits,                                    589, NetworkSecurityCertificateInfo,      FieldType_NumericU64) \
    HANDLER(SslStateBits,                                             590, NetworkSecurityCertificateInfo,      FieldType_NumericU64) \
    HANDLER(SslProcessInfoBits,                                       591, NetworkSecurityCertificateInfo,      FieldType_NumericU64) \
    HANDLER(SslProcessHeapSize,                                       592, NetworkSecurityCertificateInfo,      FieldType_NumericU32) \
    HANDLER(SslBaseErrorCode,                                         593, NetworkSecurityCertificateInfo,      FieldType_NumericI32) \
    HANDLER(GpuCrashDumpSize,                                         594, GpuCrashInfo,                        FieldType_NumericU32) \
    HANDLER(GpuCrashDump,                                             595, GpuCrashInfo,                        FieldType_U8Array   ) \
    HANDLER(RunningApplicationProgramIndex,                           596, RunningApplicationInfo,              FieldType_NumericU8 ) \
    HANDLER(UsbTopology,                                              597, UsbStateInfo,                        FieldType_U8Array   ) \
    HANDLER(AkamaiReferenceId,                                        598, ErrorInfo,                           FieldType_String    ) \
    HANDLER(NvHostErrID,                                              599, NvHostErrInfo,                       FieldType_NumericU32) \
    HANDLER(NvHostErrDataArrayU32,                                    600, NvHostErrInfo,                       FieldType_U32Array  ) \
    HANDLER(HasSyslogFlag,                                            601, ErrorInfo,                           FieldType_Bool      ) \
    HANDLER(AcpRuntimeParameterDelivery,                              602, AcpGeneralSettingsInfo,              FieldType_NumericU8 ) \
    HANDLER(PlatformRegion,                                           603, RegionSettingInfo,                   FieldType_String    ) \
    HANDLER(RunningUlaApplicationId,                                  604, RunningUlaInfo,                      FieldType_String    ) \
    HANDLER(RunningUlaAppletId,                                       605, RunningUlaInfo,                      FieldType_NumericU8 ) \
    HANDLER(RunningUlaVersion,                                        606, RunningUlaInfo,                      FieldType_NumericU32) \
    HANDLER(RunningUlaApplicationStorageLocation,                     607, RunningUlaInfo,                      FieldType_String    ) \
    HANDLER(RunningUlaPatchStorageLocation,                           608, RunningUlaInfo,                      FieldType_String    ) \
    HANDLER(NANDTotalSizeOfSystem,                                    609, NANDFreeSpaceInfo,                   FieldType_NumericU64) \
    HANDLER(NANDFreeSpaceOfSystem,                                    610, NANDFreeSpaceInfo,                   FieldType_NumericU64) \

