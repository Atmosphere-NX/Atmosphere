/*
** Sample Framework for deko3d Applications
**   CIntrusiveTree.cpp: Intrusive red-black tree helper class
*/
#include "CIntrusiveTree.h"

// This red-black tree implementation is mostly based on mtheall's work,
// which can be found here:
//   https://github.com/smealum/ctrulib/tree/master/libctru/source/util/rbtree

void CIntrusiveTreeBase::rotate(N* node, N::Leaf leaf)
{
    N *tmp    = node->child(leaf);
    N *parent = node->getParent();

    node->child(leaf) = tmp->child(!leaf);
    if (tmp->child(!leaf))
        tmp->child(!leaf)->setParent(node);

    tmp->child(!leaf) = node;
    tmp->setParent(parent);

    if (parent)
    {
        if (node == parent->child(!leaf))
            parent->child(!leaf) = tmp;
        else
            parent->child(leaf) = tmp;
    }
    else
        m_root = tmp;

    node->setParent(tmp);
}

void CIntrusiveTreeBase::recolor(N* parent, N* node)
{
    N *sibling;

    while ((!node || node->isBlack()) && node != m_root)
    {
        N::Leaf leaf = node == parent->left() ? N::Right : N::Left;
        sibling = parent->child(leaf);

        if (sibling->isRed())
        {
            sibling->setBlack();
            parent->setRed();
            rotate(parent, leaf);
            sibling = parent->child(leaf);
        }

        N::Color clr[2];
        clr[N::Left]  = sibling->left()  ? sibling->left()->getColor()  : N::Black;
        clr[N::Right] = sibling->right() ? sibling->right()->getColor() : N::Black;

        if (clr[N::Left] == N::Black && clr[N::Right] == N::Black)
        {
            sibling->setRed();
            node = parent;
            parent = node->getParent();
        }
        else
        {
            if (clr[leaf] == N::Black)
            {
                sibling->child(!leaf)->setBlack();
                sibling->setRed();
                rotate(sibling, !leaf);
                sibling = parent->child(leaf);
            }

            sibling->setColor(parent->getColor());
            parent->setBlack();
            sibling->child(leaf)->setBlack();
            rotate(parent, leaf);

            node = m_root;
        }
    }

    if (node)
        node->setBlack();
}

auto CIntrusiveTreeBase::walk(N* node, N::Leaf leaf) const -> N*
{
    if (node->child(leaf))
    {
        node = node->child(leaf);
        while (node->child(!leaf))
            node = node->child(!leaf);
    }
    else
    {
        N *parent = node->getParent();
        while (parent && node == parent->child(leaf))
        {
            node = parent;
            parent = node->getParent();
        }
        node = parent;
    }

    return node;
}

void CIntrusiveTreeBase::insert(N* node, N* parent)
{
    node->left() = node->right() = nullptr;
    node->setParent(parent);
    node->setRed();

    while ((parent = node->getParent()) && parent->isRed())
    {
        N *grandparent = parent->getParent();
        N::Leaf leaf = parent == grandparent->left() ? N::Right : N::Left;
        N *uncle = grandparent->child(leaf);

        if (uncle && uncle->isRed())
        {
            uncle->setBlack();
            parent->setBlack();
            grandparent->setRed();

            node = grandparent;
        }
        else
        {
            if (parent->child(leaf) == node)
            {
                rotate(parent, leaf);

                N* tmp = parent;
                parent = node;
                node = tmp;
            }

            parent->setBlack();
            grandparent->setRed();
            rotate(grandparent, !leaf);
        }
    }

    m_root->setBlack();
}

void CIntrusiveTreeBase::remove(N* node)
{
    N::Color color;
    N *child, *parent;

    if (node->left() && node->right())
    {
        N *old = node;

        node = node->right();
        while (node->left())
            node = node->left();

        parent = old->getParent();
        if (parent)
        {
            if (parent->left() == old)
                parent->left() = node;
            else
                parent->right() = node;
        }
        else
            m_root = node;

        child  = node->right();
        parent = node->getParent();
        color  = node->getColor();

        if (parent == old)
            parent = node;
        else
        {
            if (child)
                child->setParent(parent);
            parent->left() = child;

            node->right() = old->right();
            old->right()->setParent(node);
        }

        node->setParent(old->getParent());
        node->setColor(old->getColor());
        node->left() = old->left();
        old->left()->setParent(node);
    }
    else
    {
        child  = node->left() ? node->right() : node->left();
        parent = node->getParent();
        color  = node->getColor();

        if (child)
            child->setParent(parent);
        if (parent)
        {
            if (parent->left() == node)
                parent->left() = child;
            else
                parent->right() = child;
        }
        else
            m_root = child;
    }

    if (color == N::Black)
        recolor(parent, child);
}
