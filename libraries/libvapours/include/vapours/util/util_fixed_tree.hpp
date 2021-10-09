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
#include <vapours/common.hpp>
#include <vapours/assert.hpp>
#include <vapours/util/impl/util_available_index_finder.hpp>
#include <vapours/util/util_alignment.hpp>

namespace ams::util {

    template<typename Member, typename Compare, typename IteratorMember, size_t BufferAlignment = 8> requires std::convertible_to<Member &, IteratorMember &>
    class FixedTree {
        private:
            class IteratorBase;
            friend class IteratorBase;
        private:
            enum class Color : u8 {
                Red   = 0,
                Black = 1,
            };

            static constexpr inline int Index_Nil         = -1;
            static constexpr inline int Index_Leaf        = -2;
            static constexpr inline int Index_BeforeBegin = -3;
            static constexpr inline int Index_AfterEnd    = -4;

            static constexpr inline size_t max_size = 0x40000000;

            struct Header {
                /* "Nintendo Red-Black tree" */
                static constexpr u32 Signature = util::ReverseFourCC<'N','N','R','B'>::Code;

                u32 header_size;
                u32 header_signature;
                u32 _08;
                s32 max_elements;
                s32 cur_elements;
                s32 root_index;
                s32 left_most_index;
                s32 right_most_index;
                s32 index_signature;
                u32 buffer_size;
                u32 node_size;
                u32 element_size;
                u32 _30;
                u32 _34;
                u32 _38;
                u32 _3C;
                u32 _40;
                u32 _44;
                u32 _48;
                u32 _4C;

                void InitializeHeader(u32 _08, s32 max_e, s32 cur_e, u32 ind_sig, u32 buf_sz, u32 node_sz, u32 e_sz, u32 _30, u32 _34, u32 _38, u32 _3C, u32 _40, u32 _44) {
                    this->header_size      = sizeof(Header);
                    this->header_signature = Signature;
                    this->_08              = _08;
                    this->max_elements     = max_e;
                    this->cur_elements     = cur_e;
                    this->root_index       = Index_Nil;
                    this->left_most_index  = Index_Nil;
                    this->right_most_index = Index_Nil;
                    this->index_signature  = ind_sig;
                    this->buffer_size      = buf_sz;
                    this->node_size        = node_sz;
                    this->element_size     = e_sz;
                    this->_30              = _30;
                    this->_34              = _34;
                    this->_38              = _38;
                    this->_3C              = _3C;
                    this->_40              = _40;
                    this->_44              = _44;
                    this->_48              = 0;
                    this->_4C              = 0;
                }
            };
            static_assert(sizeof(Header) == 0x50);

            struct IndexPair {
                int first;
                int last;
            };

            struct Node {
                Member m_data;
                int m_parent;
                int m_right;
                int m_left;
                Color m_color;

                void SetLeft(int l, Node *n, int p) {
                    m_left      = l;
                    n->m_parent = p;
                }

                void SetRight(int r, Node *n, int p) {
                    m_right     = r;
                    n->m_parent = p;
                }
            };

            class Iterator;
            class ConstIterator;

            class IteratorBase {
                private:
                    friend class ConstIterator;
                private:
                    const FixedTree *m_tree;
                    int m_index;
                protected:
                    constexpr ALWAYS_INLINE IteratorBase(const FixedTree *tree, int index) : m_tree(tree), m_index(index) { /* ... */ }

                    constexpr bool IsEqualImpl(const IteratorBase &rhs) const {
                        /* Validate pre-conditions. */
                        AMS_ASSERT(m_tree);

                        /* Check for tree equality. */
                        if (m_tree != rhs.m_tree) {
                            return false;
                        }

                        /* Check for nil. */
                        if (m_tree->IsNil(m_index) && m_tree->IsNil(rhs.m_index)) {
                            return true;
                        }

                        /* Check for index equality. */
                        return m_index == rhs.m_index;
                    }

                    constexpr IteratorMember &DereferenceImpl() const {
                        /* Validate pre-conditions. */
                        AMS_ASSERT(m_tree);

                        if (!m_tree->IsNil(m_index)) {
                            return m_tree->m_nodes[m_index].m_data;
                        } else {
                            AMS_ASSERT(false);
                            return m_tree->GetNode(std::numeric_limits<int>::max())->m_data;
                        }
                    }

