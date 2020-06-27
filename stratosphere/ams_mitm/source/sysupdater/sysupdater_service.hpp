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
#include <stratosphere.hpp>
#include "sysupdater_i_async_result.hpp"
#include "sysupdater_apply_manager.hpp"

namespace ams::mitm::sysupdater {

    constexpr inline size_t FirmwareVariationCountMax = 16;

    struct UpdateInformation {
        u32 version;
        bool exfat_supported;
        u32 firmware_variation_count;
        ncm::FirmwareVariationId firmware_variation_ids[FirmwareVariationCountMax];
    };

    struct UpdateValidationInfo {
        ncm::ContentMetaKey invalid_key;
        ncm::ContentId invalid_content_id;
    };

    struct SystemUpdateProgress {
        s64 current_size;
        s64 total_size;
    };

    class SystemUpdateService final : public sf::IServiceObject {
        private:
            enum class CommandId {
                GetUpdateInformation     = 0,
                ValidateUpdate           = 1,
                SetupUpdate              = 2,
                SetupUpdateWithVariation = 3,
                RequestPrepareUpdate     = 4,
                GetPrepareUpdateProgress = 5,
                HasPreparedUpdate        = 6,
                ApplyPreparedUpdate      = 7,
            };
        private:
            SystemUpdateApplyManager apply_manager;
            std::optional<ncm::PackageSystemDowngradeTask> update_task;
            std::optional<os::TransferMemory> update_transfer_memory;
            bool setup_update;
            bool requested_update;
        public:
            constexpr SystemUpdateService() : apply_manager(), update_task(), update_transfer_memory(), setup_update(false), requested_update(false) { /* ... */ }
        private:
            Result SetupUpdateImpl(os::ManagedHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
            Result InitializeUpdateTask(os::ManagedHandle &transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
        private:
            Result GetUpdateInformation(sf::Out<UpdateInformation> out, const ncm::Path &path);
            Result ValidateUpdate(sf::Out<Result> out_validate_result, sf::Out<UpdateValidationInfo> out_validate_info, const ncm::Path &path);
            Result SetupUpdate(sf::CopyHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat);
            Result SetupUpdateWithVariation(sf::CopyHandle transfer_memory, u64 transfer_memory_size, const ncm::Path &path, bool exfat, ncm::FirmwareVariationId firmware_variation_id);
            Result RequestPrepareUpdate(sf::OutCopyHandle out_event_handle, sf::Out<std::shared_ptr<IAsyncResult>> out_async);
            Result GetPrepareUpdateProgress(sf::Out<SystemUpdateProgress> out);
            Result HasPreparedUpdate(sf::Out<bool> out);
            Result ApplyPreparedUpdate();
        public:
            DEFINE_SERVICE_DISPATCH_TABLE {
                MAKE_SERVICE_COMMAND_META(GetUpdateInformation),
                MAKE_SERVICE_COMMAND_META(ValidateUpdate),
                MAKE_SERVICE_COMMAND_META(SetupUpdate),
                MAKE_SERVICE_COMMAND_META(SetupUpdateWithVariation),
                MAKE_SERVICE_COMMAND_META(RequestPrepareUpdate),
                MAKE_SERVICE_COMMAND_META(GetPrepareUpdateProgress),
                MAKE_SERVICE_COMMAND_META(HasPreparedUpdate),
                MAKE_SERVICE_COMMAND_META(ApplyPreparedUpdate),
            };
    };

}
