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
#include "lm_common.hpp"

namespace ams::lm::impl {

    class SdCardDetectionEventNotifier {
        public:
            SdCardDetectionEventNotifier(os::SystemEvent *out_event);
        private:
            fs::SdCardDetectionEventNotifier event_notifier;
    };

    class SdCardLogging {
        NON_COPYABLE(SdCardLogging);
        NON_MOVEABLE(SdCardLogging);
        private:
            os::SdkMutex update_enabled_fn_lock;
            bool enabled;
            bool sd_card_mounted;
            bool sd_card_ok;
            char log_file_path[0x80];
            size_t log_file_offset;
            UpdateEnabledFunction update_enabled_fn;
        public:
            SdCardLogging() : update_enabled_fn_lock(), enabled(false), sd_card_mounted(false), sd_card_ok(false), log_file_path{}, log_file_offset(0), update_enabled_fn(nullptr) {}
            
            bool Initialize();
            void Dispose();
            bool EnsureLogFile();
            bool SaveLog(const void *log_data, size_t log_data_size);

            void SetEnabled(bool enabled) {
                std::scoped_lock lk(this->update_enabled_fn_lock);
                if (this->enabled != enabled) {
                    this->enabled = enabled;
                    if (this->update_enabled_fn) {
                        this->update_enabled_fn(enabled);
                    }
                }
            }

            void SetUpdateEnabledFunction(UpdateEnabledFunction update_enabled_fn) {
                std::scoped_lock lk(this->update_enabled_fn_lock);
                this->update_enabled_fn = update_enabled_fn;
            }
    };

    SdCardLogging *GetSdCardLogging();

}