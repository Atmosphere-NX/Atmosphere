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
#include "sdmmc_base_device_accessor.hpp"

namespace ams::sdmmc::impl {

    #if defined(AMS_SDMMC_THREAD_SAFE)

        #define AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX() std::scoped_lock lk(this->base_device->device_mutex)

    #else

        #define AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX()

    #endif

    void BaseDevice::GetLegacyCapacityParameters(u8 *out_c_size_mult, u8 *out_read_bl_len) const {
        AMS_ABORT_UNLESS(out_c_size_mult  != nullptr);
        AMS_ABORT_UNLESS(out_read_bl_len != nullptr);

        /* Extract C_SIZE_MULT and READ_BL_LEN from the CSD. */
        *out_c_size_mult = static_cast<u8>((this->csd[2] >> 7) & 0x7);
        *out_read_bl_len = static_cast<u8>((this->csd[4] >> 8) & 0xF);
    }

    Result BaseDevice::SetLegacyMemoryCapacity() {
        /* Get csize from the csd. */
        const u32 c_size = ((this->csd[3] >> 6) & 0x3FF) | ((this->csd[4] & 0x3) << 10);

        /* Get c_size_mult and read_bl_len. */
        u8 c_size_mult, read_bl_len;
        this->GetLegacyCapacityParameters(std::addressof(c_size_mult), std::addressof(read_bl_len));

        /* Validate the parameters. */
        R_UNLESS((read_bl_len + c_size_mult + 2) >= 9, sdmmc::ResultUnexpectedDeviceCsdValue());

        /* Set memory capacity. */
        this->memory_capacity          = (c_size + 1) << ((read_bl_len + c_size_mult + 2) - 9);
        this->is_valid_memory_capacity = true;

        return ResultSuccess();
    }

    Result BaseDevice::CheckDeviceStatus(u32 r1_resp) const {
        /* Check if there are any errors at all. */
        R_SUCCEED_IF((r1_resp & DeviceStatus_ErrorMask) == 0);

        /* Check errors individually. */
        #define AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(__ERROR__) R_UNLESS((r1_resp & DeviceStatus_##__ERROR__) == 0, sdmmc::ResultDeviceStatus##__ERROR__())

        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(ComCrcError);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(DeviceEccFailed);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(CcError);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(Error);

        if (this->GetDeviceType() == DeviceType_Mmc) {
            AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(SwitchError);
        }

        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(AddressMisaligned);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(BlockLenError);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(EraseSeqError);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(EraseParam);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(WpViolation);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(LockUnlockFailed);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(CidCsdOverwrite);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(WpEraseSkip);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(WpEraseSkip);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(EraseReset);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(IllegalCommand);
        AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR(AddressOutOfRange);

        #undef AMS_SDMMC_CHECK_DEVICE_STATUS_ERROR

        return ResultSuccess();
    }

    DeviceState BaseDevice::GetDeviceState(u32 r1_resp) const {
        return static_cast<DeviceState>((r1_resp & DeviceStatus_CurrentStateMask) >> DeviceStatus_CurrentStateShift);
    }

