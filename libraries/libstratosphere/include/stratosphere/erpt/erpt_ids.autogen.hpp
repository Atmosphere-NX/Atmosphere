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
    HANDLER(TestU64,                                                  0,   0,   0 ) \
    HANDLER(TestU32,                                                  1,   0,   1 ) \
    HANDLER(TestI64,                                                  2,   0,   2 ) \
    HANDLER(TestI32,                                                  3,   0,   3 ) \
    HANDLER(TestString,                                               4,   0,   4 ) \
    HANDLER(TestU8Array,                                              5,   0,   5 ) \
    HANDLER(TestU32Array,                                             6,   0,   6 ) \
    HANDLER(TestU64Array,                                             7,   0,   7 ) \
    HANDLER(TestI32Array,                                             8,   0,   8 ) \
    HANDLER(TestI64Array,                                             9,   0,   9 ) \
    HANDLER(ErrorCode,                                                10,  1,   4 ) \
    HANDLER(ErrorDescription,                                         11,  1,   4 ) \
    HANDLER(OccurrenceTimestamp,                                      12,  59,  2 ) \
    HANDLER(ReportIdentifier,                                         13,  59,  4 ) \
    HANDLER(ConnectionStatus,                                         14,  2,   4 ) \
    HANDLER(AccessPointSSID,                                          15,  60,  4 ) \
    HANDLER(AccessPointSecurityType,                                  16,  60,  4 ) \
    HANDLER(RadioStrength,                                            17,  58,  1 ) \
    HANDLER(NXMacAddress,                                             18,  4,   4 ) \
    HANDLER(IPAddressAcquisitionMethod,                               19,  3,   1 ) \
    HANDLER(CurrentIPAddress,                                         20,  3,   4 ) \
    HANDLER(SubnetMask,                                               21,  3,   4 ) \
    HANDLER(GatewayIPAddress,                                         22,  3,   4 ) \
    HANDLER(DNSType,                                                  23,  3,   1 ) \
    HANDLER(PriorityDNSIPAddress,                                     24,  3,   4 ) \
    HANDLER(AlternateDNSIPAddress,                                    25,  3,   4 ) \
    HANDLER(UseProxyFlag,                                             26,  3,   10) \
    HANDLER(ProxyIPAddress,                                           27,  3,   4 ) \
    HANDLER(ProxyPort,                                                28,  3,   1 ) \
    HANDLER(ProxyAutoAuthenticateFlag,                                29,  3,   10) \
    HANDLER(MTU,                                                      30,  3,   1 ) \
    HANDLER(ConnectAutomaticallyFlag,                                 31,  3,   10) \
    HANDLER(UseStealthNetworkFlag,                                    32,  5,   10) \
    HANDLER(LimitHighCapacityFlag,                                    33,  6,   10) \
    HANDLER(NATType,                                                  34,  7,   4 ) \
    HANDLER(WirelessAPMacAddress,                                     35,  8,   4 ) \
    HANDLER(GlobalIPAddress,                                          36,  9,   4 ) \
    HANDLER(EnableWirelessInterfaceFlag,                              37,  10,  10) \
    HANDLER(EnableWifiFlag,                                           38,  11,  10) \
    HANDLER(EnableBluetoothFlag,                                      39,  12,  10) \
    HANDLER(EnableNFCFlag,                                            40,  13,  10) \
    HANDLER(NintendoZoneSSIDListVersion,                              41,  14,  4 ) \
    HANDLER(LANAdapterMacAddress,                                     42,  15,  4 ) \
    HANDLER(ApplicationID,                                            43,  16,  4 ) \
    HANDLER(ApplicationTitle,                                         44,  16,  4 ) \
    HANDLER(ApplicationVersion,                                       45,  16,  4 ) \
    HANDLER(ApplicationStorageLocation,                               46,  16,  4 ) \
    HANDLER(DownloadContentType,                                      47,  17,  4 ) \
    HANDLER(InstallContentType,                                       48,  17,  4 ) \
    HANDLER(ConsoleStartingUpFlag,                                    49,  17,  10) \
    HANDLER(SystemStartingUpFlag,                                     50,  17,  10) \
    HANDLER(ConsoleFirstInitFlag,                                     51,  17,  10) \
    HANDLER(HomeMenuScreenDisplayedFlag,                              52,  17,  10) \
    HANDLER(DataManagementScreenDisplayedFlag,                        53,  17,  10) \
    HANDLER(ConnectionTestingFlag,                                    54,  17,  10) \
    HANDLER(ApplicationRunningFlag,                                   55,  17,  10) \
    HANDLER(DataCorruptionDetectedFlag,                               56,  17,  10) \
    HANDLER(ProductModel,                                             57,  18,  4 ) \
    HANDLER(CurrentLanguage,                                          58,  19,  4 ) \
    HANDLER(UseNetworkTimeProtocolFlag,                               59,  20,  10) \
    HANDLER(TimeZone,                                                 60,  21,  4 ) \
    HANDLER(ControllerFirmware,                                       61,  22,  4 ) \
    HANDLER(VideoOutputSetting,                                       62,  23,  4 ) \
    HANDLER(NANDFreeSpace,                                            63,  24,  0 ) \
    HANDLER(SDCardFreeSpace,                                          64,  25,  0 ) \
    HANDLER(SerialNumber,                                             65,  59,  4 ) \
    HANDLER(OsVersion,                                                66,  59,  4 ) \
    HANDLER(ScreenBrightnessAutoAdjustFlag,                           67,  26,  10) \
    HANDLER(HdmiAudioOutputMode,                                      68,  27,  4 ) \
    HANDLER(SpeakerAudioOutputMode,                                   69,  27,  4 ) \
    HANDLER(HeadphoneAudioOutputMode,                                 70,  27,  4 ) \
    HANDLER(MuteOnHeadsetUnpluggedFlag,                               71,  28,  10) \
    HANDLER(NumUserRegistered,                                        72,  29,  3 ) \
    HANDLER(StorageAutoOrganizeFlag,                                  73,  30,  10) \
    HANDLER(ControllerVibrationVolume,                                74,  31,  4 ) \
    HANDLER(LockScreenFlag,                                           75,  32,  10) \
    HANDLER(InternalBatteryLotNumber,                                 76,  33,  4 ) \
    HANDLER(LeftControllerSerialNumber,                               77,  34,  4 ) \
    HANDLER(RightControllerSerialNumber,                              78,  35,  4 ) \
    HANDLER(NotifyInGameDownloadCompletionFlag,                       79,  36,  10) \
    HANDLER(NotificationSoundFlag,                                    80,  36,  10) \
    HANDLER(TVResolutionSetting,                                      81,  37,  4 ) \
    HANDLER(RGBRangeSetting,                                          82,  37,  4 ) \
    HANDLER(ReduceScreenBurnFlag,                                     83,  37,  10) \
    HANDLER(TVAllowsCecFlag,                                          84,  37,  10) \
    HANDLER(HandheldModeTimeToScreenSleep,                            85,  38,  4 ) \
    HANDLER(ConsoleModeTimeToScreenSleep,                             86,  38,  4 ) \
    HANDLER(StopAutoSleepDuringContentPlayFlag,                       87,  38,  10) \
    HANDLER(LastConnectionTestDownloadSpeed,                          88,  39,  1 ) \
    HANDLER(LastConnectionTestUploadSpeed,                            89,  39,  1 ) \
    HANDLER(DEPRECATED_ServerFQDN,                                    90,  40,  4 ) \
    HANDLER(HTTPRequestContents,                                      91,  40,  4 ) \
    HANDLER(HTTPRequestResponseContents,                              92,  40,  4 ) \
    HANDLER(EdgeServerIPAddress,                                      93,  40,  4 ) \
    HANDLER(CDNContentPath,                                           94,  40,  4 ) \
    HANDLER(FileAccessPath,                                           95,  41,  4 ) \
    HANDLER(GameCardCID,                                              96,  42,  5 ) \
    HANDLER(NANDCID,                                                  97,  43,  5 ) \
    HANDLER(MicroSDCID,                                               98,  44,  5 ) \
    HANDLER(NANDSpeedMode,                                            99,  45,  4 ) \
    HANDLER(MicroSDSpeedMode,                                         100, 46,  4 ) \
    HANDLER(GameCardSpeedMode,                                        101, 47,  4 ) \
    HANDLER(UserAccountInternalID,                                    102, 48,  4 ) \
    HANDLER(NetworkServiceAccountInternalID,                          103, 49,  4 ) \
    HANDLER(NintendoAccountInternalID,                                104, 50,  4 ) \
    HANDLER(USB3AvailableFlag,                                        105, 51,  10) \
    HANDLER(CallStack,                                                106, 52,  4 ) \
    HANDLER(SystemStartupLog,                                         107, 53,  4 ) \
    HANDLER(RegionSetting,                                            108, 54,  4 ) \
    HANDLER(NintendoZoneConnectedFlag,                                109, 55,  10) \
    HANDLER(ForcedSleepHighTemperatureReading,                        110, 56,  1 ) \
    HANDLER(ForcedSleepFanSpeedReading,                               111, 56,  1 ) \
    HANDLER(ForcedSleepHWInfo,                                        112, 56,  4 ) \
    HANDLER(AbnormalPowerStateInfo,                                   113, 57,  1 ) \
    HANDLER(ScreenBrightnessLevel,                                    114, 26,  4 ) \
    HANDLER(ProgramId,                                                115, 1,   4 ) \
    HANDLER(AbortFlag,                                                116, 1,   10) \
    HANDLER(ReportVisibilityFlag,                                     117, 59,  10) \
    HANDLER(FatalFlag,                                                118, 1,   10) \
    HANDLER(OccurrenceTimestampNet,                                   119, 59,  2 ) \
    HANDLER(ResultBacktrace,                                          120, 1,   6 ) \
    HANDLER(GeneralRegisterAarch32,                                   121, 1,   6 ) \
    HANDLER(StackBacktrace32,                                         122, 1,   6 ) \
    HANDLER(ExceptionInfoAarch32,                                     123, 1,   6 ) \
    HANDLER(GeneralRegisterAarch64,                                   124, 1,   7 ) \
    HANDLER(ExceptionInfoAarch64,                                     125, 1,   7 ) \
    HANDLER(StackBacktrace64,                                         126, 1,   7 ) \
    HANDLER(RegisterSetFlag32,                                        127, 1,   1 ) \
    HANDLER(RegisterSetFlag64,                                        128, 1,   0 ) \
    HANDLER(ProgramMappedAddr32,                                      129, 1,   1 ) \
    HANDLER(ProgramMappedAddr64,                                      130, 1,   0 ) \
    HANDLER(AbortType,                                                131, 1,   1 ) \
    HANDLER(PrivateOsVersion,                                         132, 59,  4 ) \
    HANDLER(CurrentSystemPowerState,                                  133, 62,  1 ) \
    HANDLER(PreviousSystemPowerState,                                 134, 62,  1 ) \
    HANDLER(DestinationSystemPowerState,                              135, 62,  1 ) \
    HANDLER(PscTransitionCurrentState,                                136, 1,   1 ) \
    HANDLER(PscTransitionPreviousState,                               137, 1,   1 ) \
    HANDLER(PscInitializedList,                                       138, 1,   5 ) \
    HANDLER(PscCurrentPmStateList,                                    139, 1,   6 ) \
    HANDLER(PscNextPmStateList,                                       140, 1,   6 ) \
    HANDLER(PerformanceMode,                                          141, 63,  3 ) \
    HANDLER(PerformanceConfiguration,                                 142, 63,  1 ) \
    HANDLER(Throttled,                                                143, 64,  10) \
    HANDLER(ThrottlingDuration,                                       144, 64,  2 ) \
    HANDLER(ThrottlingTimestamp,                                      145, 64,  2 ) \
    HANDLER(GameCardCrcErrorCount,                                    146, 65,  1 ) \
    HANDLER(GameCardAsicCrcErrorCount,                                147, 65,  1 ) \
    HANDLER(GameCardRefreshCount,                                     148, 65,  1 ) \
    HANDLER(GameCardReadRetryCount,                                   149, 65,  1 ) \
    HANDLER(EdidBlock,                                                150, 66,  5 ) \
    HANDLER(EdidExtensionBlock,                                       151, 66,  5 ) \
    HANDLER(CreateProcessFailureFlag,                                 152, 1,   10) \
    HANDLER(TemperaturePcb,                                           153, 67,  3 ) \
    HANDLER(TemperatureSoc,                                           154, 67,  3 ) \
    HANDLER(CurrentFanDuty,                                           155, 67,  3 ) \
    HANDLER(LastDvfsThresholdTripped,                                 156, 67,  3 ) \
    HANDLER(CradlePdcHFwVersion,                                      157, 68,  1 ) \
    HANDLER(CradlePdcAFwVersion,                                      158, 68,  1 ) \
    HANDLER(CradleMcuFwVersion,                                       159, 68,  1 ) \
    HANDLER(CradleDp2HdmiFwVersion,                                   160, 68,  1 ) \
    HANDLER(RunningApplicationId,                                     161, 69,  4 ) \
    HANDLER(RunningApplicationTitle,                                  162, 69,  4 ) \
    HANDLER(RunningApplicationVersion,                                163, 69,  4 ) \
    HANDLER(RunningApplicationStorageLocation,                        164, 69,  4 ) \
    HANDLER(RunningAppletList,                                        165, 70,  7 ) \
    HANDLER(FocusedAppletHistory,                                     166, 71,  7 ) \
    HANDLER(CompositorState,                                          167, 99,  4 ) \
    HANDLER(CompositorLayerState,                                     168, 100, 4 ) \
    HANDLER(CompositorDisplayState,                                   169, 101, 4 ) \
    HANDLER(CompositorHWCState,                                       170, 102, 4 ) \
    HANDLER(InputCurrentLimit,                                        171, 73,  3 ) \
    HANDLER(BoostModeCurrentLimit,                                    172, 73,  3 ) \
    HANDLER(FastChargeCurrentLimit,                                   173, 73,  3 ) \
    HANDLER(ChargeVoltageLimit,                                       174, 73,  3 ) \
    HANDLER(ChargeConfiguration,                                      175, 73,  3 ) \
    HANDLER(HizMode,                                                  176, 73,  10) \
    HANDLER(ChargeEnabled,                                            177, 73,  10) \
    HANDLER(PowerSupplyPath,                                          178, 73,  3 ) \
    HANDLER(BatteryTemperature,                                       179, 73,  3 ) \
    HANDLER(BatteryChargePercent,                                     180, 73,  3 ) \
    HANDLER(BatteryChargeVoltage,                                     181, 73,  3 ) \
    HANDLER(BatteryAge,                                               182, 73,  3 ) \
    HANDLER(PowerRole,                                                183, 73,  3 ) \
    HANDLER(PowerSupplyType,                                          184, 73,  3 ) \
    HANDLER(PowerSupplyVoltage,                                       185, 73,  3 ) \
    HANDLER(PowerSupplyCurrent,                                       186, 73,  3 ) \
    HANDLER(FastBatteryChargingEnabled,                               187, 73,  10) \
    HANDLER(ControllerPowerSupplyAcquired,                            188, 73,  10) \
    HANDLER(OtgRequested,                                             189, 73,  10) \
    HANDLER(NANDPreEolInfo,                                           190, 74,  1 ) \
    HANDLER(NANDDeviceLifeTimeEstTypA,                                191, 74,  1 ) \
    HANDLER(NANDDeviceLifeTimeEstTypB,                                192, 74,  1 ) \
    HANDLER(NANDPatrolCount,                                          193, 75,  1 ) \
    HANDLER(NANDNumActivationFailures,                                194, 76,  1 ) \
    HANDLER(NANDNumActivationErrorCorrections,                        195, 76,  1 ) \
    HANDLER(NANDNumReadWriteFailures,                                 196, 76,  1 ) \
    HANDLER(NANDNumReadWriteErrorCorrections,                         197, 76,  1 ) \
    HANDLER(NANDErrorLog,                                             198, 77,  4 ) \
    HANDLER(SdCardUserAreaSize,                                       199, 78,  2 ) \
    HANDLER(SdCardProtectedAreaSize,                                  200, 78,  2 ) \
    HANDLER(SdCardNumActivationFailures,                              201, 79,  1 ) \
    HANDLER(SdCardNumActivationErrorCorrections,                      202, 79,  1 ) \
    HANDLER(SdCardNumReadWriteFailures,                               203, 79,  1 ) \
    HANDLER(SdCardNumReadWriteErrorCorrections,                       204, 79,  1 ) \
    HANDLER(SdCardErrorLog,                                           205, 80,  4 ) \
    HANDLER(EncryptionKey,                                            206, 1,   5 ) \
    HANDLER(EncryptedExceptionInfo,                                   207, 1,   5 ) \
    HANDLER(GameCardTimeoutRetryErrorCount,                           208, 65,  1 ) \
    HANDLER(FsRemountForDataCorruptCount,                             209, 81,  1 ) \
    HANDLER(FsRemountForDataCorruptRetryOutCount,                     210, 81,  1 ) \
    HANDLER(GameCardInsertionCount,                                   211, 65,  1 ) \
    HANDLER(GameCardRemovalCount,                                     212, 65,  1 ) \
    HANDLER(GameCardAsicInitializeCount,                              213, 65,  1 ) \
    HANDLER(TestU16,                                                  214, 0,   11) \
    HANDLER(TestU8,                                                   215, 0,   12) \
    HANDLER(TestI16,                                                  216, 0,   13) \
    HANDLER(TestI8,                                                   217, 0,   14) \
    HANDLER(SystemAppletScene,                                        218, 82,  12) \
    HANDLER(CodecType,                                                219, 83,  1 ) \
    HANDLER(DecodeBuffers,                                            220, 83,  1 ) \
    HANDLER(FrameWidth,                                               221, 83,  3 ) \
    HANDLER(FrameHeight,                                              222, 83,  3 ) \
    HANDLER(ColorPrimaries,                                           223, 83,  12) \
    HANDLER(TransferCharacteristics,                                  224, 83,  12) \
    HANDLER(MatrixCoefficients,                                       225, 83,  12) \
    HANDLER(DisplayWidth,                                             226, 83,  3 ) \
    HANDLER(DisplayHeight,                                            227, 83,  3 ) \
    HANDLER(DARWidth,                                                 228, 83,  3 ) \
    HANDLER(DARHeight,                                                229, 83,  3 ) \
    HANDLER(ColorFormat,                                              230, 83,  1 ) \
    HANDLER(ColorSpace,                                               231, 83,  4 ) \
    HANDLER(SurfaceLayout,                                            232, 83,  4 ) \
    HANDLER(BitStream,                                                233, 83,  5 ) \
    HANDLER(VideoDecState,                                            234, 83,  4 ) \
    HANDLER(GpuErrorChannelId,                                        235, 84,  1 ) \
    HANDLER(GpuErrorAruId,                                            236, 84,  0 ) \
    HANDLER(GpuErrorType,                                             237, 84,  1 ) \
    HANDLER(GpuErrorFaultInfo,                                        238, 84,  1 ) \
    HANDLER(GpuErrorWriteAccess,                                      239, 84,  10) \
    HANDLER(GpuErrorFaultAddress,                                     240, 84,  0 ) \
    HANDLER(GpuErrorFaultUnit,                                        241, 84,  1 ) \
    HANDLER(GpuErrorFaultType,                                        242, 84,  1 ) \
    HANDLER(GpuErrorHwContextPointer,                                 243, 84,  0 ) \
    HANDLER(GpuErrorContextStatus,                                    244, 84,  1 ) \
    HANDLER(GpuErrorPbdmaIntr,                                        245, 84,  1 ) \
    HANDLER(GpuErrorPbdmaErrorType,                                   246, 84,  1 ) \
    HANDLER(GpuErrorPbdmaHeaderShadow,                                247, 84,  1 ) \
    HANDLER(GpuErrorPbdmaHeader,                                      248, 84,  1 ) \
    HANDLER(GpuErrorPbdmaGpShadow0,                                   249, 84,  1 ) \
    HANDLER(GpuErrorPbdmaGpShadow1,                                   250, 84,  1 ) \
    HANDLER(AccessPointChannel,                                       251, 60,  11) \
    HANDLER(ThreadName,                                               252, 1,   4 ) \
    HANDLER(AdspExceptionRegisters,                                   253, 86,  6 ) \
    HANDLER(AdspExceptionSpsr,                                        254, 86,  1 ) \
    HANDLER(AdspExceptionProgramCounter,                              255, 86,  1 ) \
    HANDLER(AdspExceptionLinkRegister,                                256, 86,  1 ) \
    HANDLER(AdspExceptionStackPointer,                                257, 86,  1 ) \
    HANDLER(AdspExceptionArmModeRegisters,                            258, 86,  6 ) \
    HANDLER(AdspExceptionStackAddress,                                259, 86,  1 ) \
    HANDLER(AdspExceptionStackDump,                                   260, 86,  6 ) \
    HANDLER(AdspExceptionReason,                                      261, 86,  1 ) \
    HANDLER(OscillatorClock,                                          262, 85,  1 ) \
    HANDLER(CpuDvfsTableClocks,                                       263, 85,  6 ) \
    HANDLER(CpuDvfsTableVoltages,                                     264, 85,  8 ) \
    HANDLER(GpuDvfsTableClocks,                                       265, 85,  6 ) \
    HANDLER(GpuDvfsTableVoltages,                                     266, 85,  8 ) \
    HANDLER(EmcDvfsTableClocks,                                       267, 85,  6 ) \
    HANDLER(EmcDvfsTableVoltages,                                     268, 85,  8 ) \
    HANDLER(ModuleClockFrequencies,                                   269, 85,  6 ) \
    HANDLER(ModuleClockEnableFlags,                                   270, 85,  5 ) \
    HANDLER(ModulePowerEnableFlags,                                   271, 85,  5 ) \
    HANDLER(ModuleResetAssertFlags,                                   272, 85,  5 ) \
    HANDLER(ModuleMinimumVoltageClockRates,                           273, 85,  6 ) \
    HANDLER(PowerDomainEnableFlags,                                   274, 85,  5 ) \
    HANDLER(PowerDomainVoltages,                                      275, 85,  8 ) \
    HANDLER(AccessPointRssi,                                          276, 58,  3 ) \
    HANDLER(FuseInfo,                                                 277, 85,  6 ) \
    HANDLER(VideoLog,                                                 278, 83,  4 ) \
    HANDLER(GameCardDeviceId,                                         279, 42,  5 ) \
    HANDLER(GameCardAsicReinitializeCount,                            280, 65,  11) \
    HANDLER(GameCardAsicReinitializeFailureCount,                     281, 65,  11) \
    HANDLER(GameCardAsicReinitializeFailureDetail,                    282, 65,  11) \
    HANDLER(GameCardRefreshSuccessCount,                              283, 65,  11) \
    HANDLER(GameCardAwakenCount,                                      284, 65,  1 ) \
    HANDLER(GameCardAwakenFailureCount,                               285, 65,  11) \
    HANDLER(GameCardReadCountFromInsert,                              286, 65,  1 ) \
    HANDLER(GameCardReadCountFromAwaken,                              287, 65,  1 ) \
    HANDLER(GameCardLastReadErrorPageAddress,                         288, 65,  1 ) \
    HANDLER(GameCardLastReadErrorPageCount,                           289, 65,  1 ) \
    HANDLER(AppletManagerContextTrace,                                290, 1,   8 ) \
    HANDLER(NvDispIsRegistered,                                       291, 87,  10) \
    HANDLER(NvDispIsSuspend,                                          292, 87,  10) \
    HANDLER(NvDispDC0SurfaceNum,                                      293, 87,  8 ) \
    HANDLER(NvDispDC1SurfaceNum,                                      294, 87,  8 ) \
    HANDLER(NvDispWindowSrcRectX,                                     295, 88,  1 ) \
    HANDLER(NvDispWindowSrcRectY,                                     296, 88,  1 ) \
    HANDLER(NvDispWindowSrcRectWidth,                                 297, 88,  1 ) \
    HANDLER(NvDispWindowSrcRectHeight,                                298, 88,  1 ) \
    HANDLER(NvDispWindowDstRectX,                                     299, 88,  1 ) \
    HANDLER(NvDispWindowDstRectY,                                     300, 88,  1 ) \
    HANDLER(NvDispWindowDstRectWidth,                                 301, 88,  1 ) \
    HANDLER(NvDispWindowDstRectHeight,                                302, 88,  1 ) \
    HANDLER(NvDispWindowIndex,                                        303, 88,  1 ) \
    HANDLER(NvDispWindowBlendOperation,                               304, 88,  1 ) \
    HANDLER(NvDispWindowAlphaOperation,                               305, 88,  1 ) \
    HANDLER(NvDispWindowDepth,                                        306, 88,  1 ) \
    HANDLER(NvDispWindowAlpha,                                        307, 88,  12) \
    HANDLER(NvDispWindowHFilter,                                      308, 88,  10) \
    HANDLER(NvDispWindowVFilter,                                      309, 88,  10) \
    HANDLER(NvDispWindowOptions,                                      310, 88,  1 ) \
    HANDLER(NvDispWindowSyncPointId,                                  311, 88,  1 ) \
    HANDLER(NvDispDPSorPower,                                         312, 89,  10) \
    HANDLER(NvDispDPClkType,                                          313, 89,  12) \
    HANDLER(NvDispDPEnable,                                           314, 89,  1 ) \
    HANDLER(NvDispDPState,                                            315, 89,  1 ) \
    HANDLER(NvDispDPEdid,                                             316, 89,  5 ) \
    HANDLER(NvDispDPEdidSize,                                         317, 89,  1 ) \
    HANDLER(NvDispDPEdidExtSize,                                      318, 89,  1 ) \
    HANDLER(NvDispDPFakeMode,                                         319, 89,  10) \
    HANDLER(NvDispDPModeNumber,                                       320, 89,  1 ) \
    HANDLER(NvDispDPPlugInOut,                                        321, 89,  10) \
    HANDLER(NvDispDPAuxIntHandler,                                    322, 89,  10) \
    HANDLER(NvDispDPForceMaxLinkBW,                                   323, 89,  10) \
    HANDLER(NvDispDPIsConnected,                                      324, 89,  10) \
    HANDLER(NvDispDPLinkValid,                                        325, 90,  10) \
    HANDLER(NvDispDPLinkMaxBW,                                        326, 90,  12) \
    HANDLER(NvDispDPLinkMaxLaneCount,                                 327, 90,  12) \
    HANDLER(NvDispDPLinkDownSpread,                                   328, 90,  10) \
    HANDLER(NvDispDPLinkSupportEnhancedFraming,                       329, 90,  10) \
    HANDLER(NvDispDPLinkBpp,                                          330, 90,  1 ) \
    HANDLER(NvDispDPLinkScaramberCap,                                 331, 90,  10) \
    HANDLER(NvDispDPLinkBW,                                           332, 91,  12) \
    HANDLER(NvDispDPLinkLaneCount,                                    333, 91,  12) \
    HANDLER(NvDispDPLinkEnhancedFraming,                              334, 91,  10) \
    HANDLER(NvDispDPLinkScrambleEnable,                               335, 91,  10) \
    HANDLER(NvDispDPLinkActivePolarity,                               336, 91,  1 ) \
    HANDLER(NvDispDPLinkActiveCount,                                  337, 91,  1 ) \
    HANDLER(NvDispDPLinkTUSize,                                       338, 91,  1 ) \
    HANDLER(NvDispDPLinkActiveFrac,                                   339, 91,  1 ) \
    HANDLER(NvDispDPLinkWatermark,                                    340, 91,  1 ) \
    HANDLER(NvDispDPLinkHBlank,                                       341, 91,  1 ) \
    HANDLER(NvDispDPLinkVBlank,                                       342, 91,  1 ) \
    HANDLER(NvDispDPLinkOnlyEnhancedFraming,                          343, 91,  10) \
    HANDLER(NvDispDPLinkOnlyEdpCap,                                   344, 91,  10) \
    HANDLER(NvDispDPLinkSupportFastLT,                                345, 91,  10) \
    HANDLER(NvDispDPLinkLTDataValid,                                  346, 91,  10) \
    HANDLER(NvDispDPLinkTsp3Support,                                  347, 91,  10) \
    HANDLER(NvDispDPLinkAuxInterval,                                  348, 91,  12) \
    HANDLER(NvDispHdcpCreated,                                        349, 92,  10) \
    HANDLER(NvDispHdcpUserRequest,                                    350, 92,  10) \
    HANDLER(NvDispHdcpPlugged,                                        351, 92,  10) \
    HANDLER(NvDispHdcpState,                                          352, 92,  1 ) \
    HANDLER(NvDispHdcpFailCount,                                      353, 92,  3 ) \
    HANDLER(NvDispHdcpHdcp22,                                         354, 92,  14) \
    HANDLER(NvDispHdcpMaxRetry,                                       355, 92,  12) \
    HANDLER(NvDispHdcpHpd,                                            356, 92,  12) \
    HANDLER(NvDispHdcpRepeater,                                       357, 92,  12) \
    HANDLER(NvDispCecRxBuf,                                           358, 93,  5 ) \
    HANDLER(NvDispCecRxLength,                                        359, 93,  3 ) \
    HANDLER(NvDispCecTxBuf,                                           360, 93,  5 ) \
    HANDLER(NvDispCecTxLength,                                        361, 93,  3 ) \
    HANDLER(NvDispCecTxRet,                                           362, 93,  3 ) \
    HANDLER(NvDispCecState,                                           363, 93,  1 ) \
    HANDLER(NvDispCecTxInfo,                                          364, 93,  12) \
    HANDLER(NvDispCecRxInfo,                                          365, 93,  12) \
    HANDLER(NvDispDCIndex,                                            366, 94,  1 ) \
    HANDLER(NvDispDCInitialize,                                       367, 94,  10) \
    HANDLER(NvDispDCClock,                                            368, 94,  10) \
    HANDLER(NvDispDCFrequency,                                        369, 94,  1 ) \
    HANDLER(NvDispDCFailed,                                           370, 94,  10) \
    HANDLER(NvDispDCModeWidth,                                        371, 94,  3 ) \
    HANDLER(NvDispDCModeHeight,                                       372, 94,  3 ) \
    HANDLER(NvDispDCModeBpp,                                          373, 94,  1 ) \
    HANDLER(NvDispDCPanelFrequency,                                   374, 94,  1 ) \
    HANDLER(NvDispDCWinDirty,                                         375, 94,  1 ) \
    HANDLER(NvDispDCWinEnable,                                        376, 94,  1 ) \
    HANDLER(NvDispDCVrr,                                              377, 94,  10) \
    HANDLER(NvDispDCPanelInitialize,                                  378, 94,  10) \
    HANDLER(NvDispDsiDataFormat,                                      379, 95,  1 ) \
    HANDLER(NvDispDsiVideoMode,                                       380, 95,  1 ) \
    HANDLER(NvDispDsiRefreshRate,                                     381, 95,  1 ) \
    HANDLER(NvDispDsiLpCmdModeFrequency,                              382, 95,  1 ) \
    HANDLER(NvDispDsiHsCmdModeFrequency,                              383, 95,  1 ) \
    HANDLER(NvDispDsiPanelResetTimeout,                               384, 95,  1 ) \
    HANDLER(NvDispDsiPhyFrequency,                                    385, 95,  1 ) \
    HANDLER(NvDispDsiFrequency,                                       386, 95,  1 ) \
    HANDLER(NvDispDsiInstance,                                        387, 95,  1 ) \
    HANDLER(NvDispDcDsiHostCtrlEnable,                                388, 95,  10) \
    HANDLER(NvDispDcDsiInit,                                          389, 95,  10) \
    HANDLER(NvDispDcDsiEnable,                                        390, 95,  10) \
    HANDLER(NvDispDcDsiHsMode,                                        391, 95,  10) \
    HANDLER(NvDispDcDsiVendorId,                                      392, 95,  5 ) \
    HANDLER(NvDispDcDsiLcdVendorNum,                                  393, 95,  12) \
    HANDLER(NvDispDcDsiHsClockControl,                                394, 95,  1 ) \
    HANDLER(NvDispDcDsiEnableHsClockInLpMode,                         395, 95,  10) \
    HANDLER(NvDispDcDsiTeFrameUpdate,                                 396, 95,  10) \
    HANDLER(NvDispDcDsiGangedType,                                    397, 95,  1 ) \
    HANDLER(NvDispDcDsiHbpInPktSeq,                                   398, 95,  10) \
    HANDLER(NvDispErrID,                                              399, 96,  1 ) \
    HANDLER(NvDispErrData0,                                           400, 96,  1 ) \
    HANDLER(NvDispErrData1,                                           401, 96,  1 ) \
    HANDLER(SdCardMountStatus,                                        402, 97,  4 ) \
    HANDLER(SdCardMountUnexpectedResult,                              403, 97,  4 ) \
    HANDLER(NANDTotalSize,                                            404, 24,  0 ) \
    HANDLER(SdCardTotalSize,                                          405, 25,  0 ) \
    HANDLER(ElapsedTimeSinceInitialLaunch,                            406, 59,  2 ) \
    HANDLER(ElapsedTimeSincePowerOn,                                  407, 59,  2 ) \
    HANDLER(ElapsedTimeSinceLastAwake,                                408, 59,  2 ) \
    HANDLER(OccurrenceTick,                                           409, 59,  2 ) \
    HANDLER(RetailInteractiveDisplayFlag,                             410, 98,  10) \
    HANDLER(FatFsError,                                               411, 81,  3 ) \
    HANDLER(FatFsExtraError,                                          412, 81,  3 ) \
    HANDLER(FatFsErrorDrive,                                          413, 81,  3 ) \
    HANDLER(FatFsErrorName,                                           414, 81,  4 ) \
    HANDLER(MonitorManufactureCode,                                   415, 103, 4 ) \
    HANDLER(MonitorProductCode,                                       416, 103, 11) \
    HANDLER(MonitorSerialNumber,                                      417, 103, 1 ) \
    HANDLER(MonitorManufactureYear,                                   418, 103, 3 ) \
    HANDLER(PhysicalAddress,                                          419, 103, 11) \
    HANDLER(Is4k60Hz,                                                 420, 103, 10) \
    HANDLER(Is4k30Hz,                                                 421, 103, 10) \
    HANDLER(Is1080P60Hz,                                              422, 103, 10) \
    HANDLER(Is720P60Hz,                                               423, 103, 10) \
    HANDLER(PcmChannelMax,                                            424, 103, 3 ) \
    HANDLER(CrashReportHash,                                          425, 1,   5 ) \
    HANDLER(ErrorReportSharePermission,                               426, 104, 12) \
    HANDLER(VideoCodecTypeEnum,                                       427, 105, 3 ) \
    HANDLER(VideoBitRate,                                             428, 105, 3 ) \
    HANDLER(VideoFrameRate,                                           429, 105, 3 ) \
    HANDLER(VideoWidth,                                               430, 105, 3 ) \
    HANDLER(VideoHeight,                                              431, 105, 3 ) \
    HANDLER(AudioCodecTypeEnum,                                       432, 105, 3 ) \
    HANDLER(AudioSampleRate,                                          433, 105, 3 ) \
    HANDLER(AudioChannelCount,                                        434, 105, 3 ) \
    HANDLER(AudioBitRate,                                             435, 105, 3 ) \
    HANDLER(MultimediaContainerType,                                  436, 105, 3 ) \
    HANDLER(MultimediaProfileType,                                    437, 105, 3 ) \
    HANDLER(MultimediaLevelType,                                      438, 105, 3 ) \
    HANDLER(MultimediaCacheSizeEnum,                                  439, 105, 3 ) \
    HANDLER(MultimediaErrorStatusEnum,                                440, 105, 3 ) \
    HANDLER(MultimediaErrorLog,                                       441, 105, 5 ) \
    HANDLER(ServerFqdn,                                               442, 1,   4 ) \
    HANDLER(ServerIpAddress,                                          443, 1,   4 ) \
    HANDLER(TestStringEncrypt,                                        444, 0,   4 ) \
    HANDLER(TestU8ArrayEncrypt,                                       445, 0,   5 ) \
    HANDLER(TestU32ArrayEncrypt,                                      446, 0,   6 ) \
    HANDLER(TestU64ArrayEncrypt,                                      447, 0,   7 ) \
    HANDLER(TestI32ArrayEncrypt,                                      448, 0,   8 ) \
    HANDLER(TestI64ArrayEncrypt,                                      449, 0,   9 ) \
    HANDLER(CipherKey,                                                450, 59,  5 ) \
    HANDLER(FileSystemPath,                                           451, 1,   4 ) \
    HANDLER(WebMediaPlayerOpenUrl,                                    452, 1,   4 ) \
    HANDLER(WebMediaPlayerLastSocketErrors,                           453, 1,   8 ) \
    HANDLER(UnknownControllerCount,                                   454, 106, 12) \
    HANDLER(AttachedControllerCount,                                  455, 106, 12) \
    HANDLER(BluetoothControllerCount,                                 456, 106, 12) \
    HANDLER(UsbControllerCount,                                       457, 106, 12) \
    HANDLER(ControllerTypeList,                                       458, 106, 5 ) \
    HANDLER(ControllerInterfaceList,                                  459, 106, 5 ) \
    HANDLER(ControllerStyleList,                                      460, 106, 5 ) \
    HANDLER(FsPooledBufferPeakFreeSize,                               461, 107, 0 ) \
    HANDLER(FsPooledBufferRetriedCount,                               462, 107, 0 ) \
    HANDLER(FsPooledBufferReduceAllocationCount,                      463, 107, 0 ) \
    HANDLER(FsBufferManagerPeakFreeSize,                              464, 107, 0 ) \
    HANDLER(FsBufferManagerRetriedCount,                              465, 107, 0 ) \
    HANDLER(FsExpHeapPeakFreeSize,                                    466, 107, 0 ) \
    HANDLER(FsBufferPoolPeakFreeSize,                                 467, 107, 0 ) \
    HANDLER(FsPatrolReadAllocateBufferSuccessCount,                   468, 107, 0 ) \
    HANDLER(FsPatrolReadAllocateBufferFailureCount,                   469, 107, 0 ) \
    HANDLER(SteadyClockInternalOffset,                                470, 59,  2 ) \
    HANDLER(SteadyClockCurrentTimePointValue,                         471, 59,  2 ) \
    HANDLER(UserClockContextOffset,                                   472, 108, 2 ) \
    HANDLER(UserClockContextTimeStampValue,                           473, 108, 2 ) \
    HANDLER(NetworkClockContextOffset,                                474, 109, 2 ) \
    HANDLER(NetworkClockContextTimeStampValue,                        475, 109, 2 ) \
    HANDLER(SystemAbortFlag,                                          476, 1,   10) \
    HANDLER(ApplicationAbortFlag,                                     477, 1,   10) \
    HANDLER(NifmErrorCode,                                            478, 2,   4 ) \
    HANDLER(LcsApplicationId,                                         479, 1,   4 ) \
    HANDLER(LcsContentMetaKeyIdList,                                  480, 1,   7 ) \
    HANDLER(LcsContentMetaKeyVersionList,                             481, 1,   6 ) \
    HANDLER(LcsContentMetaKeyTypeList,                                482, 1,   5 ) \
    HANDLER(LcsContentMetaKeyInstallTypeList,                         483, 1,   5 ) \
    HANDLER(LcsSenderFlag,                                            484, 1,   10) \
    HANDLER(LcsApplicationRequestFlag,                                485, 1,   10) \
    HANDLER(LcsHasExFatDriverFlag,                                    486, 1,   10) \
    HANDLER(LcsIpAddress,                                             487, 1,   1 ) \
    HANDLER(AcpStartupUserAccount,                                    488, 110, 12) \
    HANDLER(AcpAocRegistrationType,                                   489, 112, 12) \
    HANDLER(AcpAttributeFlag,                                         490, 110, 1 ) \
    HANDLER(AcpSupportedLanguageFlag,                                 491, 110, 1 ) \
    HANDLER(AcpParentalControlFlag,                                   492, 110, 1 ) \
    HANDLER(AcpScreenShot,                                            493, 110, 12) \
    HANDLER(AcpVideoCapture,                                          494, 110, 12) \
    HANDLER(AcpDataLossConfirmation,                                  495, 110, 12) \
    HANDLER(AcpPlayLogPolicy,                                         496, 111, 12) \
    HANDLER(AcpPresenceGroupId,                                       497, 110, 0 ) \
    HANDLER(AcpRatingAge,                                             498, 115, 15) \
    HANDLER(AcpAocBaseId,                                             499, 112, 0 ) \
    HANDLER(AcpSaveDataOwnerId,                                       500, 114, 0 ) \
    HANDLER(AcpUserAccountSaveDataSize,                               501, 114, 2 ) \
    HANDLER(AcpUserAccountSaveDataJournalSize,                        502, 114, 2 ) \
    HANDLER(AcpDeviceSaveDataSize,                                    503, 114, 2 ) \
    HANDLER(AcpDeviceSaveDataJournalSize,                             504, 114, 2 ) \
    HANDLER(AcpBcatDeliveryCacheStorageSize,                          505, 113, 2 ) \
    HANDLER(AcpApplicationErrorCodeCategory,                          506, 110, 4 ) \
    HANDLER(AcpLocalCommunicationId,                                  507, 110, 7 ) \
    HANDLER(AcpLogoType,                                              508, 110, 12) \
    HANDLER(AcpLogoHandling,                                          509, 110, 12) \
    HANDLER(AcpRuntimeAocInstall,                                     510, 112, 12) \
    HANDLER(AcpCrashReport,                                           511, 110, 12) \
    HANDLER(AcpHdcp,                                                  512, 110, 12) \
    HANDLER(AcpSeedForPseudoDeviceId,                                 513, 110, 0 ) \
    HANDLER(AcpBcatPassphrase,                                        514, 113, 4 ) \
    HANDLER(AcpUserAccountSaveDataSizeMax,                            515, 114, 2 ) \
    HANDLER(AcpUserAccountSaveDataJournalSizeMax,                     516, 114, 2 ) \
    HANDLER(AcpDeviceSaveDataSizeMax,                                 517, 114, 2 ) \
    HANDLER(AcpDeviceSaveDataJournalSizeMax,                          518, 114, 2 ) \
    HANDLER(AcpTemporaryStorageSize,                                  519, 114, 2 ) \
    HANDLER(AcpCacheStorageSize,                                      520, 114, 2 ) \
    HANDLER(AcpCacheStorageJournalSize,                               521, 114, 2 ) \
    HANDLER(AcpCacheStorageDataAndJournalSizeMax,                     522, 114, 2 ) \
    HANDLER(AcpCacheStorageIndexMax,                                  523, 114, 13) \
    HANDLER(AcpPlayLogQueryableApplicationId,                         524, 111, 7 ) \
    HANDLER(AcpPlayLogQueryCapability,                                525, 111, 12) \
    HANDLER(AcpRepairFlag,                                            526, 110, 12) \
    HANDLER(RunningApplicationPatchStorageLocation,                   527, 69,  4 ) \
    HANDLER(RunningApplicationVersionNumber,                          528, 69,  1 ) \
    HANDLER(FsRecoveredByInvalidateCacheCount,                        529, 81,  1 ) \
    HANDLER(FsSaveDataIndexCount,                                     530, 81,  1 ) \
    HANDLER(FsBufferManagerPeakTotalAllocatableSize,                  531, 107, 0 ) \
    HANDLER(MonitorCurrentWidth,                                      532, 116, 11) \
    HANDLER(MonitorCurrentHeight,                                     533, 116, 11) \
    HANDLER(MonitorCurrentRefreshRate,                                534, 116, 4 ) \
    HANDLER(RebootlessSystemUpdateVersion,                            535, 117, 4 ) \
    HANDLER(EncryptedExceptionInfo1,                                  536, 1,   5 ) \
    HANDLER(EncryptedExceptionInfo2,                                  537, 1,   5 ) \
    HANDLER(EncryptedExceptionInfo3,                                  538, 1,   5 ) \
    HANDLER(EncryptedDyingMessage,                                    539, 1,   5 ) \
    HANDLER(DramId,                                                   540, 85,  1 ) \
    HANDLER(NifmConnectionTestRedirectUrl,                            541, 118, 4 ) \
    HANDLER(AcpRequiredNetworkServiceLicenseOnLaunchFlag,             542, 110, 12) \
    HANDLER(PciePort0Flags,                                           543, 119, 1 ) \
    HANDLER(PciePort0Speed,                                           544, 119, 12) \
    HANDLER(PciePort0ResetTimeInUs,                                   545, 119, 1 ) \
    HANDLER(PciePort0IrqCount,                                        546, 119, 1 ) \
    HANDLER(PciePort0Statistics,                                      547, 119, 6 ) \
    HANDLER(PciePort1Flags,                                           548, 119, 1 ) \
    HANDLER(PciePort1Speed,                                           549, 119, 12) \
    HANDLER(PciePort1ResetTimeInUs,                                   550, 119, 1 ) \
    HANDLER(PciePort1IrqCount,                                        551, 119, 1 ) \
    HANDLER(PciePort1Statistics,                                      552, 119, 6 ) \
    HANDLER(PcieFunction0VendorId,                                    553, 119, 11) \
    HANDLER(PcieFunction0DeviceId,                                    554, 119, 11) \
    HANDLER(PcieFunction0PmState,                                     555, 119, 12) \
    HANDLER(PcieFunction0IsAcquired,                                  556, 119, 10) \
    HANDLER(PcieFunction1VendorId,                                    557, 119, 11) \
    HANDLER(PcieFunction1DeviceId,                                    558, 119, 11) \
    HANDLER(PcieFunction1PmState,                                     559, 119, 12) \
    HANDLER(PcieFunction1IsAcquired,                                  560, 119, 10) \
    HANDLER(PcieGlobalRootComplexStatistics,                          561, 119, 6 ) \
    HANDLER(PciePllResistorCalibrationValue,                          562, 119, 12) \
    HANDLER(CertificateRequestedHostName,                             563, 120, 4 ) \
    HANDLER(CertificateCommonName,                                    564, 120, 4 ) \
    HANDLER(CertificateSANCount,                                      565, 120, 1 ) \
    HANDLER(CertificateSANs,                                          566, 120, 4 ) \
    HANDLER(FsBufferPoolMaxAllocateSize,                              567, 107, 0 ) \
    HANDLER(CertificateIssuerName,                                    568, 120, 4 ) \
    HANDLER(ApplicationAliveTime,                                     569, 59,  2 ) \
    HANDLER(ApplicationInFocusTime,                                   570, 59,  2 ) \
    HANDLER(ApplicationOutOfFocusTime,                                571, 59,  2 ) \
    HANDLER(ApplicationBackgroundFocusTime,                           572, 59,  2 ) \
    HANDLER(AcpUserAccountSwitchLock,                                 573, 110, 12) \
    HANDLER(USB3HostAvailableFlag,                                    574, 51,  10) \
    HANDLER(USB3DeviceAvailableFlag,                                  575, 51,  10) \
    HANDLER(AcpNeighborDetectionClientConfigurationSendDataId,        576, 121, 0 ) \
    HANDLER(AcpNeighborDetectionClientConfigurationReceivableDataIds, 577, 121, 7 ) \
    HANDLER(AcpStartupUserAccountOptionFlag,                          578, 110, 12) \
    HANDLER(ServerErrorCode,                                          579, 1,   4 ) \
    HANDLER(AppletManagerMetaLogTrace,                                580, 1,   7 ) \
    HANDLER(ServerCertificateSerialNumber,                            581, 120, 4 ) \
    HANDLER(ServerCertificatePublicKeyAlgorithm,                      582, 120, 4 ) \
    HANDLER(ServerCertificateSignatureAlgorithm,                      583, 120, 4 ) \
    HANDLER(ServerCertificateNotBefore,                               584, 120, 0 ) \
    HANDLER(ServerCertificateNotAfter,                                585, 120, 0 ) \
    HANDLER(CertificateAlgorithmInfoBits,                             586, 120, 0 ) \
    HANDLER(TlsConnectionPeerIpAddress,                               587, 120, 4 ) \
    HANDLER(TlsConnectionLastHandshakeState,                          588, 120, 1 ) \
    HANDLER(TlsConnectionInfoBits,                                    589, 120, 0 ) \
    HANDLER(SslStateBits,                                             590, 120, 0 ) \
    HANDLER(SslProcessInfoBits,                                       591, 120, 0 ) \
    HANDLER(SslProcessHeapSize,                                       592, 120, 1 ) \
    HANDLER(SslBaseErrorCode,                                         593, 120, 3 ) \
    HANDLER(GpuCrashDumpSize,                                         594, 122, 1 ) \
    HANDLER(GpuCrashDump,                                             595, 122, 5 ) \
    HANDLER(RunningApplicationProgramIndex,                           596, 69,  12) \
    HANDLER(UsbTopology,                                              597, 123, 5 ) \
    HANDLER(AkamaiReferenceId,                                        598, 1,   4 ) \
    HANDLER(NvHostErrID,                                              599, 124, 1 ) \
    HANDLER(NvHostErrDataArrayU32,                                    600, 124, 6 ) \
    HANDLER(HasSyslogFlag,                                            601, 1,   10) \
    HANDLER(AcpRuntimeParameterDelivery,                              602, 110, 12) \
    HANDLER(PlatformRegion,                                           603, 54,  4 ) \
    HANDLER(RunningUlaApplicationId,                                  604, 125, 4 ) \
    HANDLER(RunningUlaAppletId,                                       605, 125, 12) \
    HANDLER(RunningUlaVersion,                                        606, 125, 1 ) \
    HANDLER(RunningUlaApplicationStorageLocation,                     607, 125, 4 ) \
    HANDLER(RunningUlaPatchStorageLocation,                           608, 125, 4 ) \
    HANDLER(NANDTotalSizeOfSystem,                                    609, 24,  0 ) \
    HANDLER(NANDFreeSpaceOfSystem,                                    610, 24,  0 ) \

