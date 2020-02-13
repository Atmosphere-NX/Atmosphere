/*
 * Copyright (c) 2019-2020 Atmosphère-NX
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

#include "defines.hpp"
#include "exceptions.h"
#include "cpu/hvisor_cpu_exception_sysregs.hpp"
#include "hvisor_irq_manager.hpp"
#include "memory_map.h"

namespace ams::hvisor {

    class VirtualGic final {
        SINGLETON(VirtualGic);

        private:
            // For convenience, although they're already defined in irq manager header:
            static inline volatile auto *const gicd = IrqManager::gicd;
            static inline volatile auto *const gich = IrqManager::gich;

            // Architectural properties
            static constexpr u32 priorityShift = 3;

            // List managament constants
            static constexpr u32 spiEndIndex = GicV2Distributor::maxIrqId + 1 - 32;
            static constexpr u32 maxNumIntStates = spiEndIndex + MAX_CORE * 32;
            static constexpr u32 virqListEndIndex = maxNumIntStates;
            static constexpr u32 virqListInvalidIndex = virqListEndIndex + 1;

        private:
            struct VirqState {
                u32 listPrev        : 11;
                u32 listNext        : 11;
                u32 irqId           : 10;
                u32 priority        : 5;
                bool pending        : 1;
                bool active         : 1;
                bool handled        : 1;
                bool pendingLatch   : 1;
                bool levelSensitive : 1;
                u32 coreId          : 3;
                u32 targetList      : 8;
                u32 srcCoreId       : 3;
                bool enabled        : 1;
                u64                 : 0;

                constexpr bool IsPending() const
                {
                    return pendingLatch || (levelSensitive && pending);
                }
                constexpr void SetPending()
                {
                    if (levelSensitive) {
                        pending = true;
                    } else {
                        pendingLatch = true;
                    }
                }
                constexpr bool ClearPendingLine()
                {
                    // Don't clear pending latch status
                    pending = false;
                }
                constexpr bool ClearPending()
                {
                    // On ack, both pending line status and latch are cleared
                    pending = false;
                    pendingLatch = false;
                }
                constexpr bool IsQueued() const
                {
                    return listPrev != virqListInvalidIndex && listNext != virqListInvalidIndex;
                }
            };


            class VirqQueue final {
                private:
                    VirqState *m_first = nullptr;
                    VirqState *m_last = nullptr;
                    VirqState *m_storage = nullptr;
                public:
                    template<bool isConst>
                    class Iterator {
                        friend class Iterator<true>;
                        friend class VirqQueue;
                        private:
                            VirqState *m_node = nullptr;
                            VirqState *m_storage = nullptr;

                        private:
                            explicit constexpr Iterator(VirqState *node, VirqState *storage) : m_node{node}, m_storage{storage} {}

                        public:
                            // allow implicit const->non-const
                            constexpr Iterator(const Iterator<false> &other) : m_node{other.m_storage}, m_storage{other.m_storage} {}
                            constexpr Iterator() = default;

                        public:
                            using iterator_category = std::bidirectional_iterator_tag;
                            using value_type = VirqState;
                            using difference_type = ptrdiff_t;
                            using pointer = typename std::conditional<isConst, const VirqState *, VirqState *>::type;
                            using reference = typename std::conditional<isConst, const VirqState &, VirqState &>::type;

                            constexpr bool operator==(const Iterator &other) const { return m_node == other.m_node; }
                            constexpr bool operator!=(const Iterator &other) const { return !(*this == other); }
                            constexpr reference operator*() { return *m_node; }
                            constexpr pointer operator->() { return m_node; }

                            constexpr Iterator &operator++()
                            {
                                m_node = &m_storage[m_node->listNext];
                                return *this;
                            }

                            constexpr Iterator &operator--()
                            {
                                m_node = &m_storage[m_node->listPrev];
                                return *this;
                            }

                            constexpr Iterator &operator++(int)
                            {
                                const Iterator v{*this};
                                ++(*this);
                                return v;
                            }

                            constexpr Iterator &operator--(int)
                            {
                                const Iterator v{*this};
                                --(*this);
                                return v;
                            }
                    };

                    private:
                        constexpr u32 GetStateIndex(VirqState &elem) { return static_cast<u32>(&elem - &m_storage[0]); }
                    public:
                        using pointer = VirqState *;
                        using const_pointer = const VirqState *;
                        using reference = VirqState &;
                        using const_reference = const VirqState &;
                        using value_type = VirqState;
                        using size_type = size_t;
                        using difference_type = ptrdiff_t;
                        using iterator = Iterator<false>;
                        using const_iterator = Iterator<true>;
                        using reverse_iterator = std::reverse_iterator<iterator>;
                        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

                        constexpr void Initialize(VirqState *storage) { m_storage = storage; }

                        constexpr VirqState &front() { return *m_first; };
                        constexpr const VirqState &front() const { return *m_first; };

                        constexpr VirqState &back() { return *m_last; };
                        constexpr const VirqState &back() const { return *m_last; };

                        constexpr const_iterator cbegin() const { return const_iterator{m_first, m_storage}; }
                        constexpr const_iterator cend() const { return const_iterator{&m_storage[virqListEndIndex], m_storage}; }

                        constexpr const_iterator begin() const { return cbegin(); }
                        constexpr const_iterator end() const { return cend(); }

                        constexpr iterator begin() { return iterator{m_first, m_storage}; }
                        constexpr iterator end() { return iterator{&m_storage[virqListEndIndex], m_storage}; }

                        constexpr const_reverse_iterator crbegin() const {
                            return const_reverse_iterator{const_iterator{m_last, m_storage}};
                        }
                        constexpr const_reverse_iterator crend() const { return const_reverse_iterator{cend()}; }

                        constexpr const_reverse_iterator rbegin() const { return crbegin(); }
                        constexpr const_reverse_iterator rend() const { return crend(); }

                        constexpr reverse_iterator rbegin() { return reverse_iterator{iterator{m_first, m_storage}}; }
                        constexpr reverse_iterator rend() { return reverse_iterator{end()}; }


                        iterator insert(iterator pos, VirqState &elem);
                        iterator insert(VirqState &elem);

                        iterator erase(iterator startPos, iterator endPos);

                        iterator erase(iterator pos) { return erase(pos, std::next(pos)); }
            };


            private:
                static void NotifyOtherCoreList(u32 coreList)
                {
                    coreList &= ~BIT(currentCoreCtx->coreId);
                    if (coreList != 0) {
                        IrqManager::GenerateSgiForList(IrqManager::VgicUpdateSgi, coreList);
                    }
                }

                static void NotifyAllOtherCores()
                {
                    IrqManager::GenerateSgiForAllOthers(IrqManager::VgicUpdateSgi);
                }


            private:
                std::array<VirqState, maxNumIntStates> m_virqStates{};
                std::array<std::array<u8, 32>, MAX_CORE> m_incomingSgiPendingSources{};

                VirqQueue m_virqPendingQueue{};
                bool m_distributorEnabled = false;

            private:

                constexpr VirqState &GetVirqState(u32 coreId, u32 id)
                {
                    if (id >= 32) {
                        return m_virqStates[id - 32];
                    } else if (id <= GicV2Distributor::maxIrqId) {
                        return m_virqStates[spiEndIndex + 32 * coreId + id];
                    }
                }

                VirqState &GetVirqState(u32 id) { return GetVirqState(currentCoreCtx->coreId, id); }

                void SetDistributorControlRegister(u32 value)
                {
                    // We implement a virtual distributor/interface w/o security extensions.
                    // Moreover, we forward all interrupts as Group 0 so that non-secure code that assumes GICv2
                    // *with* security extensions (and thus all interrupts fw as group 1 there) still works (bit are in the same positions).

                    // We don't implement Group 1 interrupts, either (so that's similar to GICv1).
                    bool old = m_distributorEnabled;
                    m_distributorEnabled = (value & 1) != 0;

                    // Enable bit is actually just a global enable bit for all irq forwarding, other functions of the GICD aren't affected by it
                    if (old != m_distributorEnabled) {
                        NotifyAllOtherCores();
                    }
                }

                u32 vgicGetDistributorControlRegister(void)
                {
                    return m_distributorEnabled ? 1 : 0;
                }

                u32 vgicGetDistributorTypeRegister(void)
                {
                    // See above comment.
                    // Therefore, LSPI = 0, SecurityExtn = 0, rest = from physical distributor
                    return IrqManager::GetTypeRegister() & 0x7F;
                }

                u32 GetDistributorImplementerIdentificationRegister(void)
                {
                    u32 iidr = 'A' << 24; // Product Id: Atmosphère (?)
                    iidr |= 2 << 16;    // Major revision 2 (GICv2)
                    iidr |= 0 << 12;    // Minor revision 0
                    iidr |= 0x43B;      // Implementer: Arm (value copied from physical GICD)
                    return iidr;
                }

                bool GetInterruptEnabledState(u32 id)
                {
                    // SGIs are always enabled
                    return id < 16 || (IrqManager::IsGuestInterrupt(id) && GetVirqState(currentCoreCtx->coreId, id).enabled);
                }

                u8 GetInterruptPriorityByte(u32 id)
                {
                    return IrqManager::IsGuestInterrupt(id) ? GetVirqState(currentCoreCtx->coreId, id).priority << priorityShift : 0;
                }

                u8 GetInterruptTargets(u16 id)
                {
                    return id < 32 || (IrqManager::IsGuestInterrupt(id) && GetVirqState(currentCoreCtx->coreId, id).targetList);
                }

                u32 GetInterruptConfigBits(u16 id)
                {
                    u32 oneNModel = id < 32 || !IrqManager::IsGuestInterrupt(id) ? 0 : 1;
                    return (IrqManager::IsGuestInterrupt(id) && !GetVirqState(id).levelSensitive) ? 2 | oneNModel : oneNModel;
                }

                u32 GetPeripheralId2Register(void)
                {
                    return 2u << 4;
                }

                void SetInterruptEnabledState(u32 id);
                void ClearInterruptEnabledState(u32 id);
                void SetInterruptPriorityByte(u32 id, u8 priority);
                void SetInterruptTargets(u32 id, u8 coreList);
                void SetInterruptConfigBits(u32 id, u32 config);
                void SetSgiPendingState(u32 id, u32 coreId, u32 srcCoreId);
                void SendSgi(u32 id, GicV2Distributor::SgirTargetListFilter filter, u32 coreList);

    };
}


/*bool vgicValidateGicdRegisterAccess(size_t offset, size_t sz);
void vgicWriteGicdRegister(u32 val, size_t offset, size_t sz);
u32 vgicReadGicdRegister(size_t offset, size_t sz);

void handleVgicdMmio(ExceptionStackFrame *frame, cpu::DataAbortIss dabtIss, size_t offset);

void vgicInit(void);
void vgicUpdateState(void);
void vgicMaintenanceInterruptHandler(void);
void vgicEnqueuePhysicalIrq(u16 irqId);*/
