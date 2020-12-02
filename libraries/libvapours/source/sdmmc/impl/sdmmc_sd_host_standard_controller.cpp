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
#include "sdmmc_sd_host_standard_controller.hpp"
#include "sdmmc_timer.hpp"

#if defined(ATMOSPHERE_IS_STRATOSPHERE)
    #include <stratosphere/dd.hpp>
#endif


namespace ams::sdmmc::impl {

    namespace {

        constexpr inline u32 ControllerReactionTimeoutMilliSeconds    = 2000;
        constexpr inline u32 CommandTimeoutMilliSeconds               = 2000;
        constexpr inline u32 DefaultCheckTransferIntervalMilliSeconds = 1500;
        constexpr inline u32 BusyTimeoutMilliSeconds                  = 2000;

    }

    #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
    void SdHostStandardController::ResetBufferInfos() {
        for (auto &info : this->buffer_infos) {
            info.buffer_address = 0;
            info.buffer_size    = 0;
        }
    }

    dd::DeviceVirtualAddress SdHostStandardController::GetDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size) {
        /* Try to find the buffer in our registered regions. */
        dd::DeviceVirtualAddress device_addr = 0;
        for (const auto &info : this->buffer_infos) {
            if (info.buffer_address <= buffer && (buffer + buffer_size) <= (info.buffer_address + info.buffer_size)) {
                device_addr = info.buffer_device_virtual_address + (buffer - info.buffer_address);
                break;
            }
        }

        /* Ensure that we found the buffer. */
        AMS_ABORT_UNLESS(device_addr != 0);
        return device_addr;
    }
    #endif

    void SdHostStandardController::EnsureControl() {
        /* Perform a read of clock control to be sure previous configuration takes. */
        reg::Read(this->registers->clock_control);
    }

    Result SdHostStandardController::EnableInternalClock() {
        /* Enable internal clock. */
        reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_INTERNAL_CLOCK_ENABLE, OSCILLATE));
        this->EnsureControl();

        /* Wait for the internal clock to become stable. */
        {
            ManualTimer timer(ControllerReactionTimeoutMilliSeconds);
            while (true) {
                /* Check if the clock is steady. */
                if (reg::HasValue(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_INTERNAL_CLOCK_STABLE, READY))) {
                    break;
                }

                /* If not, check for timeout. */
                R_UNLESS(timer.Update(), sdmmc::ResultInternalClockStableSoftwareTimeout());
            }
        }

        /* Configure to use host controlled divided clock. */
        reg::ReadWrite(this->registers->host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_PRESET_VALUE_ENABLE, HOST_DRIVER));
        reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_CLOCK_GENERATOR_SELECT, DIVIDED_CLOCK));

        /* Set host version 4.0.0 enable. */
        reg::ReadWrite(this->registers->host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_HOST_VERSION_4_ENABLE, VERSION_4));

        /* Set host 64 bit addressing enable. */
        AMS_ABORT_UNLESS(reg::HasValue(this->registers->capabilities, SD_REG_BITS_ENUM(CAPABILITIES_64_BIT_SYSTEM_ADDRESS_SUPPORT_FOR_V3, SUPPORTED)));
        reg::ReadWrite(this->registers->host_control2, SD_REG_BITS_ENUM(HOST_CONTROL2_64_BIT_ADDRESSING, 64_BIT_ADDRESSING));

        /* Select SDMA mode. */
        reg::ReadWrite(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_DMA_SELECT, SDMA));

        /* Configure timeout control to use the maximum timeout value (TMCLK * 2^27) */
        reg::ReadWrite(this->registers->timeout_control, SD_REG_BITS_VALUE(TIMEOUT_CONTROL_DATA_TIMEOUT_COUNTER, 0b1110));

        return ResultSuccess();
    }

    void SdHostStandardController::SetBusPower(BusPower bus_power) {
        /* Check that we support the bus power. */
        AMS_ABORT_UNLESS(this->IsSupportedBusPower(bus_power));

        /* Set the appropriate power. */
        switch (bus_power) {
            case BusPower_Off:
                reg::ReadWrite(this->registers->power_control, SD_REG_BITS_ENUM(POWER_CONTROL_SD_BUS_POWER_FOR_VDD1, OFF));
                break;
            case BusPower_1_8V:
                reg::ReadWrite(this->registers->power_control, SD_REG_BITS_ENUM(POWER_CONTROL_SD_BUS_VOLTAGE_SELECT_FOR_VDD1, 1_8V));
                reg::ReadWrite(this->registers->power_control, SD_REG_BITS_ENUM(POWER_CONTROL_SD_BUS_POWER_FOR_VDD1, ON));
                break;
            case BusPower_3_3V:
                reg::ReadWrite(this->registers->power_control, SD_REG_BITS_ENUM(POWER_CONTROL_SD_BUS_VOLTAGE_SELECT_FOR_VDD1, 3_3V));
                reg::ReadWrite(this->registers->power_control, SD_REG_BITS_ENUM(POWER_CONTROL_SD_BUS_POWER_FOR_VDD1, ON));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void SdHostStandardController::EnableInterruptStatus() {
        /* Set the status register interrupt enables. */
        reg::ReadWrite(this->registers->normal_int_enable, SD_HOST_STANDARD_NORMAL_INTERRUPT_ENABLE_ISSUE_COMMAND(ENABLED));
        reg::ReadWrite(this->registers->error_int_enable,  SD_HOST_STANDARD_ERROR_INTERRUPT_ENABLE_ISSUE_COMMAND (ENABLED));

        /* Read/write the interrupt enables to be sure they take. */
        reg::Write(this->registers->normal_int_enable, reg::Read(this->registers->normal_int_enable));
        reg::Write(this->registers->error_int_enable,  reg::Read(this->registers->error_int_enable));

        /* If we're using interrupt events, configure appropriately. */
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            /* Clear interrupts. */
            this->ClearInterrupt();

            /* Enable the interrupt signals. */
            reg::ReadWrite(this->registers->normal_signal_enable, SD_HOST_STANDARD_NORMAL_INTERRUPT_ENABLE_ISSUE_COMMAND(ENABLED));
            reg::ReadWrite(this->registers->error_signal_enable,  SD_HOST_STANDARD_ERROR_INTERRUPT_ENABLE_ISSUE_COMMAND (ENABLED));
        }
        #endif
    }

    void SdHostStandardController::DisableInterruptStatus() {
        /* If we're using interrupt events, configure appropriately. */
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            /* Disable the interrupt signals. */
            reg::ReadWrite(this->registers->normal_signal_enable, SD_HOST_STANDARD_NORMAL_INTERRUPT_ENABLE_ISSUE_COMMAND(MASKED));
            reg::ReadWrite(this->registers->error_signal_enable,  SD_HOST_STANDARD_ERROR_INTERRUPT_ENABLE_ISSUE_COMMAND (MASKED));
        }
        #endif

        /* Mask the status register interrupt enables. */
        reg::ReadWrite(this->registers->normal_int_enable, SD_HOST_STANDARD_NORMAL_INTERRUPT_ENABLE_ISSUE_COMMAND(MASKED));
        reg::ReadWrite(this->registers->error_int_enable,  SD_HOST_STANDARD_ERROR_INTERRUPT_ENABLE_ISSUE_COMMAND (MASKED));
    }

    #if defined(AMS_SDMMC_USE_OS_EVENTS)
    Result SdHostStandardController::WaitInterrupt(u32 timeout_ms) {
        /* Ensure that we control the registers. */
        this->EnsureControl();

        /* Wait for the interrupt to be signaled. */
        os::WaitableHolderType *signaled_holder = os::TimedWaitAny(std::addressof(this->waitable_manager), TimeSpan::FromMilliSeconds(timeout_ms));
        if (signaled_holder == std::addressof(this->interrupt_event_holder)) {
            /* We received the interrupt. */
            return ResultSuccess();
        } else if (signaled_holder == std::addressof(this->removed_event_holder)) {
            /* The device was removed. */
            return sdmmc::ResultDeviceRemoved();
        } else {
            /* Timeout occurred. */
            return sdmmc::ResultWaitInterruptSoftwareTimeout();
        }
    }

    void SdHostStandardController::ClearInterrupt() {
        /* Ensure that we control the registers. */
        this->EnsureControl();

        /* Clear the interrupt event. */
        os::ClearInterruptEvent(this->interrupt_event);
    }
    #endif

    void SdHostStandardController::SetTransfer(u32 *out_num_transferred_blocks, const TransferData *xfer_data) {
        /* Ensure the transfer data is valid. */
        AMS_ABORT_UNLESS(xfer_data->block_size != 0);
        AMS_ABORT_UNLESS(xfer_data->num_blocks != 0);

        /* Determine the number of blocks. */
        const u16 num_blocks = std::min<u16>(xfer_data->num_blocks, SdHostStandardRegisters::BlockCountMax);

        /* Determine the address/how many blocks to transfer. */
        #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
        const u64 address         = this->GetDeviceVirtualAddress(reinterpret_cast<uintptr_t>(xfer_data->buffer), xfer_data->block_size * num_blocks);
        const u16 num_xfer_blocks = num_blocks;
        #else
        const u64 address         = reinterpret_cast<uintptr_t>(xfer_data->buffer);
        const u16 num_xfer_blocks = num_blocks;
        #endif

        /* Verify the address is usable. */
        AMS_ABORT_UNLESS(util::IsAligned(address, BufferDeviceVirtualAddressAlignment));

        /* Configure for sdma. */
        reg::Write(this->registers->adma_address,       static_cast<u32>(address >> 0));
        reg::Write(this->registers->upper_adma_address, static_cast<u32>(address >> BITSIZEOF(u32)));

        /* Set our next sdma address. */
        this->next_sdma_address = util::AlignDown<u64>(address + SdmaBufferBoundary, SdmaBufferBoundary);

        /* Configure block size. */
        AMS_ABORT_UNLESS(xfer_data->block_size <= SdHostStandardBlockSizeTransferBlockSizeMax);
        reg::Write(this->registers->block_size, SD_REG_BITS_ENUM (BLOCK_SIZE_SDMA_BUFFER_BOUNDARY, 512_KB),
                                                SD_REG_BITS_VALUE(BLOCK_SIZE_TRANSFER_BLOCK_SIZE,  static_cast<u16>(xfer_data->block_size)));

        /* Configure transfer blocks. */
        reg::Write(this->registers->block_count, num_xfer_blocks);
        if (out_num_transferred_blocks != nullptr) {
            *out_num_transferred_blocks = num_xfer_blocks;
        }

        /* Configure transfer mode. */
        reg::Write(this->registers->transfer_mode, SD_REG_BITS_ENUM    (TRANSFER_MODE_DMA_ENABLE, ENABLE),
                                                   SD_REG_BITS_ENUM_SEL(TRANSFER_MODE_BLOCK_COUNT_ENABLE, (xfer_data->is_multi_block_transfer), ENABLE, DISABLE),
                                                   SD_REG_BITS_ENUM_SEL(TRANSFER_MODE_MULTI_BLOCK_SELECT, (xfer_data->is_multi_block_transfer), MULTI_BLOCK, SINGLE_BLOCK),
                                                   SD_REG_BITS_ENUM_SEL(TRANSFER_MODE_DATA_TRANSFER_DIRECTION, (xfer_data->transfer_direction == TransferDirection_ReadFromDevice), READ, WRITE),
                                                   SD_REG_BITS_ENUM_SEL(TRANSFER_MODE_AUTO_CMD_ENABLE,         (xfer_data->is_stop_transmission_command_enabled), CMD12_ENABLE, DISABLE));
    }

    void SdHostStandardController::SetTransferForTuning() {
        /* Get the tuning block size. */
        u16 tuning_block_size;
        switch (this->GetBusWidth()) {
            case BusWidth_4Bit:
                tuning_block_size = 64;
                break;
            case BusWidth_8Bit:
                tuning_block_size = 128;
                break;
            case BusWidth_1Bit:
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Configure block size. */
        AMS_ABORT_UNLESS(tuning_block_size <= SdHostStandardBlockSizeTransferBlockSizeMax);
        reg::Write(this->registers->block_size, SD_REG_BITS_VALUE(BLOCK_SIZE_TRANSFER_BLOCK_SIZE, tuning_block_size));

        /* Configure transfer blocks. */
        reg::Write(this->registers->block_count, 1);

        /* Configure transfer mode. */
        reg::Write(this->registers->transfer_mode, SD_REG_BITS_ENUM(TRANSFER_MODE_DATA_TRANSFER_DIRECTION, READ));
    }

    void SdHostStandardController::SetCommand(const Command *command, bool has_xfer_data) {
        /* Encode the command value. */
        u16 command_val = 0;

        /* Encode the response type. */
        switch (command->response_type) {
            case ResponseType_R0:
                command_val |= reg::Encode(SD_REG_BITS_ENUM(COMMAND_RESPONSE_TYPE, NO_RESPONSE),
                                           SD_REG_BITS_ENUM(COMMAND_CRC_CHECK,     DISABLE),
                                           SD_REG_BITS_ENUM(COMMAND_INDEX_CHECK,   DISABLE));
                break;
            case ResponseType_R1:
            case ResponseType_R6:
            case ResponseType_R7:
                command_val |= reg::Encode(SD_REG_BITS_ENUM_SEL(COMMAND_RESPONSE_TYPE, command->is_busy, RESPONSE_LENGTH_48_CHECK_BUSY_AFTER_RESPONSE, RESPONSE_LENGTH_48),
                                           SD_REG_BITS_ENUM    (COMMAND_CRC_CHECK,     ENABLE),
                                           SD_REG_BITS_ENUM    (COMMAND_INDEX_CHECK,   ENABLE));
                break;
            case ResponseType_R2:
                command_val |= reg::Encode(SD_REG_BITS_ENUM(COMMAND_RESPONSE_TYPE, RESPONSE_LENGTH_136),
                                           SD_REG_BITS_ENUM(COMMAND_CRC_CHECK,     ENABLE),
                                           SD_REG_BITS_ENUM(COMMAND_INDEX_CHECK,   DISABLE));
                break;
            case ResponseType_R3:
                command_val |= reg::Encode(SD_REG_BITS_ENUM(COMMAND_RESPONSE_TYPE, RESPONSE_LENGTH_48),
                                           SD_REG_BITS_ENUM(COMMAND_CRC_CHECK,     DISABLE),
                                           SD_REG_BITS_ENUM(COMMAND_INDEX_CHECK,   DISABLE));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }

        /* Encode the data present select. */
        command_val |= reg::Encode(SD_REG_BITS_ENUM_SEL(COMMAND_DATA_PRESENT, has_xfer_data, DATA_PRESENT, NO_DATA_PRESENT));

        /* Encode the command index. */
        AMS_ABORT_UNLESS(command->command_index <= SdHostStandardCommandIndexMax);
        command_val |= reg::Encode(SD_REG_BITS_VALUE(COMMAND_COMMAND_INDEX, command->command_index));

        /* Write the command and argument. */
        reg::Write(this->registers->argument, command->command_argument);
        reg::Write(this->registers->command,  command_val);
    }

    void SdHostStandardController::SetCommandForTuning(u32 command_index) {
        Command command(command_index, 0, ResponseType_R1, false);
        return this->SetCommand(std::addressof(command), true);
    }

    Result SdHostStandardController::ResetCmdDatLine() {
        /* Set the software reset cmd/dat bits. */
        reg::ReadWrite(this->registers->software_reset, SD_REG_BITS_ENUM(SOFTWARE_RESET_FOR_CMD, RESET),
                                                        SD_REG_BITS_ENUM(SOFTWARE_RESET_FOR_DAT, RESET));

        /* Ensure that we control the registers. */
        this->EnsureControl();

        /* Wait until reset is done. */
        {
            ManualTimer timer(ControllerReactionTimeoutMilliSeconds);
            while (true) {
                /* Check if the target has been removed. */
                R_TRY(this->CheckRemoved());

                /* Check if command inhibit is no longer present. */
                if (reg::HasValue(this->registers->software_reset, SD_REG_BITS_ENUM(SOFTWARE_RESET_FOR_CMD, WORK),
                                                                   SD_REG_BITS_ENUM(SOFTWARE_RESET_FOR_DAT, WORK)))
                {
                    break;
                }

                /* Otherwise, check if we've timed out. */
                if (!timer.Update()) {
                    return sdmmc::ResultAbortTransactionSoftwareTimeout();
                }
            }
        }

        return ResultSuccess();
    }

    Result SdHostStandardController::AbortTransaction() {
        R_TRY(this->ResetCmdDatLine());
        return ResultSuccess();
    }

    Result SdHostStandardController::WaitWhileCommandInhibit(bool has_dat) {
        /* Ensure that we control the registers. */
        this->EnsureControl();

        /* Wait while command inhibit cmd is set. */
        {
            ManualTimer timer(ControllerReactionTimeoutMilliSeconds);
            while (true) {
                /* Check if the target has been removed. */
                R_TRY(this->CheckRemoved());

                /* Check if command inhibit is no longer present. */
                if (reg::HasValue(this->registers->present_state, SD_REG_BITS_ENUM(PRESENT_STATE_COMMAND_INHIBIT_CMD, READY))) {
                    break;
                }

                /* Otherwise, check if we've timed out. */
                if (!timer.Update()) {
                    this->AbortTransaction();
                    return sdmmc::ResultCommandInhibitCmdSoftwareTimeout();
                }
            }
        }

        /* Wait while command inhibit dat is set. */
        if (has_dat) {
            ManualTimer timer(ControllerReactionTimeoutMilliSeconds);
            while (true) {
                /* Check if the target has been removed. */
                R_TRY(this->CheckRemoved());

                /* Check if command inhibit is no longer present. */
                if (reg::HasValue(this->registers->present_state, SD_REG_BITS_ENUM(PRESENT_STATE_COMMAND_INHIBIT_DAT, READY))) {
                    break;
                }

                /* Otherwise, check if we've timed out. */
                if (!timer.Update()) {
                    this->AbortTransaction();
                    return sdmmc::ResultCommandInhibitDatSoftwareTimeout();
                }
            }
        }

        return ResultSuccess();
    }

    Result SdHostStandardController::CheckAndClearInterruptStatus(volatile u16 *out_normal_int_status, u16 wait_mask) {
        /* Read the statuses. */
        volatile u16 normal_int_status   = reg::Read(this->registers->normal_int_status);
        volatile u16 error_int_status    = reg::Read(this->registers->error_int_status);
        volatile u16 auto_cmd_err_status = reg::Read(this->registers->acmd12_err);

        /* Set the output status, if necessary. */
        if (out_normal_int_status != nullptr) {
            *out_normal_int_status = normal_int_status;
        }

        /* If we don't have an error interrupt, just use the normal status. */
        if (reg::HasValue(normal_int_status, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_ERROR_INTERRUPT, NO_ERROR))) {
            /* If the wait mask has any bits set, we're done waiting for the interrupt. */
            const u16 masked_status = (normal_int_status & wait_mask);
            R_UNLESS(masked_status != 0, sdmmc::ResultNoWaitedInterrupt());

            /* Write the masked value to the status register to ensure consistent state. */
            reg::Write(this->registers->normal_int_status, masked_status);
            return ResultSuccess();
        }

        /* We have an error interrupt. Write the status to the register to ensure consistent state. */
        reg::Write(this->registers->error_int_status, error_int_status);

        /* Check the error interrupt status bits, and return appropriate errors. */
        R_UNLESS(reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_COMMAND_INDEX,   NO_ERROR)), sdmmc::ResultResponseIndexError());
        R_UNLESS(reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_COMMAND_END_BIT, NO_ERROR)), sdmmc::ResultResponseEndBitError());
        R_UNLESS(reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_COMMAND_CRC,     NO_ERROR)), sdmmc::ResultResponseCrcError());
        R_UNLESS(reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_COMMAND_TIMEOUT, NO_ERROR)), sdmmc::ResultResponseTimeoutError());
        R_UNLESS(reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_DATA_END_BIT,    NO_ERROR)), sdmmc::ResultDataEndBitError());
        R_UNLESS(reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_DATA_CRC,        NO_ERROR)), sdmmc::ResultDataCrcError());
        R_UNLESS(reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_DATA_TIMEOUT,    NO_ERROR)), sdmmc::ResultDataTimeoutError());

        /* Check for auto cmd errors. */
        if (reg::HasValue(error_int_status, SD_REG_BITS_ENUM(ERROR_INTERRUPT_STATUS_AUTO_CMD, ERROR))) {
            R_UNLESS(reg::HasValue(auto_cmd_err_status, SD_REG_BITS_ENUM(AUTO_CMD_ERROR_AUTO_CMD_INDEX,   NO_ERROR)), sdmmc::ResultAutoCommandResponseIndexError());
            R_UNLESS(reg::HasValue(auto_cmd_err_status, SD_REG_BITS_ENUM(AUTO_CMD_ERROR_AUTO_CMD_END_BIT, NO_ERROR)), sdmmc::ResultAutoCommandResponseEndBitError());
            R_UNLESS(reg::HasValue(auto_cmd_err_status, SD_REG_BITS_ENUM(AUTO_CMD_ERROR_AUTO_CMD_CRC,     NO_ERROR)), sdmmc::ResultAutoCommandResponseCrcError());
            R_UNLESS(reg::HasValue(auto_cmd_err_status, SD_REG_BITS_ENUM(AUTO_CMD_ERROR_AUTO_CMD_TIMEOUT, NO_ERROR)), sdmmc::ResultAutoCommandResponseTimeoutError());

            /* An known auto cmd error occurred. */
            return sdmmc::ResultSdHostStandardUnknownAutoCmdError();
        } else {
            /* Unknown error occurred. */
            return sdmmc::ResultSdHostStandardUnknownError();
        }
    }

    Result SdHostStandardController::WaitCommandComplete() {
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            /* Wait for interrupt. */
            Result result = this->WaitInterrupt(CommandTimeoutMilliSeconds);
            if (R_SUCCEEDED(result)) {
                /* If we succeeded, check/clear our interrupt status. */
                result = this->CheckAndClearInterruptStatus(nullptr, reg::Encode(SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_COMMAND_COMPLETE, COMPLETE)));
                this->ClearInterrupt();

                if (R_FAILED(result)) {
                    this->AbortTransaction();
                }

                return result;
            } else if (sdmmc::ResultDeviceRemoved::Includes(result)) {
                /* Otherwise, check if the device was removed. */
                return result;
            } else {
                /* If the device wasn't removed, cancel our transaction. */
                this->AbortTransaction();
                return sdmmc::ResultCommandCompleteSoftwareTimeout();
            }
        }
        #else
        {
            /* Ensure that we control the registers. */
            this->EnsureControl();

            /* Wait while command is not complete. */
            {
                ManualTimer timer(CommandTimeoutMilliSeconds);
                while (true) {
                    /* Check and clear the interrupt status. */
                    const auto result = this->CheckAndClearInterruptStatus(nullptr, reg::Encode(SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_COMMAND_COMPLETE, COMPLETE)));

                    /* If we succeeded, we're done. */
                    if (R_SUCCEEDED(result)) {
                        return ResultSuccess();
                    } else if (sdmmc::ResultNoWaitedInterrupt::Includes(result)) {
                        /* Otherwise, if the wait for the interrupt isn't done, update the timer and check for timeout. */
                        if (!timer.Update()) {
                            this->AbortTransaction();
                            return sdmmc::ResultCommandCompleteSoftwareTimeout();
                        }
                    } else {
                        /* Otherwise, we have a generic failure. */
                        this->AbortTransaction();
                        return result;
                    }
                }
            }
        }
        #endif
    }

    Result SdHostStandardController::WaitTransferComplete() {
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            /* Wait while transfer is not complete. */
            while (true) {
                /* Get the last block count. */
                const u16 last_block_count = reg::Read(this->registers->block_count);

                /* Wait for interrupt. */
                Result result = this->WaitInterrupt(this->check_transfer_interval_ms);
                if (R_SUCCEEDED(result)) {
                    /* If we succeeded, check/clear our interrupt status. */
                    volatile u16 normal_int_status;
                    result = this->CheckAndClearInterruptStatus(std::addressof(normal_int_status), reg::Encode(SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_TRANSFER_COMPLETE,  COMPLETE),
                                                                                                               SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_DMA_INTERRUPT,     GENERATED)));

                    this->ClearInterrupt();

                    /* If the interrupt succeeded, check status. */
                    if (R_SUCCEEDED(result)) {
                        /* If the transfer is complete, we're done. */
                        if (reg::HasValue(normal_int_status, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_TRANSFER_COMPLETE, COMPLETE))) {
                            return ResultSuccess();
                        }

                        /* Otherwise, if a DMA interrupt was generated, advance to the next address. */
                        if (reg::HasValue(normal_int_status,  SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_DMA_INTERRUPT, GENERATED))) {
                            reg::Write(this->registers->adma_address,       static_cast<u32>(this->next_sdma_address >> 0));
                            reg::Write(this->registers->upper_adma_address, static_cast<u32>(this->next_sdma_address >> BITSIZEOF(u32)));

                            this->next_sdma_address += SdmaBufferBoundary;
                        }
                    } else {
                        /* Abort the transaction. */
                        this->AbortTransaction();
                        return result;
                    }

                    return result;
                } else if (sdmmc::ResultDeviceRemoved::Includes(result)) {
                    /* Otherwise, check if the device was removed. */
                    return result;
                } else {
                    /* Otherwise, timeout if the transfer hasn't advanced. */
                    if (last_block_count != reg::Read(this->registers->block_count)) {
                        this->AbortTransaction();
                        return sdmmc::ResultTransferCompleteSoftwareTimeout();
                    }
                }
            }
        }
        #else
        {
            /* Wait while transfer is not complete. */
            while (true) {
                /* Get the last block count. */
                const u16 last_block_count = reg::Read(this->registers->block_count);

                /* Wait until transfer times out. */
                {
                    ManualTimer timer(this->check_transfer_interval_ms);
                    while (true) {
                        /* Check/clear our interrupt status. */
                        volatile u16 normal_int_status;
                        const auto result = this->CheckAndClearInterruptStatus(std::addressof(normal_int_status), reg::Encode(SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_TRANSFER_COMPLETE,  COMPLETE),
                                                                                                                              SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_DMA_INTERRUPT,     GENERATED)));

                        /* If the check succeeded, check status. */
                        if (R_SUCCEEDED(result)) {
                            /* If the transfer is complete, we're done. */
                            if (reg::HasValue(normal_int_status, SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_TRANSFER_COMPLETE, COMPLETE))) {
                                return ResultSuccess();
                            }

                            /* Otherwise, if a DMA interrupt was generated, advance to the next address. */
                            if (reg::HasValue(normal_int_status,  SD_REG_BITS_ENUM(NORMAL_INTERRUPT_STATUS_DMA_INTERRUPT, GENERATED))) {
                                reg::Write(this->registers->adma_address,       static_cast<u32>(this->next_sdma_address >> 0));
                                reg::Write(this->registers->upper_adma_address, static_cast<u32>(this->next_sdma_address >> BITSIZEOF(u32)));

                                this->next_sdma_address += SdmaBufferBoundary;
                            }
                        } else if (sdmmc::ResultNoWaitedInterrupt::Includes(result)) {
                            /* Otherwise, if the wait for the interrupt isn't done, update the timer and check for timeout. */
                            if (!timer.Update()) {
                                /* Only timeout if the transfer hasn't advanced. */
                                if (last_block_count != reg::Read(this->registers->block_count)) {
                                    this->AbortTransaction();
                                    return sdmmc::ResultTransferCompleteSoftwareTimeout();
                                }
                                break;
                            }
                        } else {
                            /* Otherwise, we have a generic failure. */
                            this->AbortTransaction();
                            return result;
                        }
                    }
                }
            }
        }
        #endif
    }

    Result SdHostStandardController::WaitWhileBusy() {
        /* Ensure that we control the registers. */
        this->EnsureControl();

        /* Wait while busy. */
        {
            ManualTimer timer(BusyTimeoutMilliSeconds);
            while (true) {
                /* Check if the target has been removed. */
                R_TRY(this->CheckRemoved());

                /* If the DAT0 line signal is level high, we're done. */
                if (reg::HasValue(this->registers->present_state, SD_REG_BITS_ENUM(PRESENT_STATE_DAT0_LINE_SIGNAL_LEVEL, HIGH))) {
                    return ResultSuccess();
                }

                /* Otherwise, check if we're timed out. */
                if (!timer.Update()) {
                    this->AbortTransaction();
                    return sdmmc::ResultBusySoftwareTimeout();
                }
            }
        }
    }

    void SdHostStandardController::GetResponse(u32 *out_response, size_t response_size, ResponseType response_type) const {
        /* Check that we can write the response. */
        AMS_ABORT_UNLESS(out_response != nullptr);

        /* Get the response appropriately. */
        switch (response_type) {
            case ResponseType_R1:
            case ResponseType_R3:
            case ResponseType_R6:
            case ResponseType_R7:
                /* 32-bit response. */
                AMS_ABORT_UNLESS(response_size >= sizeof(u32) * 1);
                out_response[0] = reg::Read(this->registers->response[0]);
                break;
            case ResponseType_R2:
                /* 128-bit response. */
                AMS_ABORT_UNLESS(response_size >= sizeof(u32) * 4);
                out_response[0] = reg::Read(this->registers->response[0]);
                out_response[1] = reg::Read(this->registers->response[1]);
                out_response[2] = reg::Read(this->registers->response[2]);
                out_response[3] = reg::Read(this->registers->response[3]);
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    Result SdHostStandardController::IssueCommandWithDeviceClock(const Command *command, TransferData *xfer_data, u32 *out_num_transferred_blocks) {
        /* Wait until we can issue the command. */
        R_TRY(this->WaitWhileCommandInhibit((xfer_data != nullptr) || command->is_busy));

        /* Configure for the transfer. */
        u32 num_transferred_blocks = 0;
        if (xfer_data != nullptr) {
            /* Setup the transfer, and get the number of blocks. */
            this->SetTransfer(std::addressof(num_transferred_blocks), xfer_data);

            /* Ensure the device sees consistent data with the cpu. */
            dd::FlushDataCache(xfer_data->buffer, xfer_data->block_size * num_transferred_blocks);
        }

        /* Issue the command with interrupt status enabled. */
        {
            this->EnableInterruptStatus();
            ON_SCOPE_EXIT { this->DisableInterruptStatus(); };

            /* Set the command. */
            this->SetCommand(command, xfer_data != nullptr);

            /* Wait for the command to complete. */
            R_TRY(this->WaitCommandComplete());

            /* Process any response. */
            if (command->response_type != ResponseType_R0) {
                this->last_response_type = command->response_type;
                this->GetResponse(this->last_response, sizeof(this->last_response), this->last_response_type);
            }

            /* Wait for data to be transferred. */
            if (xfer_data != nullptr) {
                R_TRY(this->WaitTransferComplete());
            }
        }

        /* If data was transferred, ensure we're in a consistent state. */
        if (xfer_data != nullptr) {
            /* Ensure the cpu sees consistent data with the device. */
            if (xfer_data->transfer_direction == TransferDirection_ReadFromDevice) {
                dd::InvalidateDataCache(xfer_data->buffer, xfer_data->block_size * num_transferred_blocks);
            }

            /* Set the number of transferred blocks. */
            if (out_num_transferred_blocks != nullptr) {
                *out_num_transferred_blocks = num_transferred_blocks;
            }

            /* Process stop transition command. */
            this->last_stop_transmission_response = this->registers->response[3];
        }

        /* Wait until we're no longer busy. */
        if (command->is_busy || (xfer_data != nullptr)) {
            R_TRY(this->WaitWhileBusy());
        }

        return ResultSuccess();
    }

    Result SdHostStandardController::IssueStopTransmissionCommandWithDeviceClock(u32 *out_response) {
        /* Wait until we can issue the command. */
        R_TRY(this->WaitWhileCommandInhibit(false));

        /* Issue the command with interrupt status enabled. */
        constexpr ResponseType StopTransmissionCommandResponseType = ResponseType_R1;
        {
            this->EnableInterruptStatus();
            ON_SCOPE_EXIT { this->DisableInterruptStatus(); };

            /* Set the command. */
            Command command(CommandIndex_StopTransmission, 0, StopTransmissionCommandResponseType, true);
            this->SetCommand(std::addressof(command), false);

            /* Wait for the command to complete. */
            R_TRY(this->WaitCommandComplete());
        }

        /* Process response. */
        this->GetResponse(out_response, sizeof(u32), StopTransmissionCommandResponseType);

        /* Wait until we're done. */
        R_TRY(this->WaitWhileBusy());

        return ResultSuccess();
    }

    SdHostStandardController::SdHostStandardController(dd::PhysicalAddress registers_phys_addr, size_t registers_size) {
        /* Translate the physical address to a address. */
        const uintptr_t registers_addr = dd::QueryIoMapping(registers_phys_addr, registers_size);

        /* Set registers. */
        AMS_ABORT_UNLESS(registers_addr != 0);
        this->registers = reinterpret_cast<SdHostStandardRegisters *>(registers_addr);

        /* Reset DMA buffers, if we have any. */
        #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
        this->ResetBufferInfos();
        #endif

        /* Clear removed event, if we have one. */
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        this->removed_event = nullptr;
        #endif

        /* Clear dma address. */
        this->next_sdma_address          = 0;
        this->check_transfer_interval_ms = DefaultCheckTransferIntervalMilliSeconds;

        /* Clear clock/power trackers. */
        this->device_clock_frequency_khz = 0;
        this->is_power_saving_enable     = false;
        this->is_device_clock_enable     = false;

        /* Clear last response. */
        this->last_response_type              = ResponseType_R0;
        std::memset(this->last_response, 0, sizeof(this->last_response));
        this->last_stop_transmission_response = 0;
    }

    void SdHostStandardController::Initialize() {
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            os::InitializeWaitableManager(std::addressof(this->waitable_manager));

            AMS_ABORT_UNLESS(this->interrupt_event != nullptr);
            os::InitializeWaitableHolder(std::addressof(this->interrupt_event_holder), this->interrupt_event);
            os::LinkWaitableHolder(std::addressof(this->waitable_manager), std::addressof(this->interrupt_event_holder));

            if (this->removed_event != nullptr) {
                os::InitializeWaitableHolder(std::addressof(this->removed_event_holder), this->removed_event);
                os::LinkWaitableHolder(std::addressof(this->waitable_manager), std::addressof(this->removed_event_holder));
            }
        }
        #endif
    }

    void SdHostStandardController::Finalize() {
        #if defined(AMS_SDMMC_USE_OS_EVENTS)
        {
            if (this->removed_event != nullptr) {
                os::UnlinkWaitableHolder(std::addressof(this->removed_event_holder));
                os::FinalizeWaitableHolder(std::addressof(this->removed_event_holder));
            }

            os::UnlinkWaitableHolder(std::addressof(this->interrupt_event_holder));
            os::FinalizeWaitableHolder(std::addressof(this->interrupt_event_holder));

            os::FinalizeWaitableManager(std::addressof(this->waitable_manager));
        }
        #endif
    }

    #if defined(AMS_SDMMC_USE_DEVICE_VIRTUAL_ADDRESS)
    void SdHostStandardController::RegisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) {
        /* Find and set a free info. */
        for (auto &info : this->buffer_infos) {
            if (info.buffer_address == 0) {
                info = {
                    .buffer_address                = buffer,
                    .buffer_size                   = buffer_size,
                    .buffer_device_virtual_address = buffer_device_virtual_address,
                };
                return;
            }
        }

        AMS_ABORT("Out of BufferInfos\n");
    }

    void SdHostStandardController::UnregisterDeviceVirtualAddress(uintptr_t buffer, size_t buffer_size, ams::dd::DeviceVirtualAddress buffer_device_virtual_address) {
        /* Find and clear the buffer info. */
        for (auto &info : this->buffer_infos) {
            if (info.buffer_address == 0) {
                AMS_ABORT_UNLESS(info.buffer_size == buffer_size);
                AMS_ABORT_UNLESS(info.buffer_device_virtual_address == buffer_device_virtual_address);

                info.buffer_address = 0;
                info.buffer_size    = 0;
                return;
            }
        }

        AMS_ABORT("BufferInfo not found\n");
    }
    #endif

    void SdHostStandardController::SetWorkBuffer(void *wb, size_t wb_size) {
        AMS_UNUSED(wb, wb_size);
        AMS_ABORT("WorkBuffer is not needed\n");
    }

    BusPower SdHostStandardController::GetBusPower() const {
        /* Check if the bus has power. */
        if (reg::HasValue(this->registers->power_control, SD_REG_BITS_ENUM(POWER_CONTROL_SD_BUS_POWER_FOR_VDD1, ON))) {
            /* If it does, return the corresponding power. */
            switch (reg::GetValue(this->registers->power_control, SD_REG_BITS_MASK(POWER_CONTROL_SD_BUS_VOLTAGE_SELECT_FOR_VDD1))) {
                case SD_HOST_STANDARD_POWER_CONTROL_SD_BUS_VOLTAGE_SELECT_FOR_VDD1_1_8V:
                    return BusPower_1_8V;
                case SD_HOST_STANDARD_POWER_CONTROL_SD_BUS_VOLTAGE_SELECT_FOR_VDD1_3_3V:
                    return BusPower_3_3V;
                AMS_UNREACHABLE_DEFAULT_CASE();
            }
        } else {
            /* It doesn't, so it's off. */
            return BusPower_Off;
        }
    }

    void SdHostStandardController::SetBusWidth(BusWidth bus_width) {
        /* Check that we support the bus width. */
        AMS_ABORT_UNLESS(this->IsSupportedBusWidth(bus_width));

        /* Set the appropriate data transfer width. */
        switch (bus_width) {
            case BusWidth_1Bit:
                reg::ReadWrite(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_DATA_TRANSFER_WIDTH,          ONE_BIT));
                reg::ReadWrite(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_EXTENDED_DATA_TRANSFER_WIDTH, USE_DATA_TRANSFER_WIDTH));
                break;
            case BusWidth_4Bit:
                reg::ReadWrite(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_DATA_TRANSFER_WIDTH,          FOUR_BIT));
                reg::ReadWrite(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_EXTENDED_DATA_TRANSFER_WIDTH, USE_DATA_TRANSFER_WIDTH));
                break;
            case BusWidth_8Bit:
                reg::ReadWrite(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_EXTENDED_DATA_TRANSFER_WIDTH, EIGHT_BIT));
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    BusWidth SdHostStandardController::GetBusWidth() const {
        /* Check if the bus is using eight-bit extended data transfer. */
        if (reg::HasValue(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_EXTENDED_DATA_TRANSFER_WIDTH, EIGHT_BIT))) {
            return BusWidth_8Bit;
        } else {
            /* Bus is configured as USE_DATA_TRANSFER_WIDTH, so check if it's four bit. */
            if (reg::HasValue(this->registers->host_control, SD_REG_BITS_ENUM(HOST_CONTROL_DATA_TRANSFER_WIDTH, FOUR_BIT))) {
                return BusWidth_4Bit;
            } else {
                return BusWidth_1Bit;
            }
        }
    }

    void SdHostStandardController::SetPowerSaving(bool en) {
        /* Set whether we're power saving enable. */
        this->is_power_saving_enable = en;

        /* Configure accordingly. */
        if (this->is_power_saving_enable) {
            /* We want to disable SD clock if it's enabled. */
            if (reg::HasValue(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE))) {
                reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));
            }
        } else {
            /* We want to enable SD clock if it's disabled and we're supposed to enable device clock. */
            if (this->is_device_clock_enable && reg::HasValue(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE))) {
                reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));
            }
        }
    }

    void SdHostStandardController::EnableDeviceClock() {
        /* If we're not in power-saving mode and the device clock is disabled, enable it. */
        if (!this->is_power_saving_enable && reg::HasValue(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE))) {
            reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));
        }
        this->is_device_clock_enable = true;
    }

    void SdHostStandardController::DisableDeviceClock() {
        /* Unconditionally disable the device clock. */
        this->is_device_clock_enable = false;
        reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));
    }

    void SdHostStandardController::ChangeCheckTransferInterval(u32 ms) {
        this->check_transfer_interval_ms = ms;
    }

    void SdHostStandardController::SetDefaultCheckTransferInterval() {
        this->check_transfer_interval_ms = DefaultCheckTransferIntervalMilliSeconds;
    }

    Result SdHostStandardController::IssueCommand(const Command *command, TransferData *xfer_data, u32 *out_num_transferred_blocks) {
        /* We need to have device clock enabled to issue commands. */
        AMS_ABORT_UNLESS(this->is_device_clock_enable);

        /* Check if we need to temporarily re-enable the device clock. */
        const bool clock_disabled = reg::HasValue(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));

        /* Ensure that the clock is enabled and the device is usable for the period we're using it. */
        if (clock_disabled) {
            /* Turn on the clock. */
            reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));

            /* Ensure that our configuration takes. */
            this->EnsureControl();

            /* Wait 8 device clocks to be sure that it's usable. */
            WaitClocks(8, this->device_clock_frequency_khz);
        }
        ON_SCOPE_EXIT { if (clock_disabled) { reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE)); } };

        /* Issue the command. */
        {
            /* After we issue the command, we need to wait 8 device clocks. */
            ON_SCOPE_EXIT { WaitClocks(8, this->device_clock_frequency_khz); };

            return this->IssueCommandWithDeviceClock(command, xfer_data, out_num_transferred_blocks);
        }
    }

    Result SdHostStandardController::IssueStopTransmissionCommand(u32 *out_response) {
        /* We need to have device clock enabled to issue commands. */
        AMS_ABORT_UNLESS(this->is_device_clock_enable);

        /* Check if we need to temporarily re-enable the device clock. */
        const bool clock_disabled = reg::HasValue(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE));

        /* Ensure that the clock is enabled and the device is usable for the period we're using it. */
        if (clock_disabled) {
            /* Turn on the clock. */
            reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, ENABLE));

            /* Ensure that our configuration takes. */
            this->EnsureControl();

            /* Wait 8 device clocks to be sure that it's usable. */
            WaitClocks(8, this->device_clock_frequency_khz);
        }
        ON_SCOPE_EXIT { if (clock_disabled) { reg::ReadWrite(this->registers->clock_control, SD_REG_BITS_ENUM(CLOCK_CONTROL_SD_CLOCK_ENABLE, DISABLE)); } };

        /* Issue the command. */
        {
            /* After we issue the command, we need to wait 8 device clocks. */
            ON_SCOPE_EXIT { WaitClocks(8, this->device_clock_frequency_khz); };

            return this->IssueStopTransmissionCommandWithDeviceClock(out_response);
        }
    }

    void SdHostStandardController::GetLastResponse(u32 *out_response, size_t response_size, ResponseType response_type) const {
        /* Check that we can get the response. */
        AMS_ABORT_UNLESS(out_response != nullptr);
        AMS_ABORT_UNLESS(response_type == this->last_response_type);

        /* Get the response appropriately. */
        switch (response_type) {
            case ResponseType_R1:
            case ResponseType_R3:
            case ResponseType_R6:
            case ResponseType_R7:
                /* 32-bit response. */
                AMS_ABORT_UNLESS(response_size >= sizeof(u32) * 1);
                out_response[0] = this->last_response[0];
                break;
            case ResponseType_R2:
                /* 128-bit response. */
                AMS_ABORT_UNLESS(response_size >= sizeof(u32) * 4);
                out_response[0] = this->last_response[0];
                out_response[1] = this->last_response[1];
                out_response[2] = this->last_response[2];
                out_response[3] = this->last_response[3];
                break;
            AMS_UNREACHABLE_DEFAULT_CASE();
        }
    }

    void SdHostStandardController::GetLastStopTransmissionResponse(u32 *out_response, size_t response_size) const {
        /* Check that we can get the response. */
        AMS_ABORT_UNLESS(out_response != nullptr);
        AMS_ABORT_UNLESS(response_size >= sizeof(u32));

        /* Get the response. */
        out_response[0] = this->last_stop_transmission_response;
    }

}
