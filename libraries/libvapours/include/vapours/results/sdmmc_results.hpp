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
#include <vapours/results/results_common.hpp>

namespace ams::sdmmc {

    R_DEFINE_NAMESPACE_RESULT_MODULE(24);

    R_DEFINE_ERROR_RESULT(NoDevice,      1);
    R_DEFINE_ERROR_RESULT(NotActivated,  2);
    R_DEFINE_ERROR_RESULT(DeviceRemoved, 3);
    R_DEFINE_ERROR_RESULT(NotAwakened,   4);

    R_DEFINE_ERROR_RANGE(CommunicationError, 32, 126);
        R_DEFINE_ERROR_RANGE(CommunicationNotAttained, 33, 46);
            R_DEFINE_ERROR_RESULT(ResponseIndexError,               34);
            R_DEFINE_ERROR_RESULT(ResponseEndBitError,              35);
            R_DEFINE_ERROR_RESULT(ResponseCrcError,                 36);
            R_DEFINE_ERROR_RESULT(ResponseTimeoutError,             37);
            R_DEFINE_ERROR_RESULT(DataEndBitError,                  38);
            R_DEFINE_ERROR_RESULT(DataCrcError,                     39);
            R_DEFINE_ERROR_RESULT(DataTimeoutError,                 40);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseIndexError,    41);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseEndBitError,   42);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseCrcError,      43);
            R_DEFINE_ERROR_RESULT(AutoCommandResponseTimeoutError,  44);
            R_DEFINE_ERROR_RESULT(CommandCompleteSoftwareTimeout,   45);
            R_DEFINE_ERROR_RESULT(TransferCompleteSoftwareTimeout,  46);
        R_DEFINE_ERROR_RANGE(DeviceStatusHasError, 48, 70);
            R_DEFINE_ERROR_RESULT(DeviceStatusAddressOutOfRange, 49);
            R_DEFINE_ERROR_RESULT(DeviceStatusAddressMisaligned, 50);
            R_DEFINE_ERROR_RESULT(DeviceStatusBlockLenError,     51);
            R_DEFINE_ERROR_RESULT(DeviceStatusEraseSeqError,     52);
            R_DEFINE_ERROR_RESULT(DeviceStatusEraseParam,        53);
            R_DEFINE_ERROR_RESULT(DeviceStatusWpViolation,       54);
            R_DEFINE_ERROR_RESULT(DeviceStatusLockUnlockFailed,  55);
            R_DEFINE_ERROR_RESULT(DeviceStatusComCrcError,       56);
            R_DEFINE_ERROR_RESULT(DeviceStatusIllegalCommand,    57);
            R_DEFINE_ERROR_RESULT(DeviceStatusDeviceEccFailed,   58);
            R_DEFINE_ERROR_RESULT(DeviceStatusCcError,           59);
            R_DEFINE_ERROR_RESULT(DeviceStatusError,             60);
            R_DEFINE_ERROR_RESULT(DeviceStatusCidCsdOverwrite,   61);
            R_DEFINE_ERROR_RESULT(DeviceStatusWpEraseSkip,       62);
            R_DEFINE_ERROR_RESULT(DeviceStatusEraseReset,        63);
            R_DEFINE_ERROR_RESULT(DeviceStatusSwitchError,       64);
        R_DEFINE_ERROR_RESULT(UnexpectedDeviceState,                72);
        R_DEFINE_ERROR_RESULT(UnexpectedDeviceCsdValue,             73);
        R_DEFINE_ERROR_RESULT(AbortTransactionSoftwareTimeout,      74);
        R_DEFINE_ERROR_RESULT(CommandInhibitCmdSoftwareTimeout,     75);
        R_DEFINE_ERROR_RESULT(CommandInhibitDatSoftwareTimeout,     76);
        R_DEFINE_ERROR_RESULT(BusySoftwareTimeout,                  77);
        R_DEFINE_ERROR_RESULT(IssueTuningCommandSoftwareTimeout,    78);
        R_DEFINE_ERROR_RESULT(TuningFailed,                         79);
        R_DEFINE_ERROR_RESULT(MmcInitializationSoftwareTimeout,     80);
        R_DEFINE_ERROR_RESULT(MmcNotSupportExtendedCsd,             81);
        R_DEFINE_ERROR_RESULT(UnexpectedMmcExtendedCsdValue,        82);
        R_DEFINE_ERROR_RESULT(MmcEraseSoftwareTimeout,              83);
        R_DEFINE_ERROR_RESULT(SdCardValidationError,                84);
        R_DEFINE_ERROR_RESULT(SdCardInitializationSoftwareTimeout,  85);
        R_DEFINE_ERROR_RESULT(SdCardGetValidRcaSoftwareTimeout,     86);
        R_DEFINE_ERROR_RESULT(UnexpectedSdCardAcmdDisabled,         87);
        R_DEFINE_ERROR_RESULT(SdCardNotSupportSwitchFunctionStatus, 88);
        R_DEFINE_ERROR_RESULT(UnexpectedSdCardSwitchFunctionStatus, 89);
        R_DEFINE_ERROR_RESULT(SdCardNotSupportAccessMode,           90);
        R_DEFINE_ERROR_RESULT(SdCardNot4BitBusWidthAtUhsIMode,      91);
        R_DEFINE_ERROR_RESULT(SdCardNotSupportSdr104AndSdr50,       92);
        R_DEFINE_ERROR_RESULT(SdCardCannotSwitchAccessMode,         93);
        R_DEFINE_ERROR_RESULT(SdCardFailedSwitchAccessMode,         94);
        R_DEFINE_ERROR_RESULT(SdCardUnacceptableCurrentConsumption, 95);
        R_DEFINE_ERROR_RESULT(SdCardNotReadyToVoltageSwitch,        96);
        R_DEFINE_ERROR_RESULT(SdCardNotCompleteVoltageSwitch,       97);

    R_DEFINE_ERROR_RANGE(HostControllerUnexpected, 128, 158);
        R_DEFINE_ERROR_RESULT(InternalClockStableSoftwareTimeout,       129);
        R_DEFINE_ERROR_RESULT(SdHostStandardUnknownAutoCmdError,        130);
        R_DEFINE_ERROR_RESULT(SdHostStandardUnknownError,               131);
        R_DEFINE_ERROR_RESULT(SdmmcDllCalibrationSoftwareTimeout,       132);
        R_DEFINE_ERROR_RESULT(SdmmcDllApplicationSoftwareTimeout,       133);
        R_DEFINE_ERROR_RESULT(SdHostStandardFailSwitchTo1_8V,           134);
        R_DEFINE_ERROR_RESULT(DriveStrengthCalibrationNotCompleted,     135);
        R_DEFINE_ERROR_RESULT(DriveStrengthCalibrationSoftwareTimeout,  136);
        R_DEFINE_ERROR_RESULT(SdmmcCompShortToGnd,                      137);
        R_DEFINE_ERROR_RESULT(SdmmcCompOpen,                            138);

    R_DEFINE_ERROR_RANGE(InternalError, 160, 190);
        R_DEFINE_ERROR_RESULT(NoWaitedInterrupt,            161);
        R_DEFINE_ERROR_RESULT(WaitInterruptSoftwareTimeout, 162);

    R_DEFINE_ERROR_RESULT(NotImplemented, 201);
}
