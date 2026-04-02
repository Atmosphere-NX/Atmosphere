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

/* Patch teardown function call to NOP. */
constexpr inline const EmbeddedPatchEntry AmDisableTeardownPatches_22_0_0[] = {
    { 0x5CC4C, "\x1F\x20\x03\xD5", 4 },
};

constexpr inline const EmbeddedPatch AmDisableTeardownPatches[] = {
    { ParseModuleId("B337F7C5AD53E55F3D920FCD394B4DE7C3B7D606"), util::size(AmDisableTeardownPatches_22_0_0), AmDisableTeardownPatches_22_0_0 }, /* 22.0.0 */
};
