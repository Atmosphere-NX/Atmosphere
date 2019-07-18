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
#include <algorithm>
#include <cstring>
#include <switch.h>
#include "../defines.hpp"
#include "../results.hpp"

namespace sts::sm {

    struct ServiceName {
        static constexpr size_t MaxLength = 8;

        char name[MaxLength];

        static constexpr ServiceName Encode(const char *name, size_t name_size) {
            ServiceName out{};

            for (size_t i = 0; i < MaxLength; i++) {
                if (i < name_size) {
                    out.name[i] = name[i];
                } else {
                    out.name[i] = 0;
                }
            }

            return out;
        }

        static constexpr ServiceName Encode(const char *name) {
            return Encode(name, std::strlen(name));
        }
    };
    static constexpr ServiceName InvalidServiceName = ServiceName::Encode("");
    static_assert(alignof(ServiceName) == 1, "ServiceName definition!");

    inline bool operator==(const ServiceName &lhs, const ServiceName &rhs) {
        return std::memcmp(&lhs, &rhs, sizeof(ServiceName)) == 0;
    }

    inline bool operator!=(const ServiceName &lhs, const ServiceName &rhs) {
        return !(lhs == rhs);
    }

    /* For Debug Monitor extensions. */
    struct ServiceRecord {
        ServiceName service;
        u64 owner_pid;
        u64 max_sessions;
        u64 mitm_pid;
        u64 mitm_waiting_ack_pid;
        bool is_light;
        bool mitm_waiting_ack;
    };
    static_assert(sizeof(ServiceRecord) == 0x30, "ServiceRecord definition!");

    /* For process validation. */
    static constexpr u64 InvalidProcessId = static_cast<u64>(-1ull);

    /* Utility, for scoped access to libnx services. */
    template<Result Initializer(), void Finalizer()>
    class ScopedServiceHolder {
        NON_COPYABLE(ScopedServiceHolder);
        private:
            Result result;
            bool has_initialized;
        public:
            ScopedServiceHolder(bool initialize = true) : result(ResultSuccess), has_initialized(false) {
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
                rhs.result = ResultSuccess;
                rhs.has_initialized = false;
            }

            ScopedServiceHolder& operator=(ScopedServiceHolder&& rhs) {
                rhs.Swap(*this);
                return *this;
            }

            void Swap(ScopedServiceHolder& rhs) {
                std::swap(this->result, rhs.result);
                std::swap(this->has_initialized, rhs.has_initialized);
            }

            explicit operator bool() const {
                return this->has_initialized;
            }

            Result Initialize() {
                if (this->has_initialized) {
                    std::abort();
                }

                DoWithSmSession([&]() {
                    this->result = Initializer();
                });

                this->has_initialized = R_SUCCEEDED(this->result);
                return this->result;
            }

            void Finalize() {
                if (!this->has_initialized) {
                    std::abort();
                }
                Finalizer();
                this->has_initialized = false;
            }

            Result GetResult() const {
                return this->result;
            }
    };

}
