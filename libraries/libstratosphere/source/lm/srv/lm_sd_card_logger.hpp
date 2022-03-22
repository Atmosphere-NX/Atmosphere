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

namespace ams::lm::srv {

    class SdCardLogger {
        AMS_SINGLETON_TRAITS(SdCardLogger);
        public:
            using LoggingObserver = void (*)(bool available);
        private:
            os::SdkMutex m_logging_observer_mutex;
            bool m_is_enabled;
            bool m_is_sd_card_mounted;
            bool m_is_sd_card_status_unknown;
            char m_log_file_path[0x80];
            s64 m_log_file_offset;
            LoggingObserver m_logging_observer;
        public:
            void Finalize();

            void SetLoggingObserver(LoggingObserver observer);

            bool Write(const u8 *data, size_t size);
        private:
            bool GetEnabled() const;
            void SetEnabled(bool enabled);

            bool Initialize();
    };

}