                    constexpr ALWAYS_INLINE IteratorBase &IncrementImpl() {
                        /* Validate pre-conditions. */
                        AMS_ASSERT(m_tree);

                        this->OperateIndex(true);
                        return *this;
                    }

                    constexpr ALWAYS_INLINE IteratorBase &DecrementImpl() {
                        /* Validate pre-conditions. */
                        AMS_ASSERT(m_tree);

                        this->OperateIndex(false);
                        return *this;
                    }

                    constexpr void OperateIndex(bool increment) {
                        if (increment) {
                            /* We're incrementing. */
                            if (m_index == Index_BeforeBegin) {
                                m_index = 0;
                            } else {
                                m_index = m_tree->UncheckedPP(m_index);
                                if (m_tree->IsNil(m_index)) {
                                    m_index = Index_AfterEnd;
                                }
                            }
                        } else {
                            /* We're decrementing. */
                            if (m_index == Index_AfterEnd) {
                                m_index = static_cast<int>(m_tree->size()) - 1;
                            } else {
                                m_index = m_tree->UncheckedMM(m_index);
                                if (m_tree->IsNil(m_index)) {
                                    m_index = Index_BeforeBegin;
                                }
                            }
                        }
                    }
            };

            class Iterator : public IteratorBase {
                public:
                    constexpr ALWAYS_INLINE Iterator(const FixedTree &tree) : IteratorBase(std::addressof(tree), tree.size() ? tree.GetLMost() : Index_Leaf) { /* ... */ }
                    constexpr ALWAYS_INLINE Iterator(const FixedTree &tree, int index) : IteratorBase(std::addressof(tree), index) { /* ... */ }

                    constexpr ALWAYS_INLINE Iterator(const Iterator &rhs) = default;

                    constexpr ALWAYS_INLINE bool operator==(const Iterator &rhs) const {
                        return this->IsEqualImpl(rhs);
                    }

                    constexpr ALWAYS_INLINE bool operator!=(const Iterator &rhs) const {
                        return !(*this == rhs);
                    }

                    constexpr ALWAYS_INLINE IteratorMember &operator*() const {
                        return static_cast<IteratorMember &>(this->DereferenceImpl());
                    }

                    constexpr ALWAYS_INLINE IteratorMember *operator->() const {
                        return std::addressof(this->operator *());
                    }

                    constexpr ALWAYS_INLINE Iterator &operator++() {
                        return static_cast<Iterator &>(this->IncrementImpl());
                    }

                    constexpr ALWAYS_INLINE Iterator &operator--() {
                        return static_cast<Iterator &>(this->DecrementImpl());
                    }
            };

            class ConstIterator : public IteratorBase {
                public:
                    constexpr ALWAYS_INLINE ConstIterator(const FixedTree &tree) : IteratorBase(std::addressof(tree), tree.size() ? tree.GetLMost() : Index_Leaf) { /* ... */ }
                    constexpr ALWAYS_INLINE ConstIterator(const FixedTree &tree, int index) : IteratorBase(std::addressof(tree), index) { /* ... */ }

                    constexpr ALWAYS_INLINE ConstIterator(const ConstIterator &rhs) = default;
                    constexpr ALWAYS_INLINE ConstIterator(const Iterator &rhs) : IteratorBase(rhs.m_tree, rhs.m_index) { /* ... */ }

                    constexpr ALWAYS_INLINE bool operator==(const ConstIterator &rhs) const {
                        return this->IsEqualImpl(rhs);
                    }

                    constexpr ALWAYS_INLINE bool operator!=(const ConstIterator &rhs) const {
                        return !(*this == rhs);
                    }

                    constexpr ALWAYS_INLINE const IteratorMember &operator*() const {
                        return static_cast<const IteratorMember &>(this->DereferenceImpl());
                    }

                    constexpr ALWAYS_INLINE const IteratorMember *operator->() const {
                        return std::addressof(this->operator *());
                    }

                    constexpr ALWAYS_INLINE ConstIterator &operator++() {
                        return static_cast<ConstIterator &>(this->IncrementImpl());
                    }