    Result BaseDeviceAccessor::IssueCommandAndCheckR1(u32 *out_response, u32 command_index, u32 command_arg, bool is_busy, DeviceState expected_state, u32 status_ignore_mask) const {
        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(command_index, command_arg, CommandResponseType, is_busy);
        R_TRY(this->host_controller->IssueCommand(std::addressof(command)));

        /* Get the response. */
        AMS_ABORT_UNLESS(out_response != nullptr);
        this->host_controller->GetLastResponse(out_response, sizeof(u32), CommandResponseType);

        /* Mask out the ignored status bits. */
        if (status_ignore_mask != 0) {
            *out_response &= ~status_ignore_mask;
        }

        /* Check the r1 response for errors. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        R_TRY(this->base_device->CheckDeviceStatus(*out_response));

        /* Check the device state. */
        if (expected_state != DeviceState_Unknown) {
            R_UNLESS(this->base_device->GetDeviceState(*out_response) == expected_state, sdmmc::ResultUnexpectedDeviceState());
        }

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::IssueCommandGoIdleState() const {
        /* Issue the command. */
        Command command(CommandIndex_GoIdleState, 0, ResponseType_R0, false);
        return this->host_controller->IssueCommand(std::addressof(command));
    }

    Result BaseDeviceAccessor::IssueCommandAllSendCid(void *dst, size_t dst_size) const {
        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R2;
        Command command(CommandIndex_AllSendCid, 0, CommandResponseType, false);
        R_TRY(this->host_controller->IssueCommand(std::addressof(command)));

        /* Copy the data out. */
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(util::IsAligned(reinterpret_cast<uintptr_t>(dst), alignof(u32)));
        AMS_ABORT_UNLESS(dst_size >= DeviceCidSize);
        this->host_controller->GetLastResponse(static_cast<u32 *>(dst), DeviceCidSize, CommandResponseType);

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::IssueCommandSelectCard() const {
        /* Get the command argument. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        const u32 arg = static_cast<u32>(this->base_device->GetRca()) << 16;

        /* Issue the command. */
        return this->IssueCommandAndCheckR1(CommandIndex_SelectCard, arg, true, DeviceState_Unknown);
    }

    Result BaseDeviceAccessor::IssueCommandSendCsd(void *dst, size_t dst_size) const {
        /* Get the command argument. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        const u32 arg = static_cast<u32>(this->base_device->GetRca()) << 16;

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R2;
        Command command(CommandIndex_SendCsd, arg, CommandResponseType, false);
        R_TRY(this->host_controller->IssueCommand(std::addressof(command)));

        /* Copy the data out. */
        AMS_ABORT_UNLESS(dst != nullptr);
        AMS_ABORT_UNLESS(util::IsAligned(reinterpret_cast<uintptr_t>(dst), alignof(u32)));
        AMS_ABORT_UNLESS(dst_size >= DeviceCsdSize);
        this->host_controller->GetLastResponse(static_cast<u32 *>(dst), DeviceCsdSize, CommandResponseType);

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::IssueCommandSendStatus(u32 *out_device_status, u32 status_ignore_mask) const {
        /* Get the command argument. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        const u32 arg = static_cast<u32>(this->base_device->GetRca()) << 16;

        /* Issue the command. */
        return this->IssueCommandAndCheckR1(out_device_status, CommandIndex_SendStatus, arg, false, DeviceState_Tran, status_ignore_mask);
    }

    Result BaseDeviceAccessor::IssueCommandSetBlockLenToSectorSize() const {
        /* Issue the command. */
        return this->IssueCommandAndCheckR1(CommandIndex_SetBlockLen, SectorSize, false, DeviceState_Tran);
    }

    Result BaseDeviceAccessor::IssueCommandMultipleBlock(u32 *out_num_transferred_blocks, u32 sector_index, u32 num_sectors, void *buf, bool is_read) const {
        /* Get the argument. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        const u32 arg = this->base_device->IsHighCapacity() ? sector_index : sector_index * SectorSize;

        /* Get the command index and transfer direction. */
        const u32 command_index   = is_read ? CommandIndex_ReadMultipleBlock   : CommandIndex_WriteMultipleBlock;
        const auto xfer_direction = is_read ? TransferDirection_ReadFromDevice : TransferDirection_WriteToDevice;

        /* Issue the command. */
        constexpr ResponseType CommandResponseType = ResponseType_R1;
        Command command(command_index, arg, CommandResponseType, false);
        TransferData xfer_data(buf, SectorSize, num_sectors, xfer_direction, true, true);
        Result result = this->host_controller->IssueCommand(std::addressof(command), std::addressof(xfer_data), out_num_transferred_blocks);

        /* Handle the failure case. */
        if (R_FAILED(result)) {
            /* Check if we were removed. */
            R_TRY(this->CheckRemoved());

            /* By default, we'll want to return the result we just got. */
            Result result_to_return = result;

            /* Stop transmission. */
            u32 resp = 0;
            result = this->host_controller->IssueStopTransmissionCommand(std::addressof(resp));
            if (R_SUCCEEDED(result)) {
                result = this->base_device->CheckDeviceStatus(resp & (~DeviceStatus_IllegalCommand));
                if (R_FAILED(result)) {
                    result_to_return = result;
                }
            }

            /* Check if we were removed. */
            R_TRY(this->CheckRemoved());

            /* Get the device status. */
            u32 device_status = 0;
            result = this->IssueCommandSendStatus(std::addressof(device_status), DeviceStatus_IllegalCommand);

            /* If there's a device status error we don't already have, we prefer to return it. */
            if (!sdmmc::ResultDeviceStatusHasError::Includes(result_to_return) && sdmmc::ResultDeviceStatusHasError::Includes(result)) {
                result_to_return = result;
            }

            /* Return the result we chose. */
            return result_to_return;
        }

        /* Get the responses. */
        u32 resp, st_resp;
        this->host_controller->GetLastResponse(std::addressof(resp), sizeof(resp), CommandResponseType);
        this->host_controller->GetLastStopTransmissionResponse(std::addressof(st_resp), sizeof(st_resp));

        /* Check the device status. */
        R_TRY(this->base_device->CheckDeviceStatus(resp));

        /* Decide on what errors to ignore. */
        u32 status_ignore_mask = 0;
        if (is_read) {
            AMS_ABORT_UNLESS(out_num_transferred_blocks != nullptr);
            if ((*out_num_transferred_blocks + sector_index) == this->base_device->GetMemoryCapacity()) {
                status_ignore_mask = DeviceStatus_AddressOutOfRange;
            }
        }

        /* Check the device status. */
        R_TRY(this->base_device->CheckDeviceStatus(st_resp & ~status_ignore_mask));

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::ReadWriteSingle(u32 *out_num_transferred_blocks, u32 sector_index, u32 num_sectors, void *buf, bool is_read) const {
        /* Issue the read/write command. */
        AMS_ABORT_UNLESS(out_num_transferred_blocks != nullptr);
        R_TRY(this->IssueCommandMultipleBlock(out_num_transferred_blocks, sector_index, num_sectors, buf, is_read));

        /* Decide on what errors to ignore. */
        u32 status_ignore_mask = 0;
        if (is_read) {
            AMS_ABORT_UNLESS(this->base_device != nullptr);
            if ((*out_num_transferred_blocks + sector_index) == this->base_device->GetMemoryCapacity()) {
                status_ignore_mask = DeviceStatus_AddressOutOfRange;
            }
        }

        /* Get and check the status. */
        u32 device_status;
        R_TRY(this->IssueCommandSendStatus(std::addressof(device_status), status_ignore_mask));

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::ReadWriteMultiple(u32 sector_index, u32 num_sectors, u32 sector_index_alignment, void *buf, size_t buf_size, bool is_read) {
        /* Verify that we can send the command. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);

        /* If we want to read zero sectors, there's no work for us to do. */
        R_SUCCEED_IF(num_sectors == 0);

        /* Check that the buffer is big enough for the sectors we're reading. */
        AMS_ABORT_UNLESS((buf_size / SectorSize) >= num_sectors);

        /* Read sectors repeatedly until we've read all the ones we want. */
        u32 cur_sector_index  = sector_index;
        u32 remaining_sectors = num_sectors;
        u8 *cur_buf           = static_cast<u8 *>(buf);
        while (remaining_sectors > 0) {
            /* Determine how many sectors we can read in this iteration. */
            u32 cur_sectors = remaining_sectors;
            if (sector_index_alignment > 0) {
                AMS_ABORT_UNLESS((cur_sector_index % sector_index_alignment) == 0);

                const u32 max_sectors = this->host_controller->GetMaxTransferNumBlocks();
                if (remaining_sectors > max_sectors) {
                    cur_sectors = max_sectors - (max_sectors % sector_index_alignment);
                }
            }

            /* Try to perform the read/write. */
            u32 num_transferred_blocks = 0;
            Result result = this->ReadWriteSingle(std::addressof(num_transferred_blocks), cur_sector_index, cur_sectors, cur_buf, is_read);
            if (R_FAILED(result)) {
                /* Check if we were removed. */
                R_TRY(this->CheckRemoved());

                /* Log that we failed to read/write. */
                this->PushErrorLog(false, "%s %X %X:%X", is_read ? "R" : "W", cur_sector_index, cur_sectors, result.GetValue());

                /* Retry the read/write. */
                num_transferred_blocks = 0;
                result = this->ReadWriteSingle(std::addressof(num_transferred_blocks), cur_sector_index, cur_sectors, cur_buf, is_read);
                if (R_FAILED(result)) {
                    /* Check if we were removed. */
                    R_TRY(this->CheckRemoved());

                    /* Log that we failed to read/write. */
                    this->PushErrorLog(false, "%s %X %X:%X", is_read ? "R" : "W", cur_sector_index, cur_sectors, result.GetValue());

                    /* Re-startup the connection, to see if that helps. */
                    R_TRY(this->ReStartup());

                    /* Retry the read/write a third time. */
                    num_transferred_blocks = 0;
                    result = this->ReadWriteSingle(std::addressof(num_transferred_blocks), cur_sector_index, cur_sectors, cur_buf, is_read);
                    if (R_FAILED(result)) {
                        /* Log that we failed after a re-startup. */
                        this->PushErrorLog(true, "%s %X %X:%X", is_read ? "R" : "W", cur_sector_index, cur_sectors, result.GetValue());
                        return result;
                    }

                    /* Log that we succeeded after a retry. */
                    this->PushErrorLog(true, "%s %X %X:0", is_read ? "R" : "W", cur_sector_index, cur_sectors);

                    /* Increment the number of error corrections we've done. */
                    ++this->num_read_write_error_corrections;
                }
            }

            /* Update our tracking variables. */
            AMS_ABORT_UNLESS(remaining_sectors >= num_transferred_blocks);
            remaining_sectors -= num_transferred_blocks;
            cur_sector_index  += num_transferred_blocks;
            cur_buf           += num_transferred_blocks * SectorSize;
        }

        return ResultSuccess();
    }

    #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
    void BaseDeviceAccessor::RegisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Register the address. */
        return this->host_controller->RegisterDeviceVirtualAddress(buffer, buffer_size, buffer_device_virtual_address);
    }

    void BaseDeviceAccessor::UnregisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Register the address. */
        return this->host_controller->UnregisterDeviceVirtualAddress(buffer, buffer_size, buffer_device_virtual_address);
    }
    #endif

    Result BaseDeviceAccessor::Activate() {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is awake. */
        R_UNLESS(this->base_device->IsAwake(), sdmmc::ResultNotAwakened());

        /* If the device is already active, we don't need to do anything. */
        R_SUCCEED_IF(this->base_device->IsActive());

        /* Activate the base device. */
        auto activate_guard = SCOPE_GUARD { ++this->num_activation_failures; };
        R_TRY(this->OnActivate());

        /* We successfully activated the device. */
        activate_guard.Cancel();
        this->base_device->SetActive();

        return ResultSuccess();
    }

    void BaseDeviceAccessor::Deactivate() {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Deactivate the base device. */
        if (this->base_device->IsActive()) {
            this->host_controller->Shutdown();
            this->base_device->Deactivate();
        }
    }

    Result BaseDeviceAccessor::ReadWrite(u32 sector_index, u32 num_sectors, void *buffer, size_t buffer_size, bool is_read) {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Perform the read/write. */
        auto rw_guard = SCOPE_GUARD { ++this->num_read_write_failures; };
        R_TRY(this->OnReadWrite(sector_index, num_sectors, buffer, buffer_size, is_read));

        /* We successfully performed the read/write. */
        rw_guard.Cancel();
        return ResultSuccess();
    }

    Result BaseDeviceAccessor::CheckConnection(SpeedMode *out_speed_mode, BusWidth *out_bus_width) {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Get the current speed mode/bus width. */
        *out_speed_mode = this->host_controller->GetSpeedMode();
        *out_bus_width  = this->host_controller->GetBusWidth();

        /* Verify that we can get the status. */
        R_TRY(this->host_controller->GetInternalStatus());

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::GetMemoryCapacity(u32 *out_sectors) const {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Get the capacity. */
        AMS_ABORT_UNLESS(out_sectors != nullptr);
        *out_sectors = this->base_device->GetMemoryCapacity();

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::GetDeviceStatus(u32 *out) const {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Get the status. */
        R_TRY(this->IssueCommandSendStatus(out, 0));

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::GetOcr(u32 *out) const {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Get the ocr. */
        AMS_ABORT_UNLESS(out != nullptr);
        *out = this->base_device->GetOcr();

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::GetRca(u16 *out) const {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Get the rca. */
        AMS_ABORT_UNLESS(out != nullptr);
        *out = this->base_device->GetRca();

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::GetCid(void *out, size_t size) const {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Get the cid. */
        this->base_device->GetCid(out, size);

        return ResultSuccess();
    }

    Result BaseDeviceAccessor::GetCsd(void *out, size_t size) const {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Check that the device is accessible. */
        R_TRY(this->base_device->CheckAccessible());

        /* Get the csd. */
        this->base_device->GetCsd(out, size);

        return ResultSuccess();
    }

    void BaseDeviceAccessor::GetAndClearErrorInfo(ErrorInfo *out_error_info, size_t *out_log_size, char *out_log_buffer, size_t log_buffer_size) {
        /* Lock exclusive access of the base device. */
        AMS_ABORT_UNLESS(this->base_device != nullptr);
        AMS_SDMMC_LOCK_BASE_DEVICE_MUTEX();

        /* Set the output error info. */
        AMS_ABORT_UNLESS(out_error_info != nullptr);
        out_error_info->num_activation_failures           = this->num_activation_failures;
        out_error_info->num_activation_error_corrections  = this->num_activation_error_corrections;
        out_error_info->num_read_write_failures           = this->num_read_write_failures;
        out_error_info->num_read_write_error_corrections  = this->num_read_write_error_corrections;
        this->ClearErrorInfo();

        /* Check if we should write logs. */
        if (out_log_size == nullptr) {
            return;
        }

        /* Check if we can write logs. */
        if (out_log_buffer == nullptr || log_buffer_size == 0) {
            *out_log_size = 0;
            return;
        }

        /* Get and clear our logs. */
        #if defined(AMS_SDMMC_USE_LOGGER)
        {
            if (this->error_logger.HasLog()) {
                this->PushErrorTimeStamp();

                *out_log_size = this->error_logger.GetAndClearLogs(out_log_buffer, log_buffer_size);
            } else {
                *out_log_size = 0;
            }
        }
        #else
        {
            *out_log_size = 0;
        }
        #endif
    }

}
