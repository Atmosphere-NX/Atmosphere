/*
 * Copyright (c) 2019-2020 Adubbz, Atmosphère-NX
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
#include <stratosphere/ncm/ncm_package_system_update_task.hpp>

namespace ams::ncm {

    class PackageSystemDowngradeTask : public PackageSystemUpdateTask {
        public:
            Result Commit();
        protected:
            virtual Result PrepareContentMetaIfLatest(const ContentMetaKey &key) override;
        private:
            Result PreCommit();
    };

}