                    constexpr ALWAYS_INLINE ConstIterator &operator--() {
                        return static_cast<ConstIterator &>(this->DecrementImpl());
                    }
            };
        public:
            using iterator       = Iterator;
            using const_iterator = ConstIterator;
        private:
            impl::AvailableIndexFinder m_index_finder;
            Node m_dummy_leaf;
            Node *m_p_dummy_leaf;
            u8 *m_buffer;
            Header *m_header;
            Node *m_nodes;
            iterator m_end_iterator;
        public:
            FixedTree() : m_end_iterator(*this, Index_Nil) {
                this->SetDummyMemory();
            }
        protected:
            void InitializeImpl(int num_elements, void *buffer, size_t buffer_size) {
                /* Check pre-conditions. */
                AMS_ASSERT(num_elements > 0);
                AMS_ASSERT(static_cast<size_t>(num_elements) <= max_size);
                AMS_ASSERT(util::IsAligned(reinterpret_cast<uintptr_t>(buffer), BufferAlignment));
                AMS_ASSERT(buffer_size == GetRequiredMemorySize(num_elements));

                /* Set buffer. */
                m_buffer = static_cast<u8 *>(buffer);
                m_header = reinterpret_cast<Header *>(m_buffer);

                /* Setup memory. */
                this->InitializeMemory(num_elements, buffer_size, impl::AvailableIndexFinder::GetSignature());

                /* Check that buffer was set up correctly. */
                AMS_ASSERT(static_cast<u32>(buffer_size) == m_header->buffer_size);

                /* Setup dummy leaf. */
                this->SetDummyMemory();
            }
        public:
            static constexpr size_t SizeOfNodes(size_t num_elements) {
                return util::AlignUp(sizeof(Node) * num_elements, BufferAlignment);
            }

            static constexpr size_t SizeOfIndex(size_t num_elements) {
                return impl::AvailableIndexFinder::GetRequiredMemorySize(num_elements);
            }

            static constexpr size_t GetRequiredMemorySize(size_t num_elements) {
                return sizeof(Header) + SizeOfNodes(num_elements) + SizeOfIndex(num_elements);
            }
        private:
            void SetDummyMemory() {
                m_dummy_leaf.m_color  = Color::Black;
                m_dummy_leaf.m_parent = Index_Nil;
                m_dummy_leaf.m_left   = Index_Leaf;
                m_dummy_leaf.m_right  = Index_Leaf;
                m_p_dummy_leaf        = std::addressof(m_dummy_leaf);
            }

            void InitializeMemory(int num_elements, u32 buffer_size, u32 signature) {
                /* Initialize the header. */
                m_header->InitializeHeader(1, num_elements, 0, signature, buffer_size, sizeof(Node), sizeof(Member), 4, 4, 4, 4, 4, BufferAlignment);

                /* Setup index finder. */
                m_index_finder.Initialize(std::addressof(m_header->cur_elements), std::addressof(m_header->max_elements), m_buffer + sizeof(*m_header) + SizeOfNodes(num_elements));

                /* Set nodes array. */
                m_nodes = reinterpret_cast<Node *>(m_buffer + sizeof(*m_header));
            }

            Node *GetNode(int index) const {
                if (index >= 0) {
                    return m_nodes + index;
                } else {
                    return m_p_dummy_leaf;
                }
            }

            constexpr ALWAYS_INLINE bool IsNil(int index) const {
                return index < 0;
            }

            constexpr ALWAYS_INLINE bool IsLeaf(int index) const {
                return index == Index_Leaf;
            }

            int GetRoot() const { return m_header->root_index; }
            void SetRoot(int index) {
                if (index == Index_Leaf) {
                    index = Index_Nil;
                }

                m_header->root_index = index;
            }

            int GetLMost() const { return m_header->left_most_index; }
            void SetLMost(int index) { m_header->left_most_index = index; }

            int GetRMost() const { return m_header->right_most_index; }
            void SetRMost(int index) { m_header->right_most_index = index; }

            int GetParent(int index) const {
                return this->GetNode(index)->m_parent;
            }

