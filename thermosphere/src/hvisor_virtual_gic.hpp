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
#include "hvisor_core_context.hpp"
#include "cpu/hvisor_cpu_exception_sysregs.hpp"
#include "hvisor_irq_manager.hpp"
#include "memory_map.h"

namespace ams::hvisor {

    class VirtualGic final : public IInterruptTask {
        SINGLETON_WITH_ATTRS(VirtualGic, TEMPORARY);

        private:
            // For convenience, although they're already defined in irq manager header:
            static inline volatile auto *const gicd = IrqManager::gicd;
            static inline volatile auto *const gich = IrqManager::gich;

            // Architectural properties
            static constexpr u32 priorityShift = 3;
            static constexpr u32 numPriorityLevels = BIT(8 - priorityShift);
            static constexpr u32 lowestPriority = numPriorityLevels - 1;

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
                bool edgeTriggered  : 1;
                u32 coreId          : 3;
                u32 targetList      : 8;
                u32 srcCoreId       : 3;
                bool enabled        : 1;
                u64                 : 0;

                constexpr bool IsPending() const
                {
                    return pendingLatch || (!edgeTriggered && pending);
                }
                constexpr void SetPending()
                {
                    if (!edgeTriggered) {
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
                constexpr bool ClearPendingOnAck()
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

                        iterator insert(iterator pos, VirqState &elem);
                        iterator insert(VirqState &elem);

                        constexpr iterator erase(iterator startPos, iterator endPos)
                        {
                            VirqState &prev = m_storage[startPos->listPrev];
                            VirqState &next = *endPos;
                            u32 nextPos = GetStateIndex(*endPos);

                            ENSURE(startPos->IsQueued());
                            if (startPos->listPrev != virqListEndIndex) {
                                prev.listNext = nextPos;
                            } else {
                                m_first = &next;
                            }

                            if (nextPos != virqListEndIndex) {
                                next.listPrev = startPos->listPrev;
                            } else {
                                m_last = &prev;
                            }

                            for (iterator it = startPos; it != endPos; ++it) {
                                it->listPrev = it->listNext = virqListInvalidIndex;
                            }
                        }

                        constexpr iterator erase(iterator pos) { return erase(pos, std::next(pos)); }
                        constexpr iterator erase(VirqState &pos) { return erase(iterator{&pos, m_storage}); }

                        template<typename Pred>
                        void erase_if(Pred p)
                        {
                            for (iterator it = begin(); l = end(); i != l) {
                                if(p(*it)) {
                                    it = erase(it);
                                } else {
                                    ++it;
                                }
                            }
                        }

                        constexpr void Initialize(VirqState *storage)
                        {
                            m_storage = storage;
                            m_first = m_last = &(*end());
                        }
            };


            private:
                static void NotifyOtherCoreList(u32 coreList)
                {
                    coreList &= ~BIT(currentCoreCtx->GetCoreId());
                    if (coreList != 0) {
                        IrqManager::GenerateSgiForList(IrqManager::VgicUpdateSgi, coreList);
                    }
                }

                static void NotifyAllOtherCores()
                {
                    IrqManager::GenerateSgiForAllOthers(IrqManager::VgicUpdateSgi);
                }

                static u64 GetEmptyListStatusRegister()
                {
                    return static_cast<u64>(gich->elsr1) << 32 | static_cast<u64>(gich->elsr0);
                }

                static u64 GetNumberOfFreeListRegisters()
                {
                    return __builtin_popcountll(GetEmptyListStatusRegister());
                }

            private:
                std::array<VirqState, maxNumIntStates> m_virqStates{};
                std::array<std::array<u8, 32>, MAX_CORE> m_incomingSgiPendingSources{};
                std::array<u64, MAX_CORE> m_usedLrMap{};

                VirqQueue m_virqPendingQueue{};
                bool m_distributorEnabled = false;

                u8 m_numListRegisters = 0;
            private:
                constexpr VirqState &GetVirqState(u32 coreId, u32 id)
                {
                    if (id >= 32) {
                        return m_virqStates[id - 32];
                    } else if (id <= GicV2Distributor::maxIrqId) {
                        return m_virqStates[spiEndIndex + 32 * coreId + id];
                    }
                }

                VirqState &GetVirqState(u32 id) { return GetVirqState(currentCoreCtx->GetCoreId(), id); }

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

                u32 GetDistributorControlRegister(void)
                {
                    return m_distributorEnabled ? 1 : 0;
                }

                u32 GetDistributorTypeRegister(void)
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
                    return id < 16 || (IrqManager::IsGuestInterrupt(id) && GetVirqState(currentCoreCtx->GetCoreId(), id).enabled);
                }

                u8 GetInterruptPriorityByte(u32 id)
                {
                    return IrqManager::IsGuestInterrupt(id) ? GetVirqState(currentCoreCtx->GetCoreId(), id).priority << priorityShift : 0;
                }

                u8 GetInterruptTargets(u16 id)
                {
                    return id < 32 || (IrqManager::IsGuestInterrupt(id) && GetVirqState(currentCoreCtx->GetCoreId(), id).targetList);
                }

                u32 GetInterruptConfigBits(u16 id)
                {
                    u32 oneNModel = id < 32 || !IrqManager::IsGuestInterrupt(id) ? 0 : 1;
                    return (IrqManager::IsGuestInterrupt(id) && GetVirqState(id).edgeTriggered) ? 2 | oneNModel : oneNModel;
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

                void ResampleVirqLevel(VirqState &state);
                void CleanupPendingQueue();
                size_t ChoosePendingInterrupts(VirqState *chosen[], size_t maxNum);

                volatile GicV2VirtualInterfaceController::ListRegister *AllocateListRegister(void)
                {
                    u32 ff = __builtin_ffsll(GetEmptyListStatusRegister());
                    if (ff == 0) {
                        return nullptr;
                    } else {
                        m_usedLrMap[currentCoreCtx->GetCoreId()] |= BITL(ff - 1);
                        return &gich->lr[ff - 1];
                    }
                }

                void PushListRegisters(VirqState *chosen[], size_t num);
                bool UpdateListRegister(volatile GicV2VirtualInterfaceController::ListRegister *lr);

            private:
                constexpr VirtualGic() = default;

            public:
                // For convenience (when trapping lower-el data aborts):
                static constexpr uintptr_t gicdPhysicalAddress = 0; // fixme pls MEMORY_MAP_PA_GICD;
            public:
                static bool ValidateGicdRegisterAccess(size_t offset, size_t sz);
            public:
                void WriteGicdRegister(u32 val, size_t offset, size_t sz);
                u32 ReadGicdRegister(size_t offset, size_t sz);

                // Must be called by irqManager only...
                // not sure if I should have made IrqManager a friend of this class
                void UpdateState();

                std::optional<bool> InterruptTopHalfHandler(u32 irqId, u32) final;

                void EnqueuePhysicalIrq(u32 id);
                void Initialize();
    };
}
