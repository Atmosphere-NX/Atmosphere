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
#include <vapours.hpp>

namespace ams::ddsf::impl {

    template<typename Lock, typename List, typename F>
    inline Result ForEach(Lock &lock, List &list, F f, bool return_on_fail) {
        std::scoped_lock lk(lock);

        Result result = ResultSuccess();
        for (auto && it : list) {
            if (const auto cur_result = f(std::addressof(it)); R_FAILED(cur_result)) {
                if (return_on_fail) {
                    return cur_result;
                } else if (R_SUCCEEDED(result)) {
                    result = cur_result;
                }
            }
        }
        return result;
    }

    template<typename List, typename F, typename Lock>
    inline int ForEach(Lock &lock, List &list, F f) {
        std::scoped_lock lk(lock);

        int success_count = 0;
        for (auto && it : list) {
            if (!f(it)) {
                return success_count;
            }
            ++success_count;
        }
        return success_count;
    }

}
