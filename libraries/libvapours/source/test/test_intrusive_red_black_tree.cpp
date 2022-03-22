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
#include <vapours.hpp>

/* TODO: Define to enable tests? */
#if 0

namespace ams::test {

    template<typename T>
    concept IsRedBlackTreeTestNode = std::constructible_from<T, int> && requires (T &t, const T &ct) {
        { ct.GetValue() } -> std::same_as<int>;
        {  t.GetNode()  } -> std::same_as<      util::IntrusiveRedBlackTreeNode &>;
        { ct.GetNode()  } -> std::same_as<const util::IntrusiveRedBlackTreeNode &>;
    };

    template<typename T> requires IsRedBlackTreeTestNode<T>
    struct TestComparator {
        using RedBlackKeyType = int;

        static constexpr int Compare(const T &lhs, const T &rhs) {
            if (lhs.GetValue() < rhs.GetValue()) {
                return -1;
            } else if (lhs.GetValue() > rhs.GetValue()) {
                return 1;
            } else {
                return 0;
            }
        }

        static constexpr int Compare(const int &lhs, const T &rhs) {
            if (lhs < rhs.GetValue()) {
                return -1;
            } else if (lhs > rhs.GetValue()) {
                return 1;
            } else {
                return 0;
            }
        }
    };

    class TestBaseNode : public util::IntrusiveRedBlackTreeBaseNode<TestBaseNode> {
        private:
            const int m_value;
        public:
            constexpr TestBaseNode(int value) : m_value(value) { /* ... */ }

            constexpr int GetValue() const { return m_value; }

            constexpr util::IntrusiveRedBlackTreeNode &GetNode()             { return static_cast<      util::IntrusiveRedBlackTreeNode &>(*this); }
            constexpr const util::IntrusiveRedBlackTreeNode &GetNode() const { return static_cast<const util::IntrusiveRedBlackTreeNode &>(*this); }
    };
    static_assert(IsRedBlackTreeTestNode<TestBaseNode>);

    class TestTreeTypes;

    class TestMemberNode {
        private:
            friend class TestTreeTypes;
        private:
            const int m_value;
            util::IntrusiveRedBlackTreeNode m_node;
        public:
            constexpr TestMemberNode(int value) : m_value(value), m_node() { /* ... */ }

            constexpr int GetValue() const { return m_value; }

            constexpr util::IntrusiveRedBlackTreeNode &GetNode()             { return m_node; }
            constexpr const util::IntrusiveRedBlackTreeNode &GetNode() const { return m_node; }
    };
    static_assert(IsRedBlackTreeTestNode<TestMemberNode>);

    class TestTreeTypes {
        public:
            using BaseTree   = util::IntrusiveRedBlackTreeBaseTraits<TestBaseNode>::TreeType<TestComparator<TestBaseNode>>;
            using MemberTree = util::IntrusiveRedBlackTreeMemberTraits<&TestMemberNode::m_node>::TreeType<TestComparator<TestMemberNode>>;
    };

    using TestBaseTree   = TestTreeTypes::BaseTree;
    using TestMemberTree = TestTreeTypes::MemberTree;

    template<typename Tree, typename Node>
    consteval bool TestUsage() {
        constexpr int Values[] = { -3, 0, 5, 7, 11111111, 924, -100, 68, 70, 69, };

        /* Get sorted array. */
        std::array<int, util::size(Values)> sorted_values{};
        std::copy(std::begin(Values), std::end(Values), std::begin(sorted_values));
        std::sort(std::begin(sorted_values), std::end(sorted_values));

        /* Create the tree. */
        Tree tree{};
        AMS_ASSUME(tree.begin() == tree.end());

        /* Create a node for each value. */
        /* TODO: GCC bug in constant evaluation fails if we use constexpr new/dynamically allocated nodes. */
        /*       Check if this works in gcc 11. */
        std::array<Node, util::size(Values)> nodes = [&]<size_t... Ix>(std::index_sequence<Ix...>) {
            return std::array<Node, util::size(Values)> { Node(Values[Ix])... };
        }(std::make_index_sequence<util::size(Values)>());

        /* Insert each node into the tree. */
        for (size_t i = 0; i < util::size(Values); ++i) {
            tree.insert(nodes[i]);
            if (std::distance(tree.begin(), tree.end()) != static_cast<int>(i + 1)) {
                return false;
            }
        }

        /* Verify that the nodes are in sorted order. */
        {
            size_t i = 0;
            for (const auto &node : tree) {
                if (node.GetValue() != sorted_values[i++]) {
                    return false;
                }
            }
        }

        /* Verify correctness with begin() */
        {
            size_t i = 0;
            for (auto it = tree.begin(); it != tree.end(); ++it) {
                if (it->GetValue() != sorted_values[i++]) {
                    return false;
                }
            }
        }

        /* Verify correctness with cbegin() */
        {
            size_t i = 0;
            for (auto it = tree.cbegin(); it != tree.cend(); ++it) {
                if (it->GetValue() != sorted_values[i++]) {
                    return false;
                }
            }
        }

        /* Verify min/max. */
        if (tree.front().GetValue() != sorted_values[0]) {
            return false;
        }
        if (tree.back().GetValue()  != sorted_values[sorted_values.size() - 1]) {
            return false;
        }

        /* Remove a value. */
        tree.erase(tree.iterator_to(nodes[3]));

        /* Verify nodes are in sorted order. */
        {
            size_t i = 0;
            for (const auto &node : tree) {
                if (node.GetValue() == nodes[3].GetValue()) {
                    return false;
                }

                if (node.GetValue() != sorted_values[i++]) {
                    if (node.GetValue() != sorted_values[i++]) {
                        return false;
                    }
                }
            }
        }

        /* Add the node back. */
        tree.insert(nodes[3]);

        /* Verify nodes are in sorted order. */
        {
            size_t i = 0;
            for (const auto &node : tree) {
                if (node.GetValue() != sorted_values[i++]) {
                    return false;
                }
            }
        }

        /* Verify that find works. */
        for (size_t i = 0; i < util::size(Values); ++i) {
            if (tree.find(Node(Values[i])) != tree.iterator_to(nodes[i])) {
                return false;
            }
            if (tree.nfind(Node(sorted_values[i]))->GetValue() != sorted_values[i]) {
                return false;
            }
            if (tree.find_key(Values[i]) != tree.iterator_to(nodes[i])) {
                return false;
            }
            if (tree.nfind_key(sorted_values[i])->GetValue() != sorted_values[i]) {
                return false;
            }
        }

        if (tree.find(Node(std::numeric_limits<int>::min())) != tree.end()) {
            return false;
        }

        /* Verify that nfind works. */
        for (size_t i = 0; i < util::size(Values) - 1; ++i) {
            if (tree.nfind(Node(sorted_values[i] + 1))->GetValue() != sorted_values[i + 1]) {
                return false;
            }
            if (tree.nfind_key(sorted_values[i] + 1)->GetValue() != sorted_values[i + 1]) {
                return false;
            }
        }

        return true;
    }

    static_assert(TestUsage<TestBaseTree, TestBaseNode>());
    static_assert(TestUsage<TestMemberTree, TestMemberNode>());


}

#endif
