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
#if defined(ATMOSPHERE_IS_STRATOSPHERE)
#include <stratosphere.hpp>
#elif defined(ATMOSPHERE_IS_MESOSPHERE)
#include <mesosphere.hpp>
#elif defined(ATMOSPHERE_IS_EXOSPHERE)
#include <exosphere.hpp>
#else
#include <vapours.hpp>
#endif
#include "sdmmc_gc_asic_device_accessor.hpp"
#include "sdmmc_timer.hpp"

namespace ams::sdmmc::impl {

    #if defined(AMS_SDMMC_THREAD_SAFE)

        #define AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX() std::scoped_lock lk(this->gc_asic_device.device_mutex)

    #else

        #define AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX()

    #endif

    #if defined(AMS_SDMMC_USE_OS_EVENTS)

        #define AMS_SDMMC_CHECK_GC_ASIC_REMOVED() R_UNLESS(!this->gc_asic_device.IsRemoved(), sdmmc::ResultDeviceRemoved())

    #else

        #define AMS_SDMMC_CHECK_GC_ASIC_REMOVED()

    #endif

    Result GcAsicDeviceAccessor::IssueCommandWriteOperation(const void *op_buf, size_t op_buf_size) const {
        /* Validate the operation buffer. */
        AMS_ABORT_UNLESS(op_buf != nullptr);
        AMS_ABORT_UNLESS(op_buf_size >= GcAsicOperationSize);

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(CommandIndex_GcAsicWriteOperation, 0, CommandResponseType, false);
        TransferData xfer_data(const_cast<void *>(op_buf), GcAsicOperationSize, 1, TransferDirection_WriteToDevice);
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        Result result = hc->IssueCommand(std::addressof(command), std::addressof(xfer_data));
        if (R_FAILED(result)) {
            /* We failed to write operation. Check if we were removed. */
            AMS_SDMMC_CHECK_GC_ASIC_REMOVED();

            /* Determine what result we should return. */
            Result return_result = result;
            {
                /* Issue a stop transmission command. */
                u32 resp = 0;
                result = hc->IssueStopTransmissionCommand(std::addressof(resp));
                if (R_SUCCEEDED(result)) {
                    /* If we successfully stopped transmission but have an error status, we prefer to return that. */
                    result = this->gc_asic_device.CheckDeviceStatus(resp);
                    if (R_FAILED(result)) {
                        return_result = result;
                    }
                }

                /* Check again if we were removed. */
                AMS_SDMMC_CHECK_GC_ASIC_REMOVED();

                /* Request device status. */
                u32 device_status;
                result = BaseDeviceAccessor::IssueCommandSendStatus(std::addressof(device_status), 0);

                /* If we got a device status error here and we didn't previously, we prefer to return that. */
                if (!sdmmc::ResultDeviceStatusHasError::Includes(return_result) && sdmmc::ResultDeviceStatusHasError::Includes(result)) {
                    return_result = result;
                }
            }
            return return_result;
        }

        /* Get the response. */
        u32 resp;
        hc->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);
        R_TRY(this->gc_asic_device.CheckDeviceStatus(resp));

        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::IssueCommandFinishOperation() const {
        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_GcAsicFinishOperation, 0, true, DeviceState_Tran));
        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::IssueCommandSleep() {
        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_GcAsicSleep, 0, true, DeviceState_Tran));
        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::IssueCommandUpdateKey() const {
        /* Issue the command. */
        R_TRY(BaseDeviceAccessor::IssueCommandAndCheckR1(CommandIndex_GcAsicUpdateKey, 0, true, DeviceState_Tran));
        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::StartupGcAsicDevice() {
        /* Start up the host controller. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        R_TRY(hc->Startup(BusPower_1_8V, BusWidth_8Bit, SpeedMode_GcAsicSpeed, false));

        /* Wait 10 clocks for configuration to take. */
        WaitClocks(10, hc->GetDeviceClockFrequencyKHz());

        /* Perform tuning with command index 21. */
        AMS_ABORT_UNLESS(hc->IsSupportedTuning());
        R_TRY(hc->Tuning(SpeedMode_GcAsicSpeed, 21));

        /* Set the device as low capacity/no memory. */
        this->gc_asic_device.SetHighCapacity(false);
        this->gc_asic_device.SetMemoryCapacity(0);

        /* Enable power saving. */
        hc->SetPowerSaving(true);

        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::OnActivate() {
        /* If we fail to start up the device, ensure the host controller is shut down. */
        auto power_guard = SCOPE_GUARD { BaseDeviceAccessor::GetHostController()->Shutdown(); };

        /* Try to start up the device. */
        R_TRY(this->StartupGcAsicDevice());

        /* We started up, so we don't need to power down. */
        power_guard.Cancel();
        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::OnReadWrite(u32 sector_index, u32 num_sectors, void *buf, size_t buf_size, bool is_read) {
        /* Check that we're not performing zero-byte rw. */
        AMS_ABORT_UNLESS(num_sectors > 0);

        /* Check that the buffer is big enough for the rw. */
        AMS_ABORT_UNLESS((buf_size / SectorSize) >= num_sectors);

        /* Perform the read/write. */
        u32 num_transferred_blocks;
        R_TRY(BaseDeviceAccessor::ReadWriteSingle(std::addressof(num_transferred_blocks), sector_index, num_sectors, buf, is_read));

        /* Require that we read/wrote as many sectors as we expected. */
        AMS_ABORT_UNLESS(num_transferred_blocks == num_sectors);

        return ResultSuccess();
    }

    void GcAsicDeviceAccessor::Initialize() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* If we've already initialized, we don't need to do anything. */
        if (this->is_initialized) {
            return;
        }

        /* Set the base device to our gc asic device. */
        BaseDeviceAccessor::SetDevice(std::addressof(this->gc_asic_device));

        /* Initialize. */
        IHostController *hc = BaseDeviceAccessor::GetHostController();
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            this->gc_asic_device.InitializeRemovedEvent();
            hc->PreSetRemovedEvent(this->gc_asic_device.GetRemovedEvent());
        }
        #endif
        hc->Initialize();

        /* Mark ourselves as initialized. */
        this->is_initialized = true;
    }

    void GcAsicDeviceAccessor::Finalize() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* If we've already finalized, we don't need to do anything. */
        if (!this->is_initialized) {
            return;
        }
        this->is_initialized = false;

        /* Deactivate the device. */
        BaseDeviceAccessor::Deactivate();

        /* Finalize the host controller. */
        BaseDeviceAccessor::GetHostController()->Finalize();

        /* Finalize the removed event. */
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            this->gc_asic_device.FinalizeRemovedEvent();
        }
        #endif
    }

    Result GcAsicDeviceAccessor::GetSpeedMode(SpeedMode *out_speed_mode) const {
        /* Check that we can write to output. */
        AMS_ABORT_UNLESS(out_speed_mode != nullptr);

        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->gc_asic_device.CheckAccessible());

        *out_speed_mode = SpeedMode_GcAsicSpeed;
        return ResultSuccess();
    }

    void GcAsicDeviceAccessor::PutGcAsicToSleep() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* If the device isn't awake, we don't need to do anything. */
        if (!this->gc_asic_device.IsAwake()) {
            return;
        }

        /* If necessary, put the host controller to sleep. */
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        if (this->gc_asic_device.IsActive() && !this->gc_asic_device.IsRemoved())
        #else
        if (this->gc_asic_device.IsActive())
        #endif
        {
            BaseDeviceAccessor::GetHostController()->PutToSleep();
        }

        /* Put the gc asic device to sleep. */
        this->gc_asic_device.PutToSleep();
    }

    Result GcAsicDeviceAccessor::AwakenGcAsic() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* If the device is awake, we don't need to do anything. */
        R_SUCCEED_IF(this->gc_asic_device.IsAwake());

        /* Wake the device. */
        this->gc_asic_device.Awaken();

        /* Wake the host controller, if we need to.*/
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        if (this->gc_asic_device.IsActive() && !this->gc_asic_device.IsRemoved())
        #else
        if (this->gc_asic_device.IsActive())
        #endif
        {
            R_TRY(BaseDeviceAccessor::GetHostController()->Awaken());
        }

        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::WriteGcAsicOperation(const void *op_buf, size_t op_buf_size) {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->gc_asic_device.CheckAccessible());

        /* Issue the command. */
        R_TRY(this->IssueCommandWriteOperation(op_buf, op_buf_size));
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::FinishGcAsicOperation() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->gc_asic_device.CheckAccessible());

        /* Issue the command. */
        R_TRY(this->IssueCommandFinishOperation());
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::AbortGcAsicOperation() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->gc_asic_device.CheckAccessible());

        /* Issue stop transmission command. */
        u32 resp = 0;
        R_TRY(BaseDeviceAccessor::GetHostController()->IssueStopTransmissionCommand(std::addressof(resp)));
        R_TRY(this->gc_asic_device.CheckDeviceStatus(resp));

        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::SleepGcAsic() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->gc_asic_device.CheckAccessible());

        /* Issue the command. */
        R_TRY(this->IssueCommandSleep());
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        return ResultSuccess();
    }

    Result GcAsicDeviceAccessor::UpdateGcAsicKey() {
        /* Acquire exclusive access to the device. */
        AMS_SDMMC_LOCK_GC_ASIC_DEVICE_MUTEX();

        /* Check that we're accessible. */
        R_TRY(this->gc_asic_device.CheckAccessible());

        /* Issue the command. */
        R_TRY(this->IssueCommandUpdateKey());
        R_TRY(BaseDeviceAccessor::IssueCommandSendStatus());

        return ResultSuccess();
    }

}
