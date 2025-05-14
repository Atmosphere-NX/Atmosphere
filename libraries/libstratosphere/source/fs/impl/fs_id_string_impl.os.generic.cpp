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
#include <exosphere/pkg1.hpp>

namespace ams::fs::impl {

    #define ADD_ENUM_CASE(v) case v: return #v

    template<> const char *IdString::ToString<pkg1::KeyGeneration>(pkg1::KeyGeneration id) {
        static_assert(pkg1::KeyGeneration_Current == pkg1::KeyGeneration_20_0_0);
        switch (id) {
            using enum pkg1::KeyGeneration;
            case KeyGeneration_1_0_0:  return "1.0.0-2.3.0";
            case KeyGeneration_3_0_0:  return "3.0.0";
            case KeyGeneration_3_0_1:  return "3.0.1-3.0.2";
            case KeyGeneration_4_0_0:  return "4.0.0-4.1.0";
            case KeyGeneration_5_0_0:  return "5.0.0-5.1.0";
            case KeyGeneration_6_0_0:  return "6.0.0-6.1.0";
            case KeyGeneration_6_2_0:  return "6.2.0";
            case KeyGeneration_7_0_0:  return "7.0.0-8.0.1";
            case KeyGeneration_8_1_0:  return "8.1.0-8.1.1";
            case KeyGeneration_9_0_0:  return "9.0.0-9.0.1";
            case KeyGeneration_9_1_0:  return "9.1.0-12.0.3";
            case KeyGeneration_12_1_0: return "12.1.0";
            case KeyGeneration_13_0_0: return "13.0.0-13.2.1";
            case KeyGeneration_14_0_0: return "14.0.0-14.1.2";
            case KeyGeneration_15_0_0: return "15.0.0-15.0.1";
            case KeyGeneration_16_0_0: return "16.0.0-16.0.3";
            case KeyGeneration_17_0_0: return "17.0.0-17.0.1";
            case KeyGeneration_18_0_0: return "18.0.0-18.1.0";
            case KeyGeneration_19_0_0: return "19.0.0-19.0.1";
            case KeyGeneration_20_0_0: return "20.0.0-";
            default: return "Unknown";
        }
    }

}
