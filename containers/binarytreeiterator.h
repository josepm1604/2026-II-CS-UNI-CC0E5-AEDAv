#ifndef CONTAINERS_BINARYTREEITERATOR_H
#define CONTAINERS_BINARYTREEITERATOR_H

#include "GeneralIterator.h"

enum class TreeOrder { PreOrder, InOrder, PostOrder };

template <typename Node>
Node* extreme(Node* cur, bool side) {
    while (cur && cur->m_pChild[side])
        cur = cur->m_pChild[side];
    return cur;
}

template <typename Node, TreeOrder Order = TreeOrder::InOrder, bool Reverse = false>
class BinaryTreeIterator : public GeneralIterator<BinaryTreeIterator<Node, Order, Reverse>, Node> {
    static constexpr bool L = Reverse;
    static constexpr bool R = !Reverse;

    static Node* deepest(Node* cur) {
        while (cur && (cur->m_pChild[L] || cur->m_pChild[R]))
            cur = cur->m_pChild[L] ? cur->m_pChild[L] : cur->m_pChild[R];
        return cur;
    }

    static Node* first(Node* root) {
        if constexpr (Order == TreeOrder::PreOrder)
            return root;
        else if constexpr (Order == TreeOrder::InOrder)
            return extreme(root, L);
        else
            return deepest(root);
    }

public:
    using value_type        = Node;
    using MySelf            = BinaryTreeIterator<Node, Order, Reverse>;
    using Parent            = GeneralIterator<MySelf, Node>;

    BinaryTreeIterator(Node* root = nullptr) : Parent(first(root)) {}

    // Prefix increment
    BinaryTreeIterator& operator++() {
        Node* cur = Parent::m_ptr;
        Node* p = cur->m_pParent;
        if constexpr (Order == TreeOrder::InOrder) {
            if (cur->m_pChild[R]) {
                Parent::m_ptr = extreme(cur->m_pChild[R], L);
                return *this;
            }
            while (p && cur == p->m_pChild[R]) {
                cur = p;
                p = p->m_pParent;
            }
            Parent::m_ptr = p;
        } else if constexpr (Order == TreeOrder::PreOrder) {
            if (cur->m_pChild[L] || cur->m_pChild[R]) {
                Parent::m_ptr = cur->m_pChild[L] ? cur->m_pChild[L] : cur->m_pChild[R];
                return *this;
            }
            while (p && (cur == p->m_pChild[R] || !p->m_pChild[R])) {
                cur = p;
                p = p->m_pParent;
            }
            Parent::m_ptr = p ? p->m_pChild[R] : nullptr;
        } else {
            if (p && cur == p->m_pChild[L] && p->m_pChild[R])
                Parent::m_ptr = deepest(p->m_pChild[R]);
            else
                Parent::m_ptr = p;
        }
        return *this;
    }
};

template <typename Iterator>
class TreeTraversal {
    typename Iterator::pointer m_root;

public:
    explicit TreeTraversal(typename Iterator::pointer root) : m_root(root) {}
    Iterator begin() const { return Iterator(m_root); }
    Iterator end() const { return Iterator(nullptr); }
};

#endif // CONTAINERS_BINARYTREEITERATOR_H
