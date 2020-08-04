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

namespace ams::fatal::srv {

    class FatalConfig {
        private:
            settings::system::SerialNumber serial_number{};
            settings::system::FirmwareVersion firmware_version{};
            u64 language_code{};
            TimeSpan quest_reboot_interval{};
            bool transition_to_fatal{};
            bool show_extra_info{};
            bool quest_flag{};
            const char *error_msg{};
            const char *error_desc{};
            const char *quest_desc{};
            TimeSpan fatal_auto_reboot_interval{};
            bool fatal_auto_reboot_enabled{};
        public:
            FatalConfig();

            const settings::system::SerialNumber &GetSerialNumber() const {
                return this->serial_number;
            }

            const settings::system::FirmwareVersion &GetFirmwareVersion() const {
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

            TimeSpan GetQuestRebootTimeoutInterval() const {
                return this->quest_reboot_interval;
            }

            TimeSpan GetFatalRebootTimeoutInterval() const {
                return this->fatal_auto_reboot_interval;
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

    os::WaitableHolderType *GetFatalDirtyWaitableHolder();
    void OnFatalDirtyEvent();
    const FatalConfig &GetFatalConfig();

}
