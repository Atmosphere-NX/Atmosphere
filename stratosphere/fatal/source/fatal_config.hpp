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
#include <stratosphere.hpp>

namespace ams::fatal::srv {

    class FatalConfig {
        private:
            settings::system::SerialNumber m_serial_number{};
            settings::system::FirmwareVersion m_firmware_version{};
            u64 m_language_code{};
            TimeSpan m_quest_reboot_interval{};
            bool m_transition_to_fatal{};
            bool m_show_extra_info{};
            bool m_quest_flag{};
            const char *m_error_msg{};
            const char *m_error_desc{};
            const char *m_quest_desc{};
            TimeSpan m_fatal_auto_reboot_interval{};
            bool m_fatal_auto_reboot_enabled{};
        public:
            FatalConfig();

            const settings::system::SerialNumber &GetSerialNumber() const {
                return m_serial_number;
            }

            const settings::system::FirmwareVersion &GetFirmwareVersion() const {
                return m_firmware_version;
            }

            void UpdateLanguageCode() {
                setGetLanguageCode(&m_language_code);
            }

            u64 GetLanguageCode() const {
                return m_language_code;
            }

            bool ShouldTransitionToFatal() const {
                return m_transition_to_fatal;
            }

            bool ShouldShowExtraInfo() const {
                return m_show_extra_info;
            }

            bool IsQuest() const {
                return m_quest_flag;
            }

            bool IsFatalRebootEnabled() const {
                return m_fatal_auto_reboot_enabled;
            }

            TimeSpan GetQuestRebootTimeoutInterval() const {
                return m_quest_reboot_interval;
            }

            TimeSpan GetFatalRebootTimeoutInterval() const {
                return m_fatal_auto_reboot_interval;
            }

            const char *GetErrorMessage() const {
                return m_error_msg;
            }

            const char *GetErrorDescription() const {
                if (this->IsQuest()) {
                    return m_quest_desc;
                } else {
                    return m_error_desc;
                }
            }
    };

    os::MultiWaitHolderType *GetFatalDirtyMultiWaitHolder();
    void OnFatalDirtyEvent();
    const FatalConfig &GetFatalConfig();

}
