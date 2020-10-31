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
#include <stratosphere.hpp>

namespace ams::i2c {

    namespace {

        constinit os::SdkMutex g_init_mutex;
        constinit int g_initialize_count = 0;

        constinit os::SdkMutex g_i2c_mutex;
        std::shared_ptr<sf::IManager> g_i2c_manager;
        constinit int g_i2c_count = 0;

        constinit os::SdkMutex g_i2c_pcv_mutex;
        std::shared_ptr<sf::IManager> g_i2c_pcv_manager;
        constinit int g_i2c_pcv_count = 0;

    }

    void InitializeWith(std::shared_ptr<i2c::sf::IManager> &&sp, std::shared_ptr<i2c::sf::IManager> &&sp_pcv) {
        std::scoped_lock lk(g_init_mutex);

        AMS_ABORT_UNLESS(g_initialize_count == 0);

        {
            std::scoped_lock lk(g_i2c_mutex);
            g_i2c_manager = std::move(sp);
            AMS_ABORT_UNLESS(g_i2c_count == 0);
            g_i2c_count = 1;
        }

        {
            std::scoped_lock lk(g_i2c_pcv_mutex);
            g_i2c_pcv_manager = std::move(sp);
            AMS_ABORT_UNLESS(g_i2c_pcv_count == 0);
            g_i2c_pcv_count = 1;
        }

        g_initialize_count = 1;
    }

    void InitializeEmpty() {
        std::scoped_lock lk(g_init_mutex);

        ++g_initialize_count;
    }

    void Finalize() {
        std::scoped_lock lk(g_init_mutex);

        AMS_ASSERT(g_initialize_count > 0);

        if ((--g_initialize_count) == 0) {
            {
                std::scoped_lock lk(g_i2c_mutex);
                AMS_ASSERT(g_i2c_count > 0);
                if ((--g_i2c_count) == 0) {
                    g_i2c_manager.reset();
                }
            }
            {
                std::scoped_lock lk(g_i2c_pcv_mutex);
                AMS_ASSERT(g_i2c_pcv_count > 0);
                if ((--g_i2c_pcv_count) == 0) {
                    g_i2c_pcv_manager.reset();
                }
            }
        }
    }

}
