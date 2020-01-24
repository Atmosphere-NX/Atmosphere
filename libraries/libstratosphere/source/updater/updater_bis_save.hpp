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
#include "updater_bis_management.hpp"

namespace ams::updater {

    class BisSave {
        public:
            static constexpr size_t SaveSize = BctSize;
        private:
            Boot0Accessor accessor;
            void *save_buffer;
        public:
            BisSave() : save_buffer(nullptr) { }
        private:
            static size_t GetVerificationFlagOffset(BootModeType mode);
        public:
            Result Initialize(void *work_buffer, size_t work_buffer_size);
            void Finalize();

            Result Load();
            Result Save();
            bool GetNeedsVerification(BootModeType mode);
            void SetNeedsVerification(BootModeType mode, bool needs_verification);
    };

}
