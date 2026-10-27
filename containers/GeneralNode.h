#ifndef __GENERAL_NODE_H__
#define __GENERAL_NODE_H__
#include <iostream>
#include "../types.h" // Ref

struct StreamDelimiters {
    using Delim = std::istream::char_type;
    static constexpr Delim container_open  = '[';
    static constexpr Delim container_close = ']';
    static constexpr Delim separator       = ',';
    static constexpr Delim node_open       = '(';
    static constexpr Delim node_close      = ')';
};

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

    using Delim = StreamDelimiters::Delim;

    friend std::ostream &operator <<(std::ostream &os, const GeneralNode<T> &node) {
        os << StreamDelimiters::node_open
           << node.getValue()
           << StreamDelimiters::separator
           << node.getRef()
           << StreamDelimiters::node_close;
        return os;
    }

    friend std::istream &operator >>(std::istream &is, GeneralNode<T> &node){
        Delim open{}, sep{}, close{};
        if (!(is >> open) || open != StreamDelimiters::node_open ||
            !(is >> node.m_value) ||
            !(is >> sep) || sep != StreamDelimiters::separator ||
            !(is >> node.m_ref) ||
            !(is >> close) || close != StreamDelimiters::node_close) {
            is.setstate(std::ios::failbit);
        }
        return is;
    }
};

#endif // __GENERAL_NODE_H__