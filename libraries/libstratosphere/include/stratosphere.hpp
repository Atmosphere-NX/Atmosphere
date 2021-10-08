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

/* libvapours (pulls in util, svc, results). */
#include <vapours.hpp>

/* Libstratosphere diagnostics. */
#include <stratosphere/diag.hpp>

/* Libstratosphere definitions. */
#include <stratosphere/ams/impl/ams_system_thread_definitions.hpp>

/* Libstratosphere-only utility. */
#include <stratosphere/util.hpp>

/* Sadly required shims. */
#include <stratosphere/svc/svc_stratosphere_shims.hpp>

/* Critical modules with no dependencies. */
#include <stratosphere/ams.hpp>
#include <stratosphere/os.hpp>
#include <stratosphere/dd.hpp>
#include <stratosphere/lmem.hpp>
#include <stratosphere/mem.hpp>

/* Pull in all ID definitions from NCM. */
#include <stratosphere/ncm/ncm_ids.hpp>

/* At this point, just include the rest alphabetically. */
/* TODO: Figure out optimal order. */
#include <stratosphere/boot2.hpp>
#include <stratosphere/cal.hpp>
#include <stratosphere/capsrv.hpp>
#include <stratosphere/cfg.hpp>
#include <stratosphere/clkrst.hpp>
#include <stratosphere/cs.hpp>
#include <stratosphere/ddsf.hpp>
#include <stratosphere/dmnt.hpp>
#include <stratosphere/erpt.hpp>
#include <stratosphere/err.hpp>
#include <stratosphere/fatal.hpp>
#include <stratosphere/gpio.hpp>
#include <stratosphere/hid.hpp>
#include <stratosphere/hos.hpp>
#include <stratosphere/htc.hpp>
#include <stratosphere/htcfs.hpp>
#include <stratosphere/htclow.hpp>
#include <stratosphere/htcs.hpp>
#include <stratosphere/i2c.hpp>
#include <stratosphere/init.hpp>
#include <stratosphere/kvdb.hpp>
#include <stratosphere/ldr.hpp>
#include <stratosphere/lr.hpp>
#include <stratosphere/lm.hpp>
#include <stratosphere/ncm.hpp>
#include <stratosphere/nim.hpp>
#include <stratosphere/ns.hpp>
#include <stratosphere/nsd.hpp>
#include <stratosphere/osdbg.hpp>
#include <stratosphere/patcher.hpp>
#include <stratosphere/pcv.hpp>
#include <stratosphere/pgl.hpp>
#include <stratosphere/pinmux.hpp>
#include <stratosphere/powctl.hpp>
#include <stratosphere/psc.hpp>
#include <stratosphere/pm.hpp>
#include <stratosphere/pwm.hpp>
#include <stratosphere/regulator.hpp>
#include <stratosphere/ro.hpp>
#include <stratosphere/settings.hpp>
#include <stratosphere/scs.hpp>
#include <stratosphere/sf.hpp>
#include <stratosphere/sm.hpp>
#include <stratosphere/socket.hpp>
#include <stratosphere/spl.hpp>
#include <stratosphere/sprofile.hpp>
#include <stratosphere/time.hpp>
#include <stratosphere/tipc.hpp>
#include <stratosphere/tma.hpp>
#include <stratosphere/updater.hpp>
#include <stratosphere/usb.hpp>
#include <stratosphere/wec.hpp>

/* Include FS last. */
#include <stratosphere/fs.hpp>
#include <stratosphere/fssrv.hpp>
#include <stratosphere/fssystem.hpp>

/* External modules that we're including. */
#include <stratosphere/rapidjson.hpp>