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
#include <stratosphere/ams/ams_environment.hpp>

#define RAPIDJSON_NAMESPACE          ams::rapidjson
#define RAPIDJSON_ASSERT(x)          AMS_ABORT_UNLESS(x)

#define RAPIDJSON_MALLOC(size)       ams::MallocForRapidJson(size)
#define RAPIDJSON_REALLOC(ptr, size) ams::ReallocForRapidJson(ptr, size)
#define RAPIDJSON_FREE(ptr)          ams::FreeForRapidJson(ptr)

#include <stratosphere/rapidjson/rapidjson.h>
#include <stratosphere/rapidjson/encodings.h>
#include <stratosphere/rapidjson/reader.h>
