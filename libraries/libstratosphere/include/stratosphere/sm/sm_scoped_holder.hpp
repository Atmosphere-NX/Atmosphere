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

#include "sm_api.hpp"

namespace ams::sm {

    /* Utility, for scoped access to libnx services. */
    template<auto Initializer(), void Finalizer()>
    class ScopedServiceHolder {
        NON_COPYABLE(ScopedServiceHolder);
        private:
            Result result;
            bool has_initialized;
        public:
            ScopedServiceHolder(bool initialize = true) : result(ResultSuccess()), has_initialized(false) {
                if (initialize) {
                    this->Initialize();
                }
            }

            ~ScopedServiceHolder() {
                if (this->has_initialized) {
                    this->Finalize();
                }
            }

            ScopedServiceHolder(ScopedServiceHolder&& rhs) {
                this->result = rhs.result;
                this->has_initialized = rhs.has_initialized;
                rhs.result = ResultSuccess();
                rhs.has_initialized = false;
            }

            ScopedServiceHolder &operator=(ScopedServiceHolder&& rhs) {
                rhs.Swap(*this);
                return *this;
            }

            void Swap(ScopedServiceHolder &rhs) {
                std::swap(this->result, rhs.result);
                std::swap(this->has_initialized, rhs.has_initialized);
            }

            explicit operator bool() const {
                return this->has_initialized;
            }

            Result Initialize() {
                AMS_ABORT_UNLESS(!this->has_initialized);

                sm::DoWithSession([&]() {
                    if constexpr (std::is_same<decltype(Initializer()), void>::value) {
                        Initializer();
                        this->result = ResultSuccess();
                    } else {
                        this->result = Initializer();
                    }
                });

                this->has_initialized = R_SUCCEEDED(this->result);
                return this->result;
            }

            void Finalize() {
                AMS_ABORT_UNLESS(this->has_initialized);
                Finalizer();
                this->has_initialized = false;
            }

            Result GetResult() const {
                return this->result;
            }
    };

}
