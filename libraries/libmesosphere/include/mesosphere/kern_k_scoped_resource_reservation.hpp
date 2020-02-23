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
#include <mesosphere/kern_common.hpp>
#include <mesosphere/kern_k_resource_limit.hpp>
#include <mesosphere/kern_k_process.hpp>

namespace ams::kern {

    class KScopedResourceReservation {
        private:
            KResourceLimit *limit;
            s64 value;
            ams::svc::LimitableResource resource;
            bool succeeded;
        public:
            ALWAYS_INLINE KScopedResourceReservation(KResourceLimit *l, ams::svc::LimitableResource r, s64 v, s64 timeout) : limit(l), value(v), resource(r) {
                if (this->limit && this->value) {
                    this->succeeded = this->limit->Reserve(this->resource, this->value, timeout);
                } else {
                    this->succeeded = true;
                }
            }

            ALWAYS_INLINE KScopedResourceReservation(KResourceLimit *l, ams::svc::LimitableResource r, s64 v = 1) : limit(l), value(v), resource(r) {
                if (this->limit && this->value) {
                    this->succeeded = this->limit->Reserve(this->resource, this->value);
                } else {
                    this->succeeded = true;
                }
            }

            ALWAYS_INLINE KScopedResourceReservation(const KProcess *p, ams::svc::LimitableResource r, s64 v, s64 t) : KScopedResourceReservation(p->GetResourceLimit(), r, v, t) { /* ... */ }
            ALWAYS_INLINE KScopedResourceReservation(const KProcess *p, ams::svc::LimitableResource r, s64 v = 1) : KScopedResourceReservation(p->GetResourceLimit(), r, v) { /* ... */ }

            ALWAYS_INLINE ~KScopedResourceReservation() {
                if (this->limit && this->value && this->succeeded) {
                    this->limit->Release(this->resource, this->value);
                }
            }

            ALWAYS_INLINE void Commit() {
                this->limit = nullptr;
            }

            ALWAYS_INLINE bool Succeeded() const {
                return this->succeeded;
            }
    };

}
