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

namespace ams::init {

    WEAK_SYMBOL void InitializeSystemModuleBeforeConstructors() {
        /* This should only be used in exceptional circumstances. */
    }

    WEAK_SYMBOL void InitializeSystemModule() {
        /* TODO: What should we do here, if anything? */
        /* Nintendo does nndiagStartup(); nn::diag::InitializeSystemProcessAbortObserver(); */
    }

    WEAK_SYMBOL void FinalizeSystemModule() {
        /* Do nothing by default. */
    }

    WEAK_SYMBOL void Startup() {
        /* TODO: What should we do here, if anything? */
        /* Nintendo determines heap size and does init::InitializeAllocator, as relevant. */
    }

}