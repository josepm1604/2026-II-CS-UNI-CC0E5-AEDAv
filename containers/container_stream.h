#ifndef CONTAINERS_CONTAINER_STREAM_H
#define CONTAINERS_CONTAINER_STREAM_H

#include "GeneralNode.h"
#include <concepts>
#include <istream>
#include <ostream>
#include <vector>

namespace container_stream {

using Delim = StreamDelimiters::Delim;

inline bool expect(std::istream& is, Delim wanted) {
    Delim actual;
    if (!(is >> actual)) return false;
    if (actual == wanted) return true;
    is.setstate(std::ios::failbit);
    return false;
}

template <class Container>
concept PrintableContainer = requires(const Container& c, std::ostream& os) {
    typename Container::Node;
    c.begin();
    c.end();
    requires std::same_as<decltype(c.begin()), decltype(c.end())>;
    c.begin() != c.end();
    ++c.begin();
    os << *c.begin();
};

template <class Container>
concept ReadableContainer = PrintableContainer<Container> &&
    requires(Container& c, typename Container::value_type value, Ref ref,
             std::istream& is, GeneralNode<typename Container::value_type>& node) {
    typename Container::value_type;
    requires std::default_initializable<GeneralNode<typename Container::value_type>>;
    is >> node;
    c.clear();
    c.push_back(value, ref);
};

template <class It>
std::ostream& write(std::ostream& os, It first, It last) {
    os << StreamDelimiters::container_open;
    if (first != last) {
        os << *first;
        while (++first != last)
            os << StreamDelimiters::separator << *first;
    }
    return os << StreamDelimiters::container_close;
}

template <typename T>
bool read_nodes(std::istream& is, std::vector<GeneralNode<T>>& nodes) {
    if (!expect(is, StreamDelimiters::container_open)) return false;
    is >> std::ws;
    if (is.peek() == StreamDelimiters::container_close) {
        is.get();
        return true;
    }
    while (true) {
        GeneralNode<T> node;
        if (!(is >> node)) return false;
        nodes.push_back(node);
        Delim separator;
        if (!(is >> separator)) return false;
        if (separator == StreamDelimiters::container_close) return true;
        if (separator != StreamDelimiters::separator) {
            is.setstate(std::ios::failbit);
            return false;
        }
    }
}

template <ReadableContainer Container>
void insert_nodes(Container& destination,
                  const std::vector<GeneralNode<typename Container::value_type>>& nodes) {
    destination.clear();
    for (const auto& node : nodes)
        destination.push_back(node.getValue(), node.getRef());
}

} // namespace container_stream

template <container_stream::PrintableContainer Container>
std::ostream& operator<<(std::ostream& os, const Container& container) {
    return container_stream::write(os, container.begin(), container.end());
}

template <container_stream::ReadableContainer Container>
std::istream& operator>>(std::istream& is, Container& container) {
    std::vector<GeneralNode<typename Container::value_type>> nodes;
    if (container_stream::read_nodes(is, nodes))
        container_stream::insert_nodes(container, nodes);
    return is;
}

#endif