            int AcquireIndex() { return m_index_finder.AcquireIndex(); }
            void ReleaseIndex(int index) { return m_index_finder.ReleaseIndex(index); }

            int EraseByIndex(int target_index) {
                /* Setup tracking variables. */
                const auto next_index = this->UncheckedPP(target_index);
                auto *target_node     = this->GetNode(target_index);

                auto a_index   = Index_Leaf;
                auto *a_node   = this->GetNode(a_index);
                auto b_index   = Index_Leaf;
                auto *b_node   = this->GetNode(b_index);
                auto cur_index = target_index;
                auto *cur_node = this->GetNode(cur_index);

                if (cur_node->m_left == Index_Leaf) {
                    a_index = cur_node->m_right;
                    a_node  = this->GetNode(a_index);

                    m_p_dummy_leaf->m_parent = cur_index;
                } else {
                    if (cur_node->m_right == Index_Leaf) {
                        a_index = cur_node->m_left;
                    } else {
                        cur_index = next_index;
                        cur_node  = this->GetNode(cur_index);
                        a_index   = cur_node->m_right;
                    }
                    a_node = this->GetNode(a_index);

                    m_p_dummy_leaf->m_parent = cur_index;
                }

                /* Ensure the a node is updated (redundant) */
                a_node = this->GetNode(a_index);

                /* Update relevant metrics/links. */
                if (cur_index == target_index) {
                    /* No left, but has right. */
                    b_index = target_node->m_parent;
                    b_node  = this->GetNode(b_index);

                    if (a_index != Index_Leaf) {
                        a_node->m_parent = b_index;
                    }

                    if (this->GetRoot() == target_index) {
                        this->SetRoot(a_index);
                    } else if (b_node->m_left == target_index) {
                        b_node->m_left  = a_index;
                    } else {
                        b_node->m_right = a_index;
                    }

                    if (this->GetLMost() == target_index) {
                        this->SetLMost((a_index != Index_Leaf) ? this->FindMinInSubtree(a_index) : b_index);
                    }

                    if (this->GetRMost() == target_index) {
                        this->SetRMost((a_index != Index_Leaf) ? this->FindMaxInSubtree(a_index) : b_index);
                    }
                } else {
                    /* Has left or doesn't have right. */

                    /* Fix left links. */
                    this->GetNode(target_node->m_left)->m_parent = cur_index;
                    cur_node->m_left = target_node->m_left;

                    if (cur_index == target_node->m_right) {
                        b_index = cur_index;
                        b_node  = this->GetNode(b_index);
                    } else {
                        b_index = cur_node->m_parent;
                        b_node  = this->GetNode(b_index);

                        if (!this->IsNil(a_index)) {
                            a_node->m_parent = b_index;
                        }

                        b_node->m_left = a_index;
                        cur_node->m_right = target_node->m_right;

                        this->GetNode(target_node->m_right)->m_parent = cur_index;
                    }

                    if (this->GetRoot() == target_index) {
                        this->SetRoot(cur_index);
                    } else {
                        if (this->GetNode(target_node->m_parent)->m_left == target_index) {
                            this->GetNode(target_node->m_parent)->m_left = cur_index;
                        } else {
                            this->GetNode(target_node->m_parent)->m_right = cur_index;
                        }
                    }

                    cur_node->m_parent = target_node->m_parent;
                    std::swap(cur_node->m_color, target_node->m_color);
                }

                /* Ensure the tree remains balanced. */
                if (target_node->m_color == Color::Black) {
                    while (true) {
                        if (a_index == this->GetRoot() || a_node->m_color != Color::Black) {
                            break;
                        }

                        if (a_index == b_node->m_left) {
                            cur_index = b_node->m_right;
                            cur_node  = this->GetNode(cur_index);

                            if (cur_node->m_color == Color::Red) {
                                cur_node->m_color = Color::Black;
                                b_node->m_color   = Color::Red;
                                AMS_ASSERT(m_p_dummy_leaf->m_color == Color::Black);

                                this->RotateLeft(b_index);

                                cur_index = b_node->m_right;
                                cur_node  = this->GetNode(cur_index);
                            }

                            if (this->IsNil(cur_index)) {
                                a_index = b_index;
                                a_node  = b_node;
                            } else {
                                if (this->GetNode(cur_node->m_left)->m_color != Color::Black || this->GetNode(cur_node->m_right)->m_color != Color::Black) {
                                    if (this->GetNode(cur_node->m_right)->m_color == Color::Black) {
                                        this->GetNode(cur_node->m_left)->m_color = Color::Black;
                                        cur_node->m_color                        = Color::Red;
                                        AMS_ASSERT(m_p_dummy_leaf->m_color == Color::Black);

                                        this->RotateRight(cur_index);

                                        cur_index = b_node->m_right;
                                        cur_node  = this->GetNode(cur_index);
                                    }

                                    cur_node->m_color = b_node->m_color;
                                    b_node->m_color   = Color::Black;

                                    this->GetNode(cur_node->m_right)->m_color = Color::Black;

                                    this->RotateLeft(b_index);

                                    break;
                                }

                                cur_node->m_color = Color::Red;
                                AMS_ASSERT(m_p_dummy_leaf->m_color == Color::Black);

                                a_index = b_index;
                                a_node  = b_node;
                            }
                        } else {
                            cur_index = b_node->m_left;
                            cur_node  = this->GetNode(cur_index);

                            if (cur_node->m_color == Color::Red) {
                                cur_node->m_color = Color::Black;
                                b_node->m_color   = Color::Red;
                                AMS_ASSERT(m_p_dummy_leaf->m_color == Color::Black);

                                this->RotateRight(b_index);

                                cur_index = b_node->m_left;
                                cur_node  = this->GetNode(cur_index);
                            }

                            if (this->IsNil(cur_index)) {
                                a_index = b_index;
                                a_node  = b_node;
                            } else {
                                if (this->GetNode(cur_node->m_right)->m_color != Color::Black || this->GetNode(cur_node->m_left)->m_color != Color::Black) {
                                    if (this->GetNode(cur_node->m_left)->m_color == Color::Black) {
                                        this->GetNode(cur_node->m_right)->m_color = Color::Black;
                                        cur_node->m_color                         = Color::Red;
                                        AMS_ASSERT(m_p_dummy_leaf->m_color == Color::Black);

                                        this->RotateLeft(cur_index);

                                        cur_index = b_node->m_left;
                                        cur_node  = this->GetNode(cur_index);
                                    }

                                    cur_node->m_color = b_node->m_color;
                                    b_node->m_color   = Color::Black;

                                    this->GetNode(cur_node->m_left)->m_color = Color::Black;

                                    this->RotateRight(b_index);

                                    break;
                                }

                                cur_node->m_color = Color::Red;
                                AMS_ASSERT(m_p_dummy_leaf->m_color == Color::Black);

                                a_index = b_index;
                                a_node  = b_node;
                            }
                        }

                        b_index = a_node->m_parent;
                        b_node  = this->GetNode(b_index);
                    }

                    a_node->m_color = Color::Black;
                }

                /* Release the index. */
                this->ReleaseIndex(target_index);
                return target_index;
            }

