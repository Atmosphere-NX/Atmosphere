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
#include <stratosphere/powctl/powctl_types.hpp>

namespace ams::powctl::impl {

    constexpr inline const double MinRawDefaultPercentage = 3.0;
    constexpr inline const double MaxRawDefaultPercentage = 99.0;

    constexpr inline const double MinRawThresholdPercentage = 11.0;

    constexpr inline const int MinDisplayPercentage = 1;
    constexpr inline const int MaxDisplayPercentage = 100;

    constexpr inline void CalculateMarginatedRawPercentage(double *out_marginated_min, double *out_marginated_max, double min, double max) {
        /* Ensure minimum is in correct range. */
        min = std::max(std::min(min, MinRawThresholdPercentage), MinRawDefaultPercentage);

        /* Calculate the marginated values. */
        constexpr const double MinMarginPercentage = 0.93359375;
        constexpr const double MaxMarginPercentage = -0.83593750;

        const auto margin_factor = (max - min) / (MaxRawDefaultPercentage - MinRawDefaultPercentage);
        *out_marginated_min = min + MinMarginPercentage * margin_factor;
        *out_marginated_max = max + MaxMarginPercentage * margin_factor;
    }

    constexpr inline int GetDisplayPercentage(double raw_percentage, double min, double max) {
        /* Calculate the display percentage. */
        constexpr const double BaseDisplayPercentage = 2.0;
        const auto display_percentage = BaseDisplayPercentage + ((static_cast<double>(MaxDisplayPercentage - MinDisplayPercentage) * (raw_percentage - min)) / (max - min));

        /* Clamp the display percentage within bounds. */
        return std::max(std::min(static_cast<int>(display_percentage), MaxDisplayPercentage), MinDisplayPercentage);
    }

    constexpr inline int ConvertBatteryChargePercentage(double raw_percentage, double min, double max) {
        /* Marginate the min/max. */
        double marginated_min = 0.0, marginated_max = 0.0;
        CalculateMarginatedRawPercentage(std::addressof(marginated_min), std::addressof(marginated_max), min, max);

        /* Convert to display percentage. */
        return GetDisplayPercentage(raw_percentage, marginated_min, marginated_max);
    }

    constexpr inline int ConvertBatteryChargePercentage(double raw_percentage) {
        return ConvertBatteryChargePercentage(raw_percentage, MinRawDefaultPercentage, MaxRawDefaultPercentage);
    }

}
