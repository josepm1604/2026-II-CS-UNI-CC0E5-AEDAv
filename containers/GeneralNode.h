#ifndef __GENERAL_NODE_H__
#define __GENERAL_NODE_H__
#include <iostream>
#include <string>
#include <type_traits>
#include "../types.h" // Ref
template <typename T>
struct GeneralNode{
private:
    T   m_value;
    Ref m_ref;      // Reference to the value

public:
    GeneralNode() = default; // requerido por resize(): new Node[new_cap]
    GeneralNode(const T& value, Ref ref) : m_value(value), m_ref(ref) {}
    T    getValue() const { return m_value; }
    Ref  getRef()   const { return m_ref;   }
    T&   value()          { return m_value; } // acceso mutable para ApplyFunction

    friend std::ostream &operator <<(std::ostream &os, const GeneralNode<T> &node) {
        os << "(" << node.getValue() << "," << node.getRef() << ")";
        return os;
    }

    friend std::istream &operator >>(std::istream &is, GeneralNode<T> &node) {
        char separator;
        if (!(is >> separator) || separator != '(') { is.setstate(std::ios::failbit); return is; }
        if constexpr (std::is_same_v<T, std::string>) {
            if (!std::getline(is, node.m_value, ',')) return is;
        } else {
            if (!(is >> node.m_value >> separator) || separator != ',') { is.setstate(std::ios::failbit); return is; }
        }
        if (!(is >> node.m_ref >> separator) || separator != ')') { is.setstate(std::ios::failbit); return is; }
        return is;
    }
};

#endif // __GENERAL_NODE_H__