            int FindIndex(const Member &elem) const {
                return this->FindIndexSub(this->GetRoot(), elem);
            }

            int FindIndexSub(int index, const Member &elem) const {
                if (index != Index_Nil) {
                    auto *node = this->GetNode(index);
                    if (Compare{}(elem, node->m_data)) {
                        if (!this->IsLeaf(node->m_left)) {
                            return this->FindIndexSub(node->m_left, elem);
                        }
                    } else {
                        if (!Compare{}(node->m_data, elem)) {
                            return index;
                        }

                        if (!this->IsLeaf(node->m_right)) {
                            return this->FindIndexSub(node->m_right, elem);
                        }
                    }
                }

                return Index_Nil;
            }

            int FindMaxInSubtree(int index) const {
                int max = index;
                for (auto *node = this->GetNode(index); !this->IsNil(node->m_right); node = this->GetNode(node->m_right)) {
                    max = node->m_right;
                }
                return max;
            }

            int FindMinInSubtree(int index) const {
                int min = index;
                for (auto *node = this->GetNode(index); !this->IsNil(node->m_left); node = this->GetNode(node->m_left)) {
                    min = node->m_left;
                }
                return min;
            }

            int InsertAt(bool before, int parent, const Member &elem) {
                /* Get an index for the new element. */
                const auto index = this->AcquireIndex();

                /* Create the node. */
                auto *node = this->GetNode(index);
                node->m_color  = Color::Red;
                node->m_parent = parent;
                node->m_right  = Index_Leaf;
                node->m_left   = Index_Leaf;
                std::memcpy(reinterpret_cast<u8 *>(std::addressof(node->m_data)), reinterpret_cast<const u8 *>(std::addressof(elem)), sizeof(node->m_data));

                /* Fix up the parent node. */
                auto *parent_node = this->GetNode(parent);
                if (before) {
                    parent_node->m_left = index;
                    if (parent == this->GetLMost()) {
                        this->SetLMost(index);
                    }
                } else {
                    parent_node->m_right = index;
                    if (parent == this->GetRMost()) {
                        this->SetRMost(index);
                    }
                }

                /* Ensure the tree is balanced. */
                int cur_index = index;
                while (true) {
                    auto *cur_node = this->GetNode(cur_index);
                    if (this->GetNode(cur_node->m_parent)->m_color != Color::Red) {
                        break;
                    }

                    auto *p_node = this->GetNode(cur_node->m_parent);
                    auto *g_node = this->GetNode(p_node->m_parent);
                    if (cur_node->m_parent == g_node->m_left) {
                        if (auto *gr_node = this->GetNode(g_node->m_right); gr_node->m_color == Color::Red) {
                            p_node->m_color  = Color::Black;
                            gr_node->m_color = Color::Black;
                            g_node->m_color  = Color::Red;
                            AMS_ASSERT(m_p_dummy_leaf->m_color != Color::Red);

                            cur_index = p_node->m_parent;
                            continue;
                        }

                        if (cur_index == p_node->m_right) {
                            cur_index = cur_node->m_parent;
                            cur_node  = this->GetNode(cur_index);
                            this->RotateLeft(cur_index);
                        }

                        p_node = this->GetNode(cur_node->m_parent);
                        p_node->m_color = Color::Black;

                        g_node = this->GetNode(p_node->m_parent);
                        g_node->m_color = Color::Red;

                        AMS_ASSERT(m_p_dummy_leaf->m_color != Color::Red);

                        this->RotateRight(p_node->m_parent);
                    } else {
                        if (auto *gl_node = this->GetNode(g_node->m_left); gl_node->m_color == Color::Red) {
                            p_node->m_color  = Color::Black;
                            gl_node->m_color = Color::Black;
                            g_node->m_color  = Color::Red;
                            AMS_ASSERT(m_p_dummy_leaf->m_color != Color::Red);

                            cur_index = p_node->m_parent;
                            continue;
                        }

                        if (cur_index == p_node->m_left) {
                            cur_index = cur_node->m_parent;
                            cur_node  = this->GetNode(cur_index);
                            this->RotateRight(cur_index);
                        }

                        p_node = this->GetNode(cur_node->m_parent);
                        p_node->m_color = Color::Black;

                        g_node = this->GetNode(p_node->m_parent);
                        g_node->m_color = Color::Red;

                        AMS_ASSERT(m_p_dummy_leaf->m_color != Color::Red);

                        this->RotateLeft(p_node->m_parent);
                    }
                }

                /* Set root color. */
                this->GetNode(this->GetRoot())->m_color = Color::Black;

                return index;
            }

