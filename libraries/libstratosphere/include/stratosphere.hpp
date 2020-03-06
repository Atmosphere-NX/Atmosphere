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

/* libvapours (pulls in util, svc, results). */
#include <vapours.hpp>

/* Libstratosphere-only utility. */
#include "stratosphere/util.hpp"

/* Critical modules with no dependencies. */
#include "stratosphere/ams.hpp"
#include "stratosphere/os.hpp"
#include "stratosphere/dd.hpp"
#include "stratosphere/lmem.hpp"

/* Pull in all ID definitions from NCM. */
#include "stratosphere/ncm/ncm_ids.hpp"

/* At this point, just include the rest alphabetically. */
/* TODO: Figure out optimal order. */
#include "stratosphere/boot2.hpp"
#include "stratosphere/cfg.hpp"
#include "stratosphere/dmnt.hpp"
#include "stratosphere/fatal.hpp"
#include "stratosphere/hid.hpp"
#include "stratosphere/hos.hpp"
#include "stratosphere/kvdb.hpp"
#include "stratosphere/ldr.hpp"
#include "stratosphere/lr.hpp"
#include "stratosphere/map.hpp"
#include "stratosphere/ncm.hpp"
#include "stratosphere/patcher.hpp"
#include "stratosphere/pm.hpp"
#include "stratosphere/reg.hpp"
#include "stratosphere/ro.hpp"
#include "stratosphere/settings.hpp"
#include "stratosphere/sf.hpp"
#include "stratosphere/sm.hpp"
#include "stratosphere/spl.hpp"
#include "stratosphere/updater.hpp"

/* Include FS last. */
#include "stratosphere/fs.hpp"
#include "stratosphere/fssrv.hpp"
#include "stratosphere/fssystem.hpp"
