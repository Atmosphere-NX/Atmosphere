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
#include <stratosphere.hpp>

namespace ams::settings::impl {

    template<typename T, typename Tag>
    class StaticObject final {
        NON_COPYABLE(StaticObject);
        NON_MOVEABLE(StaticObject);
        private:
        StaticObject();
        public:
            static T &Get() {
                /* Declare static instance variables. */
                static constinit util::TypedStorage<T> s_storage = {};
                static constinit bool s_initialized = false;
                static constinit os::SdkMutex s_mutex;

                /* If we haven't already done so, construct the instance. */
                if (AMS_UNLIKELY(!s_initialized)) {
                    std::scoped_lock lk(s_mutex);

                    /* Check that we didn't concurrently construct the instance. */
                    if (AMS_LIKELY(!s_initialized)) {
                        /* Construct the instance. */
                        util::ConstructAt(s_storage);

                        /* Note that we constructed. */
                        s_initialized = true;
                    }
                }

                /* Return the constructed instance. */
                return util::GetReference(s_storage);
            }
    };

}
