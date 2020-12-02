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
#include "fs_common.hpp"

namespace ams::fs {

    class SdCardDetectionEventNotifier {
        NON_COPYABLE(SdCardDetectionEventNotifier);
        NON_MOVEABLE(SdCardDetectionEventNotifier);
        private:
            void *notifier;
        public:
            SdCardDetectionEventNotifier() : notifier(nullptr) {}
            ~SdCardDetectionEventNotifier();

            void Open(void *notifier) {
                this->notifier = notifier;
            }

            Result GetEventHandle(os::SystemEvent *out_event, os::EventClearMode clear_mode);
    };

    Result MountSdCard(const char *name);

    Result MountSdCardErrorReportDirectoryForAtmosphere(const char *name);

    bool IsSdCardInserted();

    Result OpenSdCardDetectionEventNotifier(SdCardDetectionEventNotifier *out);

}