            int InsertNoHint(bool before, const Member &elem) {
                int cur_index  = this->GetRoot();
                int prev_index = Index_Nil;
                bool less = true;
                while (cur_index != Index_Nil && cur_index != Index_Leaf) {
                    auto *node = this->GetNode(cur_index);
                    prev_index = cur_index;

                    if (before) {
                        less = Compare{}(node->m_data, elem);
                    } else {
                        less = Compare{}(elem, node->m_data);
                    }

                    if (less) {
                        cur_index = node->m_left;
                    } else {
                        cur_index = node->m_right;
                    }
                }

                if (cur_index == Index_Nil) {
                    /* Create a new node. */
                    const auto index = this->AcquireIndex();
                    auto *node       = this->GetNode(index);
                    node->m_color  = Color::Black;
                    node->m_parent = Index_Nil;
                    node->m_right  = Index_Leaf;
                    node->m_left   = Index_Leaf;
                    std::memcpy(reinterpret_cast<u8 *>(std::addressof(node->m_data)), reinterpret_cast<const u8 *>(std::addressof(elem)), sizeof(node->m_data));

                    this->SetRoot(index);
                    this->SetLMost(index);
                    this->SetRMost(index);

                    return index;
                } else {
                    auto *compare_node = this->GetNode(prev_index);
                    if (less) {
                        if (prev_index == this->GetLMost()) {
                            return this->InsertAt(less, prev_index, elem);
                        } else {
                            compare_node = this->GetNode(this->UncheckedMM(prev_index));
                        }
                    }

                    if (Compare{}(compare_node->m_data, elem)) {
                        return this->InsertAt(less, prev_index, elem);
                    } else {
                        return Index_Nil;
                    }
                }

            }

