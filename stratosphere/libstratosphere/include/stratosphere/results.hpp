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

#pragma once

/* Utilities. */
#include "results/utilities.h"

/* Official. */
#include "results/creport_results.hpp"
#include "results/debug_results.hpp"
#include "results/dmnt_results.hpp"
#include "results/err_results.hpp"
#include "results/fatal_results.hpp"
#include "results/fs_results.hpp"
#include "results/hipc_results.hpp"
#include "results/i2c_results.hpp"
#include "results/kernel_results.hpp"
#include "results/kvdb_results.hpp"
#include "results/loader_results.hpp"
#include "results/lr_results.hpp"
#include "results/ncm_results.hpp"
#include "results/pm_results.hpp"
#include "results/ro_results.hpp"
#include "results/settings_results.hpp"
#include "results/sf_results.hpp"
#include "results/sm_results.hpp"
#include "results/spl_results.hpp"
#include "results/updater_results.hpp"
#include "results/vi_results.hpp"

/* Unofficial. */
#include "results/ams_results.hpp"

static constexpr Result ResultSuccess = 0;
