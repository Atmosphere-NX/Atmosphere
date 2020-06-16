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

using __ACCESS_TABLE_NAME__ = AccessTable<__ACCESS_TABLE_ADDRESS__, [] {
        /* Declare a table. */
        std::array<u8, 0x80> table = {};

        /* Declare a helper. */
        auto SetRegisterAllowed = [&](uintptr_t reg) { SetRegisterTableAllowed(table, reg); };

        /* Populate the table. */
        #include __ACCESS_TABLE_INC__

        return table;
}()>;

static_assert(__ACCESS_TABLE_NAME__::Address >= __ACCESS_TABLE_ADDRESS__);