            void RotateLeft(int index) {
                /* Determine indices. */
                const auto p_index  = this->GetParent(index);
                const auto r_index  = this->GetNode(index)->m_right;
                const auto l_index  = this->GetNode(index)->m_left;
                const auto rl_index = this->GetNode(r_index)->m_left;
                const auto rr_index = this->GetNode(r_index)->m_right;

                /* Get nodes. */
                auto *node    = this->GetNode(index);
                auto *p_node  = this->GetNode(p_index);
                auto *r_node  = this->GetNode(r_index);
                auto *l_node  = this->GetNode(l_index);
                auto *rl_node = this->GetNode(rl_index);
                auto *rr_node = this->GetNode(rr_index);

                /* Perform the rotation. */
                if (p_index == Index_Nil) {
                    r_node->m_parent     = Index_Nil;
                    m_header->root_index = r_index;
                } else if (p_node->m_left == index) {
                    p_node->SetLeft(r_index, r_node, p_index);
                } else {
                    p_node->SetRight(r_index, r_node, p_index);
                }
                r_node->SetLeft(index, node, r_index);
                r_node->SetRight(rr_index, rr_node, r_index);
                node->SetLeft(l_index, l_node, index);
                node->SetRight(rl_index, rl_node, index);
            }

            void RotateRight(int index) {
                /* Determine indices. */
                const auto p_index  = this->GetParent(index);
                const auto l_index  = this->GetNode(index)->m_left;
                const auto ll_index = this->GetNode(l_index)->m_left;
                const auto lr_index = this->GetNode(l_index)->m_right;
                const auto r_index  = this->GetNode(index)->m_right;

                /* Get nodes. */
                auto *node    = this->GetNode(index);
                auto *p_node  = this->GetNode(p_index);
                auto *l_node  = this->GetNode(l_index);
                auto *ll_node = this->GetNode(ll_index);
                auto *lr_node = this->GetNode(lr_index);
                auto *r_node  = this->GetNode(r_index);

                /* Perform the rotation. */
                if (p_index == Index_Nil) {
                    l_node->m_parent     = Index_Nil;
                    m_header->root_index = l_index;
                } else if (p_node->m_left == index) {
                    p_node->SetLeft(l_index, l_node, p_index);
                } else {
                    p_node->SetRight(l_index, l_node, p_index);
                }
                l_node->SetLeft(ll_index, ll_node, l_index);
                l_node->SetRight(index, node, l_index);
                node->SetLeft(lr_index, lr_node, index);
                node->SetRight(r_index, r_node, index);
            }

            int UncheckedMM(int index) const {
                auto *node = this->GetNode(index);
                if (this->IsNil(index)) {
                    index = this->GetRMost();
                    node  = this->GetNode(index);
                } else if (this->IsNil(node->m_left)) {
                    int parent = node->m_parent;
                    Node *p;

                    for (p = this->GetNode(parent); !this->IsNil(parent) && index == p->m_left; p = this->GetNode(parent)) {
                        index  = parent;
                        node   = p;
                        parent = p->m_parent;
                    }

                    if (!this->IsNil(index)) {
                        index = parent;
                        node  = p;
                    }
                } else {
                    index = this->FindMaxInSubtree(node->m_left);
                    node  = this->GetNode(index);
                }

                if (this->IsNil(index)) {
                    return Index_Leaf;
                } else {
                    return index;
                }
            }

