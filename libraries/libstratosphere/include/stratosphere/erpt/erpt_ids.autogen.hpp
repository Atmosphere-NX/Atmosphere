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
    HANDLER(ErrorInfoDefaults,                   61 ) \
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
    HANDLER(CompositorInfo,                      72 ) \
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
    HANDLER(InternalPanelInfo,                   126) \
    HANDLER(ResourceLimitLimitInfo,              127) \
    HANDLER(ResourceLimitPeakInfo,               128) \
    HANDLER(TouchScreenInfo,                     129) \
    HANDLER(AcpUserAccountSettingsInfo,          130) \
    HANDLER(AudioDeviceInfo,                     131) \
    HANDLER(AbnormalWakeInfo,                    132) \
    HANDLER(ServiceProfileInfo,                  133) \
    HANDLER(BluetoothAudioInfo,                  134) \
    HANDLER(BluetoothPairingCountInfo,           135) \
    HANDLER(FsProxyErrorInfo2,                   136) \
    HANDLER(BuiltInWirelessOUIInfo,              137) \
    HANDLER(WirelessAPOUIInfo,                   138) \
    HANDLER(EthernetAdapterOUIInfo,              139) \
    HANDLER(NANDTypeInfo,                        140) \
    HANDLER(MicroSDTypeInfo,                     141) \

