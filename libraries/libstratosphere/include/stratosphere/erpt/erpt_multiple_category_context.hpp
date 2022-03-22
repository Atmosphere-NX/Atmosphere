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
#pragma once
#include <vapours.hpp>
#include <stratosphere/erpt/erpt_types.hpp>
#include <stratosphere/sf/sf_buffer_tags.hpp>

namespace ams::erpt {

    constexpr inline u32 CategoriesPerMultipleCategoryContext = 0x10;
    constexpr inline u32 FieldsPerMultipleCategoryContext     = CategoriesPerMultipleCategoryContext * 4;

    struct MultipleCategoryContextEntry : public sf::LargeData, public sf::PrefersMapAliasTransferMode {
        u32 version;
        u32 category_count;
        CategoryId categories[CategoriesPerMultipleCategoryContext];
        u32 field_counts[CategoriesPerMultipleCategoryContext];
        u32 array_buf_counts[CategoriesPerMultipleCategoryContext];
        FieldEntry fields[FieldsPerMultipleCategoryContext];
    };

}
