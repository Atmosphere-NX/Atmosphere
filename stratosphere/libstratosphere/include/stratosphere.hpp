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

#include "stratosphere/defines.hpp"

#include "stratosphere/utilities.hpp"
#include "stratosphere/emummc_utilities.hpp"

#include "stratosphere/scope_guard.hpp"

#include "stratosphere/version_check.hpp"

#include "stratosphere/auto_handle.hpp"
#include "stratosphere/hossynch.hpp"
#include "stratosphere/message_queue.hpp"
#include "stratosphere/iwaitable.hpp"
#include "stratosphere/event.hpp"

#include "stratosphere/waitable_manager.hpp"

#include "stratosphere/ipc.hpp"

#include "stratosphere/mitm.hpp"

#include "stratosphere/services.hpp"

#include "stratosphere/results.hpp"

#include "stratosphere/on_crash.hpp"

#include "stratosphere/svc.hpp"
#include "stratosphere/cfg.hpp"
#include "stratosphere/fatal.hpp"
#include "stratosphere/hid.hpp"
#include "stratosphere/ncm.hpp"
#include "stratosphere/pm.hpp"
#include "stratosphere/rnd.hpp"
#include "stratosphere/sm.hpp"
#include "stratosphere/util.hpp"