#define AMS_ERPT_FOREACH_FIELD(HANDLER) \
    HANDLER(TestU64,                                               0,   Test,                              FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(TestU32,                                               1,   Test,                              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(TestI64,                                               2,   Test,                              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(TestI32,                                               3,   Test,                              FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(TestString,                                            4,   Test,                              FieldType_String,     FieldFlag_None   ) \
    HANDLER(TestU8Array,                                           5,   Test,                              FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(TestU32Array,                                          6,   Test,                              FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(TestU64Array,                                          7,   Test,                              FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(TestI32Array,                                          8,   Test,                              FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(TestI64Array,                                          9,   Test,                              FieldType_I64Array,   FieldFlag_None   ) \
    HANDLER(ErrorCode,                                             10,  ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(ErrorDescription,                                      11,  ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(OccurrenceTimestamp,                                   12,  ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ReportIdentifier,                                      13,  ErrorInfoAuto,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(ConnectionStatus,                                      14,  ConnectionStatusInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(AccessPointSSID,                                       15,  AccessPointInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(AccessPointSecurityType,                               16,  AccessPointInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(RadioStrength,                                         17,  RadioStrengthInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(IPAddressAcquisitionMethod,                            18,  NetworkInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SubnetMask,                                            19,  NetworkInfo,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(GatewayIPAddress,                                      20,  NetworkInfo,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(DNSType,                                               21,  NetworkInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PriorityDNSIPAddress,                                  22,  NetworkInfo,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(AlternateDNSIPAddress,                                 23,  NetworkInfo,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(UseProxyFlag,                                          24,  NetworkInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ProxyIPAddress,                                        25,  NetworkInfo,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(ProxyPort,                                             26,  NetworkInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ProxyAutoAuthenticateFlag,                             27,  NetworkInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(MTU,                                                   28,  NetworkInfo,                       FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ConnectAutomaticallyFlag,                              29,  NetworkInfo,                       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(UseStealthNetworkFlag,                                 30,  StealthNetworkInfo,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LimitHighCapacityFlag,                                 31,  LimitHighCapacityInfo,             FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NATType,                                               32,  NATTypeInfo,                       FieldType_String,     FieldFlag_None   ) \
    HANDLER(EnableWirelessInterfaceFlag,                           33,  EnableWirelessInterfaceInfo,       FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(EnableWifiFlag,                                        34,  EnableWifiInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(EnableBluetoothFlag,                                   35,  EnableBluetoothInfo,               FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(EnableNFCFlag,                                         36,  EnableNFCInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NintendoZoneSSIDListVersion,                           37,  NintendoZoneSSIDListVersionInfo,   FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationID,                                         38,  ApplicationInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationTitle,                                      39,  ApplicationInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationVersion,                                    40,  ApplicationInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationStorageLocation,                            41,  ApplicationInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(ProductModel,                                          42,  ProductModelInfo,                  FieldType_String,     FieldFlag_None   ) \
    HANDLER(CurrentLanguage,                                       43,  CurrentLanguageInfo,               FieldType_String,     FieldFlag_None   ) \
    HANDLER(UseNetworkTimeProtocolFlag,                            44,  UseNetworkTimeProtocolInfo,        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TimeZone,                                              45,  TimeZoneInfo,                      FieldType_String,     FieldFlag_None   ) \
    HANDLER(VideoOutputSetting,                                    46,  VideoOutputInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(NANDFreeSpace,                                         47,  NANDFreeSpaceInfo,                 FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SDCardFreeSpace,                                       48,  SDCardFreeSpaceInfo,               FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SerialNumber,                                          49,  ErrorInfoAuto,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(OsVersion,                                             50,  ErrorInfoAuto,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(ScreenBrightnessAutoAdjustFlag,                        51,  ScreenBrightnessInfo,              FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(HdmiAudioOutputMode,                                   52,  AudioFormatInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(SpeakerAudioOutputMode,                                53,  AudioFormatInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(MuteOnHeadsetUnpluggedFlag,                            54,  MuteOnHeadsetUnpluggedInfo,        FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ControllerVibrationVolume,                             55,  ControllerVibrationInfo,           FieldType_String,     FieldFlag_None   ) \
    HANDLER(LockScreenFlag,                                        56,  LockScreenInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(InternalBatteryLotNumber,                              57,  InternalBatteryLotNumberInfo,      FieldType_String,     FieldFlag_None   ) \
    HANDLER(NotifyInGameDownloadCompletionFlag,                    58,  NotificationInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NotificationSoundFlag,                                 59,  NotificationInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TVResolutionSetting,                                   60,  TVInfo,                            FieldType_String,     FieldFlag_None   ) \
    HANDLER(RGBRangeSetting,                                       61,  TVInfo,                            FieldType_String,     FieldFlag_None   ) \
    HANDLER(ReduceScreenBurnFlag,                                  62,  TVInfo,                            FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TVAllowsCecFlag,                                       63,  TVInfo,                            FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(HandheldModeTimeToScreenSleep,                         64,  SleepInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(ConsoleModeTimeToScreenSleep,                          65,  SleepInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(StopAutoSleepDuringContentPlayFlag,                    66,  SleepInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LastConnectionTestDownloadSpeed,                       67,  ConnectionInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(LastConnectionTestUploadSpeed,                         68,  ConnectionInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardCID,                                           69,  GameCardCIDInfo,                   FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NANDCID,                                               70,  NANDCIDInfo,                       FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(MicroSDCID,                                            71,  MicroSDCIDInfo,                    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NANDSpeedMode,                                         72,  NANDSpeedModeInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(MicroSDSpeedMode,                                      73,  MicroSDSpeedModeInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(USB3AvailableFlag,                                     74,  USB3AvailableInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(RegionSetting,                                         75,  RegionSettingInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(NintendoZoneConnectedFlag,                             76,  NintendoZoneConnectedInfo,         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ScreenBrightnessLevel,                                 77,  ScreenBrightnessInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(ProgramId,                                             78,  ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(AbortFlag,                                             79,  ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ReportVisibilityFlag,                                  80,  ErrorInfoAuto,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(FatalFlag,                                             81,  ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(OccurrenceTimestampNet,                                82,  ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ResultBacktrace,                                       83,  ErrorInfo,                         FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(GeneralRegisterAarch64,                                84,  ErrorInfo,                         FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(StackBacktrace64,                                      85,  ErrorInfo,                         FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(RegisterSetFlag64,                                     86,  ErrorInfo,                         FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ProgramMappedAddr64,                                   87,  ErrorInfo,                         FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AbortType,                                             88,  ErrorInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PrivateOsVersion,                                      89,  ErrorInfoAuto,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(CurrentSystemPowerState,                               90,  SystemPowerStateInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PreviousSystemPowerState,                              91,  SystemPowerStateInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(DestinationSystemPowerState,                           92,  SystemPowerStateInfo,              FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PscTransitionCurrentState,                             93,  ErrorInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PscTransitionPreviousState,                            94,  ErrorInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PscInitializedList,                                    95,  ErrorInfo,                         FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(PscCurrentPmStateList,                                 96,  ErrorInfo,                         FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PscNextPmStateList,                                    97,  ErrorInfo,                         FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PerformanceMode,                                       98,  PerformanceInfo,                   FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PerformanceConfiguration,                              99,  PerformanceInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(Throttled,                                             100, ThrottlingInfo,                    FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ThrottlingDuration,                                    101, ThrottlingInfo,                    FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ThrottlingTimestamp,                                   102, ThrottlingInfo,                    FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(GameCardCrcErrorCount,                                 103, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardAsicCrcErrorCount,                             104, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardRefreshCount,                                  105, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardReadRetryCount,                                106, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(EdidBlock,                                             107, EdidInfo,                          FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EdidExtensionBlock,                                    108, EdidInfo,                          FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(CreateProcessFailureFlag,                              109, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(TemperaturePcb,                                        110, ThermalInfo,                       FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(TemperatureSoc,                                        111, ThermalInfo,                       FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(CurrentFanDuty,                                        112, ThermalInfo,                       FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(LastDvfsThresholdTripped,                              113, ThermalInfo,                       FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(CradlePdcHFwVersion,                                   114, CradleFirmwareInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CradlePdcAFwVersion,                                   115, CradleFirmwareInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CradleMcuFwVersion,                                    116, CradleFirmwareInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CradleDp2HdmiFwVersion,                                117, CradleFirmwareInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(RunningApplicationId,                                  118, RunningApplicationInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationTitle,                               119, RunningApplicationInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationVersion,                             120, RunningApplicationInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationStorageLocation,                     121, RunningApplicationInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningAppletList,                                     122, RunningAppletInfo,                 FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(FocusedAppletHistory,                                  123, FocusedAppletHistoryInfo,          FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(CompositorState,                                       124, CompositorStateInfo,               FieldType_String,     FieldFlag_None   ) \
    HANDLER(CompositorLayerState,                                  125, CompositorLayerInfo,               FieldType_String,     FieldFlag_None   ) \
    HANDLER(CompositorDisplayState,                                126, CompositorDisplayInfo,             FieldType_String,     FieldFlag_None   ) \
    HANDLER(CompositorHWCState,                                    127, CompositorHWCInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(InputCurrentLimit,                                     128, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BoostModeCurrentLimit,                                 129, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FastChargeCurrentLimit,                                130, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ChargeVoltageLimit,                                    131, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ChargeConfiguration,                                   132, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(HizMode,                                               133, BatteryChargeInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ChargeEnabled,                                         134, BatteryChargeInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PowerSupplyPath,                                       135, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryTemperature,                                    136, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryChargePercent,                                  137, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryChargeVoltage,                                  138, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(BatteryAge,                                            139, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerRole,                                             140, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerSupplyType,                                       141, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerSupplyVoltage,                                    142, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PowerSupplyCurrent,                                    143, BatteryChargeInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FastBatteryChargingEnabled,                            144, BatteryChargeInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ControllerPowerSupplyAcquired,                         145, BatteryChargeInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(OtgRequested,                                          146, BatteryChargeInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NANDPreEolInfo,                                        147, NANDExtendedCsd,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDDeviceLifeTimeEstTypA,                             148, NANDExtendedCsd,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDDeviceLifeTimeEstTypB,                             149, NANDExtendedCsd,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDPatrolCount,                                       150, NANDPatrolInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumActivationFailures,                             151, NANDErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumActivationErrorCorrections,                     152, NANDErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumReadWriteFailures,                              153, NANDErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDNumReadWriteErrorCorrections,                      154, NANDErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NANDErrorLog,                                          155, NANDDriverLog,                     FieldType_String,     FieldFlag_None   ) \
    HANDLER(SdCardUserAreaSize,                                    156, SdCardSizeSpec,                    FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SdCardProtectedAreaSize,                               157, SdCardSizeSpec,                    FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SdCardNumActivationFailures,                           158, SdCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardNumActivationErrorCorrections,                   159, SdCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardNumReadWriteFailures,                            160, SdCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardNumReadWriteErrorCorrections,                    161, SdCardErrorInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardErrorLog,                                        162, SdCardDriverLog ,                  FieldType_String,     FieldFlag_None   ) \
    HANDLER(EncryptionKey,                                         163, ErrorInfo,                         FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardTimeoutRetryErrorCount,                        164, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsRemountForDataCorruptCount,                          165, FsProxyErrorInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsRemountForDataCorruptRetryOutCount,                  166, FsProxyErrorInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardInsertionCount,                                167, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardRemovalCount,                                  168, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardAsicInitializeCount,                           169, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(TestU16,                                               170, Test,                              FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(TestU8,                                                171, Test,                              FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(TestI16,                                               172, Test,                              FieldType_NumericI16, FieldFlag_None   ) \
    HANDLER(TestI8,                                                173, Test,                              FieldType_NumericI8,  FieldFlag_None   ) \
    HANDLER(SystemAppletScene,                                     174, SystemAppletSceneInfo,             FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(CodecType,                                             175, VideoInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(DecodeBuffers,                                         176, VideoInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FrameWidth,                                            177, VideoInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FrameHeight,                                           178, VideoInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ColorPrimaries,                                        179, VideoInfo,                         FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(TransferCharacteristics,                               180, VideoInfo,                         FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(MatrixCoefficients,                                    181, VideoInfo,                         FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(DisplayWidth,                                          182, VideoInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(DisplayHeight,                                         183, VideoInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(DARWidth,                                              184, VideoInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(DARHeight,                                             185, VideoInfo,                         FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(ColorFormat,                                           186, VideoInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(ColorSpace,                                            187, VideoInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(SurfaceLayout,                                         188, VideoInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(BitStream,                                             189, VideoInfo,                         FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(VideoDecState,                                         190, VideoInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(GpuErrorChannelId,                                     191, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorAruId,                                         192, GpuErrorInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(GpuErrorType,                                          193, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorFaultInfo,                                     194, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorWriteAccess,                                   195, GpuErrorInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(GpuErrorFaultAddress,                                  196, GpuErrorInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(GpuErrorFaultUnit,                                     197, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorFaultType,                                     198, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorHwContextPointer,                              199, GpuErrorInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(GpuErrorContextStatus,                                 200, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaIntr,                                     201, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaErrorType,                                202, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaHeaderShadow,                             203, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaHeader,                                   204, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaGpShadow0,                                205, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuErrorPbdmaGpShadow1,                                206, GpuErrorInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AccessPointChannel,                                    207, AccessPointInfo,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(ThreadName,                                            208, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(AdspExceptionRegisters,                                209, AdspErrorInfo,                     FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionSpsr,                                     210, AdspErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionProgramCounter,                           211, AdspErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionLinkRegister,                             212, AdspErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionStackPointer,                             213, AdspErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionArmModeRegisters,                         214, AdspErrorInfo,                     FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionStackAddress,                             215, AdspErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AdspExceptionStackDump,                                216, AdspErrorInfo,                     FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(AdspExceptionReason,                                   217, AdspErrorInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(OscillatorClock,                                       218, PowerClockInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CpuDvfsTableClocks,                                    219, PowerClockInfo,                    FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(CpuDvfsTableVoltages,                                  220, PowerClockInfo,                    FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(GpuDvfsTableClocks,                                    221, PowerClockInfo,                    FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(GpuDvfsTableVoltages,                                  222, PowerClockInfo,                    FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(EmcDvfsTableClocks,                                    223, PowerClockInfo,                    FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(EmcDvfsTableVoltages,                                  224, PowerClockInfo,                    FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(ModuleClockFrequencies,                                225, PowerClockInfo,                    FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(ModuleClockEnableFlags,                                226, PowerClockInfo,                    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ModulePowerEnableFlags,                                227, PowerClockInfo,                    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ModuleResetAssertFlags,                                228, PowerClockInfo,                    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ModuleMinimumVoltageClockRates,                        229, PowerClockInfo,                    FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PowerDomainEnableFlags,                                230, PowerClockInfo,                    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(PowerDomainVoltages,                                   231, PowerClockInfo,                    FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(AccessPointRssi,                                       232, RadioStrengthInfo,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FuseInfo,                                              233, PowerClockInfo,                    FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(VideoLog,                                              234, VideoInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(GameCardDeviceId,                                      235, GameCardCIDInfo,                   FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardAsicReinitializeCount,                         236, GameCardErrorInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardAsicReinitializeFailureCount,                  237, GameCardErrorInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardAsicReinitializeFailureDetail,                 238, GameCardErrorInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardRefreshSuccessCount,                           239, GameCardErrorInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardAwakenCount,                                   240, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardAwakenFailureCount,                            241, GameCardErrorInfo,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(GameCardReadCountFromInsert,                           242, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardReadCountFromAwaken,                           243, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLastReadErrorPageAddress,                      244, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLastReadErrorPageCount,                        245, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AppletManagerContextTrace,                             246, ErrorInfo,                         FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(NvDispIsRegistered,                                    247, NvDispDeviceInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispIsSuspend,                                       248, NvDispDeviceInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDC0SurfaceNum,                                   249, NvDispDeviceInfo,                  FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(NvDispDC1SurfaceNum,                                   250, NvDispDeviceInfo,                  FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectX,                                  251, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectY,                                  252, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectWidth,                              253, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSrcRectHeight,                             254, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectX,                                  255, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectY,                                  256, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectWidth,                              257, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDstRectHeight,                             258, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowIndex,                                     259, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowBlendOperation,                            260, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowAlphaOperation,                            261, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowDepth,                                     262, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowAlpha,                                     263, NvDispDcWindowInfo,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispWindowHFilter,                                   264, NvDispDcWindowInfo,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispWindowVFilter,                                   265, NvDispDcWindowInfo,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispWindowOptions,                                   266, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispWindowSyncPointId,                               267, NvDispDcWindowInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPSorPower,                                      268, NvDispDpModeInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPClkType,                                       269, NvDispDpModeInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPEnable,                                        270, NvDispDpModeInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPState,                                         271, NvDispDpModeInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPEdid,                                          272, NvDispDpModeInfo,                  FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispDPEdidSize,                                      273, NvDispDpModeInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPEdidExtSize,                                   274, NvDispDpModeInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPFakeMode,                                      275, NvDispDpModeInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPModeNumber,                                    276, NvDispDpModeInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPPlugInOut,                                     277, NvDispDpModeInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPAuxIntHandler,                                 278, NvDispDpModeInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPForceMaxLinkBW,                                279, NvDispDpModeInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPIsConnected,                                   280, NvDispDpModeInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkValid,                                     281, NvDispDpLinkSpec,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkMaxBW,                                     282, NvDispDpLinkSpec,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkMaxLaneCount,                              283, NvDispDpLinkSpec,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkDownSpread,                                284, NvDispDpLinkSpec,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkSupportEnhancedFraming,                    285, NvDispDpLinkSpec,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkBpp,                                       286, NvDispDpLinkSpec,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkScaramberCap,                              287, NvDispDpLinkSpec,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkBW,                                        288, NvDispDpLinkStatus,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkLaneCount,                                 289, NvDispDpLinkStatus,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDPLinkEnhancedFraming,                           290, NvDispDpLinkStatus,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkScrambleEnable,                            291, NvDispDpLinkStatus,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkActivePolarity,                            292, NvDispDpLinkStatus,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkActiveCount,                               293, NvDispDpLinkStatus,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkTUSize,                                    294, NvDispDpLinkStatus,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkActiveFrac,                                295, NvDispDpLinkStatus,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkWatermark,                                 296, NvDispDpLinkStatus,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkHBlank,                                    297, NvDispDpLinkStatus,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkVBlank,                                    298, NvDispDpLinkStatus,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDPLinkOnlyEnhancedFraming,                       299, NvDispDpLinkStatus,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkOnlyEdpCap,                                300, NvDispDpLinkStatus,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkSupportFastLT,                             301, NvDispDpLinkStatus,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkLTDataValid,                               302, NvDispDpLinkStatus,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkTsp3Support,                               303, NvDispDpLinkStatus,                FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDPLinkAuxInterval,                               304, NvDispDpLinkStatus,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpCreated,                                     305, NvDispDpHdcpInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispHdcpUserRequest,                                 306, NvDispDpHdcpInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispHdcpPlugged,                                     307, NvDispDpHdcpInfo,                  FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispHdcpState,                                       308, NvDispDpHdcpInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispHdcpFailCount,                                   309, NvDispDpHdcpInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispHdcpHdcp22,                                      310, NvDispDpHdcpInfo,                  FieldType_NumericI8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpMaxRetry,                                    311, NvDispDpHdcpInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpHpd,                                         312, NvDispDpHdcpInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispHdcpRepeater,                                    313, NvDispDpHdcpInfo,                  FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispCecRxBuf,                                        314, NvDispDpAuxCecInfo,                FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispCecRxLength,                                     315, NvDispDpAuxCecInfo,                FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispCecTxBuf,                                        316, NvDispDpAuxCecInfo,                FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispCecTxLength,                                     317, NvDispDpAuxCecInfo,                FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispCecTxRet,                                        318, NvDispDpAuxCecInfo,                FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispCecState,                                        319, NvDispDpAuxCecInfo,                FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispCecTxInfo,                                       320, NvDispDpAuxCecInfo,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispCecRxInfo,                                       321, NvDispDpAuxCecInfo,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDCIndex,                                         322, NvDispDcInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCInitialize,                                    323, NvDispDcInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCClock,                                         324, NvDispDcInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCFrequency,                                     325, NvDispDcInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCFailed,                                        326, NvDispDcInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCModeWidth,                                     327, NvDispDcInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispDCModeHeight,                                    328, NvDispDcInfo,                      FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(NvDispDCModeBpp,                                       329, NvDispDcInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCPanelFrequency,                                330, NvDispDcInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCWinDirty,                                      331, NvDispDcInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCWinEnable,                                     332, NvDispDcInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDCVrr,                                           333, NvDispDcInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDCPanelInitialize,                               334, NvDispDcInfo,                      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDsiDataFormat,                                   335, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiVideoMode,                                    336, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiRefreshRate,                                  337, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiLpCmdModeFrequency,                           338, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiHsCmdModeFrequency,                           339, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiPanelResetTimeout,                            340, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiPhyFrequency,                                 341, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiFrequency,                                    342, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDsiInstance,                                     343, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHostCtrlEnable,                             344, NvDispDsiInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiInit,                                       345, NvDispDsiInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiEnable,                                     346, NvDispDsiInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHsMode,                                     347, NvDispDsiInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiVendorId,                                   348, NvDispDsiInfo,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NvDispDcDsiLcdVendorNum,                               349, NvDispDsiInfo,                     FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHsClockControl,                             350, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDcDsiEnableHsClockInLpMode,                      351, NvDispDsiInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiTeFrameUpdate,                              352, NvDispDsiInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispDcDsiGangedType,                                 353, NvDispDsiInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispDcDsiHbpInPktSeq,                                354, NvDispDsiInfo,                     FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NvDispErrID,                                           355, NvDispErrIDInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispErrData0,                                        356, NvDispErrIDInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvDispErrData1,                                        357, NvDispErrIDInfo,                   FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SdCardMountStatus,                                     358, SdCardMountInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(SdCardMountUnexpectedResult,                           359, SdCardMountInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(NANDTotalSize,                                         360, NANDFreeSpaceInfo,                 FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SdCardTotalSize,                                       361, SDCardFreeSpaceInfo,               FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ElapsedTimeSinceInitialLaunch,                         362, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ElapsedTimeSincePowerOn,                               363, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(ElapsedTimeSinceLastAwake,                             364, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(OccurrenceTick,                                        365, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(RetailInteractiveDisplayFlag,                          366, RetailInteractiveDisplayInfo,      FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(FatFsError,                                            367, FsProxyErrorInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsExtraError,                                       368, FsProxyErrorInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsErrorDrive,                                       369, FsProxyErrorInfo,                  FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsErrorName,                                        370, FsProxyErrorInfo,                  FieldType_String,     FieldFlag_None   ) \
    HANDLER(MonitorManufactureCode,                                371, MonitorCapability,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(MonitorProductCode,                                    372, MonitorCapability,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(MonitorSerialNumber,                                   373, MonitorCapability,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(MonitorManufactureYear,                                374, MonitorCapability,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(PhysicalAddress,                                       375, MonitorCapability,                 FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(Is4k60Hz,                                              376, MonitorCapability,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(Is4k30Hz,                                              377, MonitorCapability,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(Is1080P60Hz,                                           378, MonitorCapability,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(Is720P60Hz,                                            379, MonitorCapability,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PcmChannelMax,                                         380, MonitorCapability,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(CrashReportHash,                                       381, ErrorInfo,                         FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ErrorReportSharePermission,                            382, ErrorReportSharePermissionInfo,    FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(VideoCodecTypeEnum,                                    383, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoBitRate,                                          384, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoFrameRate,                                        385, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoWidth,                                            386, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(VideoHeight,                                           387, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioCodecTypeEnum,                                    388, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioSampleRate,                                       389, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioChannelCount,                                     390, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(AudioBitRate,                                          391, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaContainerType,                               392, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaProfileType,                                 393, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaLevelType,                                   394, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaCacheSizeEnum,                               395, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaErrorStatusEnum,                             396, MultimediaInfo,                    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(MultimediaErrorLog,                                    397, MultimediaInfo,                    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ServerFqdn,                                            398, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerIpAddress,                                       399, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(TestStringEncrypt,                                     400, Test,                              FieldType_String,     FieldFlag_Encrypt) \
    HANDLER(TestU8ArrayEncrypt,                                    401, Test,                              FieldType_U8Array,    FieldFlag_Encrypt) \
    HANDLER(TestU32ArrayEncrypt,                                   402, Test,                              FieldType_U32Array,   FieldFlag_Encrypt) \
    HANDLER(TestU64ArrayEncrypt,                                   403, Test,                              FieldType_U64Array,   FieldFlag_Encrypt) \
    HANDLER(TestI32ArrayEncrypt,                                   404, Test,                              FieldType_I32Array,   FieldFlag_Encrypt) \
    HANDLER(TestI64ArrayEncrypt,                                   405, Test,                              FieldType_I64Array,   FieldFlag_Encrypt) \
    HANDLER(CipherKey,                                             406, ErrorInfoAuto,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(FileSystemPath,                                        407, ErrorInfo,                         FieldType_String,     FieldFlag_Encrypt) \
    HANDLER(WebMediaPlayerOpenUrl,                                 408, ErrorInfo,                         FieldType_String,     FieldFlag_Encrypt) \
    HANDLER(WebMediaPlayerLastSocketErrors,                        409, ErrorInfo,                         FieldType_I32Array,   FieldFlag_None   ) \
    HANDLER(UnknownControllerCount,                                410, ConnectedControllerInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AttachedControllerCount,                               411, ConnectedControllerInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothControllerCount,                              412, ConnectedControllerInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(UsbControllerCount,                                    413, ConnectedControllerInfo,           FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ControllerTypeList,                                    414, ConnectedControllerInfo,           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ControllerInterfaceList,                               415, ConnectedControllerInfo,           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ControllerStyleList,                                   416, ConnectedControllerInfo,           FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(FsPooledBufferPeakFreeSize,                            417, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPooledBufferRetriedCount,                            418, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPooledBufferReduceAllocationCount,                   419, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsBufferManagerPeakFreeSize,                           420, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsBufferManagerRetriedCount,                           421, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsExpHeapPeakFreeSize,                                 422, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsBufferPoolPeakFreeSize,                              423, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPatrolReadAllocateBufferSuccessCount,                424, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(FsPatrolReadAllocateBufferFailureCount,                425, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SteadyClockInternalOffset,                             426, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SteadyClockCurrentTimePointValue,                      427, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(UserClockContextOffset,                                428, UserClockContextInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(UserClockContextTimeStampValue,                        429, UserClockContextInfo,              FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(NetworkClockContextOffset,                             430, NetworkClockContextInfo,           FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(NetworkClockContextTimeStampValue,                     431, NetworkClockContextInfo,           FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemAbortFlag,                                       432, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(ApplicationAbortFlag,                                  433, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(NifmErrorCode,                                         434, ConnectionStatusInfo,              FieldType_String,     FieldFlag_None   ) \
    HANDLER(LcsApplicationId,                                      435, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyIdList,                               436, ErrorInfo,                         FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyVersionList,                          437, ErrorInfo,                         FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyTypeList,                             438, ErrorInfo,                         FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(LcsContentMetaKeyInstallTypeList,                      439, ErrorInfo,                         FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(LcsSenderFlag,                                         440, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LcsApplicationRequestFlag,                             441, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LcsHasExFatDriverFlag,                                 442, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(LcsIpAddress,                                          443, ErrorInfo,                         FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpStartupUserAccount,                                 444, AcpUserAccountSettingsInfo,        FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpAocRegistrationType,                                445, AcpAocSettingsInfo,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpAttributeFlag,                                      446, AcpGeneralSettingsInfo,            FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpSupportedLanguageFlag,                              447, AcpGeneralSettingsInfo,            FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpParentalControlFlag,                                448, AcpGeneralSettingsInfo,            FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(AcpScreenShot,                                         449, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpVideoCapture,                                       450, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpDataLossConfirmation,                               451, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpPlayLogPolicy,                                      452, AcpPlayLogSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpPresenceGroupId,                                    453, AcpGeneralSettingsInfo,            FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpRatingAge,                                          454, AcpRatingSettingsInfo,             FieldType_I8Array,    FieldFlag_None   ) \
    HANDLER(AcpAocBaseId,                                          455, AcpAocSettingsInfo,                FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataSize,                            456, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataJournalSize,                     457, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataSize,                                 458, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataJournalSize,                          459, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpBcatDeliveryCacheStorageSize,                       460, AcpBcatSettingsInfo,               FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpApplicationErrorCodeCategory,                       461, AcpGeneralSettingsInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(AcpLocalCommunicationId,                               462, AcpGeneralSettingsInfo,            FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(AcpLogoType,                                           463, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpLogoHandling,                                       464, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpRuntimeAocInstall,                                  465, AcpAocSettingsInfo,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpCrashReport,                                        466, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpHdcp,                                               467, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpSeedForPseudoDeviceId,                              468, AcpGeneralSettingsInfo,            FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataSizeMax,                         469, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSaveDataJournalSizeMax,                  470, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataSizeMax,                              471, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpDeviceSaveDataJournalSizeMax,                       472, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpTemporaryStorageSize,                               473, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageSize,                                   474, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageJournalSize,                            475, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageDataAndJournalSizeMax,                  476, AcpStorageSettingsInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpCacheStorageIndexMax,                               477, AcpStorageSettingsInfo,            FieldType_NumericI16, FieldFlag_None   ) \
    HANDLER(AcpPlayLogQueryableApplicationId,                      478, AcpPlayLogSettingsInfo,            FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(AcpPlayLogQueryCapability,                             479, AcpPlayLogSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AcpRepairFlag,                                         480, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(RunningApplicationPatchStorageLocation,                481, RunningApplicationInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(RunningApplicationVersionNumber,                       482, RunningApplicationInfo,            FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsRecoveredByInvalidateCacheCount,                     483, FsProxyErrorInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsSaveDataIndexCount,                                  484, FsProxyErrorInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsBufferManagerPeakTotalAllocatableSize,               485, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(MonitorCurrentWidth,                                   486, MonitorSettings,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(MonitorCurrentHeight,                                  487, MonitorSettings,                   FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(MonitorCurrentRefreshRate,                             488, MonitorSettings,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(RebootlessSystemUpdateVersion,                         489, RebootlessSystemUpdateVersionInfo, FieldType_String,     FieldFlag_None   ) \
    HANDLER(EncryptedDyingMessage,                                 490, ErrorInfo,                         FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(DramId,                                                491, PowerClockInfo,                    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NifmConnectionTestRedirectUrl,                         492, NifmConnectionTestInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(AcpRequiredNetworkServiceLicenseOnLaunchFlag,          493, AcpUserAccountSettingsInfo,        FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PciePort0Flags,                                        494, PcieLoggedStateInfo,               FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort0Speed,                                        495, PcieLoggedStateInfo,               FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PciePort0ResetTimeInUs,                                496, PcieLoggedStateInfo,               FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort0IrqCount,                                     497, PcieLoggedStateInfo,               FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort0Statistics,                                   498, PcieLoggedStateInfo,               FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PciePort1Flags,                                        499, PcieLoggedStateInfo,               FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort1Speed,                                        500, PcieLoggedStateInfo,               FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PciePort1ResetTimeInUs,                                501, PcieLoggedStateInfo,               FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort1IrqCount,                                     502, PcieLoggedStateInfo,               FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PciePort1Statistics,                                   503, PcieLoggedStateInfo,               FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PcieFunction0VendorId,                                 504, PcieLoggedStateInfo,               FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction0DeviceId,                                 505, PcieLoggedStateInfo,               FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction0PmState,                                  506, PcieLoggedStateInfo,               FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PcieFunction0IsAcquired,                               507, PcieLoggedStateInfo,               FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PcieFunction1VendorId,                                 508, PcieLoggedStateInfo,               FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction1DeviceId,                                 509, PcieLoggedStateInfo,               FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(PcieFunction1PmState,                                  510, PcieLoggedStateInfo,               FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PcieFunction1IsAcquired,                               511, PcieLoggedStateInfo,               FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(PcieGlobalRootComplexStatistics,                       512, PcieLoggedStateInfo,               FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(PciePllResistorCalibrationValue,                       513, PcieLoggedStateInfo,               FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(CertificateRequestedHostName,                          514, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(CertificateCommonName,                                 515, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(CertificateSANCount,                                   516, NetworkSecurityCertificateInfo,    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(CertificateSANs,                                       517, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(FsBufferPoolMaxAllocateSize,                           518, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(CertificateIssuerName,                                 519, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(ApplicationAliveTime,                                  520, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(AcpUserAccountSwitchLock,                              521, AcpUserAccountSettingsInfo,        FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(USB3HostAvailableFlag,                                 522, USB3AvailableInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(USB3DeviceAvailableFlag,                               523, USB3AvailableInfo,                 FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(AcpStartupUserAccountOptionFlag,                       524, AcpUserAccountSettingsInfo,        FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ServerErrorCode,                                       525, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(AppletManagerMetaLogTrace,                             526, ErrorInfo,                         FieldType_U64Array,   FieldFlag_None   ) \
    HANDLER(ServerCertificateSerialNumber,                         527, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerCertificatePublicKeyAlgorithm,                   528, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerCertificateSignatureAlgorithm,                   529, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(ServerCertificateNotBefore,                            530, NetworkSecurityCertificateInfo,    FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ServerCertificateNotAfter,                             531, NetworkSecurityCertificateInfo,    FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(CertificateAlgorithmInfoBits,                          532, NetworkSecurityCertificateInfo,    FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(TlsConnectionPeerIpAddress,                            533, NetworkSecurityCertificateInfo,    FieldType_String,     FieldFlag_None   ) \
    HANDLER(TlsConnectionLastHandshakeState,                       534, NetworkSecurityCertificateInfo,    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(TlsConnectionInfoBits,                                 535, NetworkSecurityCertificateInfo,    FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SslStateBits,                                          536, NetworkSecurityCertificateInfo,    FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SslProcessInfoBits,                                    537, NetworkSecurityCertificateInfo,    FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SslProcessHeapSize,                                    538, NetworkSecurityCertificateInfo,    FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(SslBaseErrorCode,                                      539, NetworkSecurityCertificateInfo,    FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(GpuCrashDumpSize,                                      540, GpuCrashInfo,                      FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GpuCrashDump,                                          541, GpuCrashInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(RunningApplicationProgramIndex,                        542, RunningApplicationInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(UsbTopology,                                           543, UsbStateInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(AkamaiReferenceId,                                     544, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(NvHostErrID,                                           545, NvHostErrInfo,                     FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(NvHostErrDataArrayU32,                                 546, NvHostErrInfo,                     FieldType_U32Array,   FieldFlag_None   ) \
    HANDLER(HasSyslogFlag,                                         547, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(AcpRuntimeParameterDelivery,                           548, AcpGeneralSettingsInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PlatformRegion,                                        549, RegionSettingInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(NANDTotalSizeOfSystem,                                 550, NANDFreeSpaceInfo,                 FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(NANDFreeSpaceOfSystem,                                 551, NANDFreeSpaceInfo,                 FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AccessPointSSIDAsHex,                                  552, AccessPointInfo,                   FieldType_String,     FieldFlag_None   ) \
    HANDLER(PanelVendorId,                                         553, InternalPanelInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PanelRevisionId,                                       554, InternalPanelInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(PanelModelId,                                          555, InternalPanelInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ErrorContext,                                          556, ErrorInfoAuto,                     FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(ErrorContextSize,                                      557, ErrorInfoAuto,                     FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(ErrorContextTotalSize,                                 558, ErrorInfoAuto,                     FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(SystemPhysicalMemoryLimit,                             559, ResourceLimitLimitInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemThreadCountLimit,                                560, ResourceLimitLimitInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemEventCountLimit,                                 561, ResourceLimitLimitInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemTransferMemoryCountLimit,                        562, ResourceLimitLimitInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemSessionCountLimit,                               563, ResourceLimitLimitInfo,            FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemPhysicalMemoryPeak,                              564, ResourceLimitPeakInfo,             FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemThreadCountPeak,                                 565, ResourceLimitPeakInfo,             FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemEventCountPeak,                                  566, ResourceLimitPeakInfo,             FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemTransferMemoryCountPeak,                         567, ResourceLimitPeakInfo,             FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(SystemSessionCountPeak,                                568, ResourceLimitPeakInfo,             FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(GpuCrashHash,                                          569, GpuCrashInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(TouchScreenPanelGpioValue,                             570, TouchScreenInfo,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BrowserCertificateHostName,                            571, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(BrowserCertificateCommonName,                          572, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(BrowserCertificateOrganizationalUnitName,              573, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(FsPooledBufferFailedIdealAllocationCountOnAsyncAccess, 574, FsMemoryInfo,                      FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(AudioOutputTarget,                                     575, AudioDeviceInfo,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AudioOutputChannelCount,                               576, AudioDeviceInfo,                   FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(AppletTotalActiveTime,                                 577, ErrorInfoAuto,                     FieldType_NumericI64, FieldFlag_None   ) \
    HANDLER(WakeCount,                                             578, AbnormalWakeInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(PredominantWakeReason,                                 579, AbnormalWakeInfo,                  FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(EdidExtensionBlock2,                                   580, EdidInfo,                          FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(EdidExtensionBlock3,                                   581, EdidInfo,                          FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(LumenRequestId,                                        582, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(LlnwLlid,                                              583, ErrorInfo,                         FieldType_String,     FieldFlag_None   ) \
    HANDLER(SupportingLimitedApplicationLicenses,                  584, RunningApplicationInfo,            FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(RuntimeLimitedApplicationLicenseUpgrade,               585, RunningApplicationInfo,            FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(ServiceProfileRevisionKey,                             586, ServiceProfileInfo,                FieldType_NumericU64, FieldFlag_None   ) \
    HANDLER(BluetoothAudioConnectionCount,                         587, BluetoothAudioInfo,                FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothHidPairingInfoCount,                          588, BluetoothPairingCountInfo,         FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothAudioPairingInfoCount,                        589, BluetoothPairingCountInfo,         FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(BluetoothLePairingInfoCount,                           590, BluetoothPairingCountInfo,         FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFilePeakOpenCount,                       591, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisSystemDirectoryPeakOpenCount,                  592, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserFilePeakOpenCount,                         593, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserDirectoryPeakOpenCount,                    594, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardFilePeakOpenCount,                          595, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardDirectoryPeakOpenCount,                     596, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(SslAlertInfo,                                          597, NetworkSecurityCertificateInfo,    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(SslVersionInfo,                                        598, NetworkSecurityCertificateInfo,    FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(FatFsBisSystemUniqueFileEntryPeakOpenCount,            599, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisSystemUniqueDirectoryEntryPeakOpenCount,       600, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserUniqueFileEntryPeakOpenCount,              601, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsBisUserUniqueDirectoryEntryPeakOpenCount,         602, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardUniqueFileEntryPeakOpenCount,               603, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(FatFsSdCardUniqueDirectoryEntryPeakOpenCount,          604, FsProxyErrorInfo,                  FieldType_NumericU16, FieldFlag_None   ) \
    HANDLER(ServerErrorIsRetryable,                                605, ErrorInfo,                         FieldType_Bool,       FieldFlag_None   ) \
    HANDLER(FsDeepRetryStartCount,                                 606, FsProxyErrorInfo2,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(FsUnrecoverableByGameCardAccessFailedCount,            607, FsProxyErrorInfo2,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(BuiltInWirelessOUI,                                    608, BuiltInWirelessOUIInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(WirelessAPOUI,                                         609, WirelessAPOUIInfo,                 FieldType_String,     FieldFlag_None   ) \
    HANDLER(EthernetAdapterOUI,                                    610, EthernetAdapterOUIInfo,            FieldType_String,     FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFatSafeControlResult,                    611, FsProxyErrorInfo2,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFatErrorNumber,                          612, FsProxyErrorInfo2,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsBisSystemFatSafeErrorNumber,                      613, FsProxyErrorInfo2,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsBisUserFatSafeControlResult,                      614, FsProxyErrorInfo2,                 FieldType_NumericU8,  FieldFlag_None   ) \
    HANDLER(FatFsBisUserFatErrorNumber,                            615, FsProxyErrorInfo2,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(FatFsBisUserFatSafeErrorNumber,                        616, FsProxyErrorInfo2,                 FieldType_NumericI32, FieldFlag_None   ) \
    HANDLER(GpuCrashDump2,                                         617, GpuCrashInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(NANDType,                                              618, NANDTypeInfo,                      FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(MicroSDType,                                           619, MicroSDTypeInfo,                   FieldType_U8Array,    FieldFlag_None   ) \
    HANDLER(GameCardLastDeactivateReasonResult,                    620, GameCardErrorInfo,                 FieldType_NumericU32, FieldFlag_None   ) \
    HANDLER(GameCardLastDeactivateReason,                          621, GameCardErrorInfo,                 FieldType_NumericU8,  FieldFlag_None   ) \

