/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace ams::fatal::srv {

    class FatalConfig {
        private:
            char serial_number[0x18];
            SetSysFirmwareVersion firmware_version;
            u64 language_code;
            u64 quest_reboot_interval_second;
            bool transition_to_fatal;
            bool show_extra_info;
            bool quest_flag;
            const char *error_msg;
            const char *error_desc;
            const char *quest_desc;
            u64 fatal_auto_reboot_interval;
            bool fatal_auto_reboot_enabled;
        public:
            FatalConfig();

            const char *GetSerialNumber() const {
                return this->serial_number;
            }

            const SetSysFirmwareVersion &GetFirmwareVersion() const {
                return this->firmware_version;
            }

            void UpdateLanguageCode() {
                setGetLanguageCode(&this->language_code);
            }

            u64 GetLanguageCode() const {
                return this->language_code;
            }

            bool ShouldTransitionToFatal() const {
                return this->transition_to_fatal;
            }

            bool ShouldShowExtraInfo() const {
                return this->show_extra_info;
            }

            bool IsQuest() const {
                return this->quest_flag;
            }

            bool IsFatalRebootEnabled() const {
                return this->fatal_auto_reboot_enabled;
            }

            u64 GetQuestRebootTimeoutInterval() const {
                return this->quest_reboot_interval_second * 1'000'000'000ul;
            }

            u64 GetFatalRebootTimeoutInterval() const {
                return this->fatal_auto_reboot_interval * 1'000'000ul;
            }

            const char *GetErrorMessage() const {
                return this->error_msg;
            }

            const char *GetErrorDescription() const {
                if (this->IsQuest()) {
                    return this->quest_desc;
                } else {
                    return this->error_desc;
                }
            }
    };

    os::WaitableHolder *GetFatalDirtyWaitableHolder();
    void OnFatalDirtyEvent();
    const FatalConfig &GetFatalConfig();

}
