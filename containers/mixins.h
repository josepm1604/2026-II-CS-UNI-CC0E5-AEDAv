#ifndef __MIXINS__
#define __MIXINS__
#include <vector>
#include "../foreach.h"
#include "ContainerCore.h"
#include "GeneralNode.h"

// template <typename Traits>
// using Parent = ContainerCore<Traits>;

template <typename Traits>
class Callable : virtual public ContainerCore<Traits>  {
public:
    template<typename Func, typename... Args>
    decltype(auto) call(Func func, Args&&... args) {
        std::lock_guard lock(this->m_mutex);
        return ::call(this->begin(), this->end(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

template <typename Traits>
class Applicable : virtual public Callable<Traits> {
public:
    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args&&... args) { this->call(func, std::forward<Args>(args)...); }
};

template <typename Traits>
class Searchable : virtual public Callable<Traits>  {
public:
    using Parent    = Callable<Traits>;
    using Node      = typename Parent::Node;

    template <typename Func, typename... Args>
    Node& FirstThat(Func func, Args&&... args) { return this->call(func, std::forward<Args>(args)...); }
};

template <typename Traits>
class ReverseCallable : virtual public ContainerCore<Traits>  {
public:
    using BackwardIterator = typename Traits::BackwardIterator;
public:
    // Igual que call(), pero recorriendo el container en sentido inverso
    // (rbegin()/rend()) en vez de hacia adelante.
    template<typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args) {
        std::lock_guard lock(this->m_mutex);
        return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
    }

    virtual BackwardIterator rbegin() const = 0;
    virtual BackwardIterator rend() const = 0;
};

template <typename Traits>
class Printable : virtual public ContainerCore<Traits> {
    friend std::ostream& operator<<(std::ostream& os, const Printable& container) {
        std::lock_guard lock(container.m_mutex);
        bool first = true;
        os << StreamDelimiters::container_open;
        for (const auto& e : container) {
            if (!first) os << StreamDelimiters::separator;
            os << e;
            first = false;
        }
        return os << StreamDelimiters::container_close;
    }
};

template <typename Traits>
class Readable : virtual public ContainerCore<Traits> {
    using Delim = StreamDelimiters::Delim;
    using Entry = GeneralNode<typename Traits::value_type>;

    static bool parse(std::istream& is, std::vector<Entry>& entries) {
        Delim d{};
        if (!(is >> d) || d != StreamDelimiters::container_open) {
            is.setstate(std::ios::failbit);
            return false;
        }
        if (is >> std::ws && is.peek() == StreamDelimiters::container_close) {
            is.get();
            return true;
        }
        Entry entry;
        while (is >> entry >> d) {
            entries.push_back(entry);
            if (d == StreamDelimiters::container_close) return true;
            if (d != StreamDelimiters::separator) break;
        }
        is.setstate(std::ios::failbit);
        return false;
    }

    friend std::istream& operator>>(std::istream& is, Readable& container) {
        std::vector<Entry> entries;
        if (!parse(is, entries)) return is;

        std::lock_guard lock(container.m_mutex);
        container.clear();
        for (const auto& entry : entries)
            container.push_back(entry.getValue(), entry.getRef());
        return is;
    }
};

#endif