            int UncheckedPP(int index) const {
                auto *node = this->GetNode(index);

                if (!this->IsNil(index)) {
                    if (this->IsNil(node->m_right)) {
                        int parent = node->m_parent;
                        Node *p;

                        for (p = this->GetNode(parent); !this->IsNil(parent) && index == p->m_right; p = this->GetNode(parent)) {
                            index  = parent;
                            node   = p;
                            parent = p->m_parent;
                        }

                        index = parent;
                        node  = p;
                    } else {
                        index = this->FindMinInSubtree(node->m_right);
                        node  = this->GetNode(index);
                    }
                }

                if (this->IsNil(index)) {
                    return Index_Leaf;
                } else {
                    return index;
                }
            }
        public:
            void Initialize(size_t num_elements, void *buffer, size_t buffer_size) {
                AMS_ASSERT(num_elements <= max_size);

                return this->InitializeImpl(static_cast<int>(num_elements), buffer, buffer_size);
            }

            iterator begin() { return iterator(*this); }
            const_iterator begin() const { return const_iterator(*this); }

            iterator end() { return m_end_iterator; }
            const_iterator end() const { return m_end_iterator; }

            size_t size() const { return m_header->cur_elements; }

            void clear() {
                const auto num_elements = m_header->max_elements;
                const auto buffer_size  = m_header->buffer_size;
                AMS_ASSERT(buffer_size == static_cast<u32>(GetRequiredMemorySize(num_elements)));

                return this->InitializeMemory(num_elements, buffer_size, impl::AvailableIndexFinder::GetSignature());
            }

            bool erase(const Member &elem) {
                const auto range = this->equal_range(elem);
                if (range.first != range.last) {
                    this->EraseByIndex(range.first);
                    return true;
                } else {
                    return false;
                }
            }

            iterator find(const Member &elem) {
                if (const auto index = this->FindIndex(elem); index >= 0) {
                    return iterator(*this, index);
                } else {
                    return this->end();
                }
            }

            const_iterator find(const Member &elem) const {
                if (const auto index = this->FindIndex(elem); index >= 0) {
                    return const_iterator(*this, index);
                } else {
                    return this->end();
                }
            }

            std::pair<iterator, bool> insert(const Member &elem) {
                const auto index = this->InsertNoHint(false, elem);
                const auto it    = iterator(*this, index);
                return std::make_pair(it, !this->IsNil(index));
            }

            IndexPair equal_range(const Member &elem) {
                /* Get node to start iteration. */
                auto cur_index = this->GetRoot();
                auto cur_node  = this->GetNode(cur_index);

                auto min_index = Index_Leaf;
                auto min_node  = this->GetNode(min_index);

                auto max_index = Index_Leaf;
                auto max_node  = this->GetNode(max_index);

                /* Iterate until current is leaf, to find min/max. */
                while (cur_index != Index_Leaf) {
                    if (Compare{}(cur_node->m_data, elem)) {
                        cur_index = cur_node->m_right;
                        cur_node  = this->GetNode(cur_index);
                    } else {
                        if (max_index == Index_Leaf && Compare{}(elem, cur_node->m_data)) {
                            max_index = cur_index;
                            max_node  = this->GetNode(max_index);
                        }
                        min_index = cur_index;
                        min_node  = this->GetNode(min_index);

                        cur_index = cur_node->m_left;
                        cur_node  = this->GetNode(cur_index);
                    }
                }

                /* Iterate again, to find correct range extent for max. */
                cur_index = (max_index == Index_Leaf) ? this->GetRoot() : max_node->m_left;
                cur_node  = this->GetNode(cur_index);
                while (cur_index != Index_Leaf) {
                    if (Compare{}(elem, cur_node->m_data)) {
                        max_index = cur_index;
                        max_node  = cur_node;
                        cur_index = cur_node->m_left;
                    } else {
                        cur_index = cur_node->m_right;
                    }
                    cur_node = this->GetNode(cur_index);
                }

                AMS_UNUSED(min_node);
                return IndexPair{min_index, max_index};
            }
    };



}
