/*
 * Copyright (c) 2018-2019 Atmosphère-NX
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

namespace ams::result {

    extern bool CallFatalOnResultAssertion;

}

namespace ams::result::impl {

    NORETURN WEAK void OnResultAssertion(Result result) {
        /* Assert that we should call fatal on result assertion. */
        /* If we shouldn't fatal, this will std::abort(); */
        /* If we should, we'll continue onwards. */
        AMS_ASSERT((ams::result::CallFatalOnResultAssertion));

        /* TODO: ams::fatal:: */
        fatalThrow(result.GetValue());
        while (true) { /* ... */ }
    }

}