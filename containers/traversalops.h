#ifndef TRAVERSALOPS_H
#define TRAVERSALOPS_H

#include "container_stream.h"
#include "traversal_algorithms.h"
#include <istream>
#include <mutex>
#include <utility>

template <class Container>
concept ReverseTraversable = requires(Container& container) {
    container.rbegin();
    container.rend();
};

class LockableOp {
    friend struct LockedTraversal;

protected:
    mutable std::mutex m_mutex;
};

struct LockedTraversal {
    template <bool Reverse, class Self, class Algorithm, class... Args>
    static decltype(auto) run(Self& c, Algorithm algorithm, Args&&... args) {
        std::lock_guard lock(c.m_mutex);
        if constexpr (Reverse)
            return algorithm(c.rbegin(), c.rend(), std::forward<Args>(args)...);
        else
            return algorithm(c.begin(), c.end(), std::forward<Args>(args)...);
    }
};

class FirstThatOp {
public:
    template <class Self, class Pred, class... Args>
    decltype(auto) FirstThat(this Self& c, Pred&& pred, Args&&... args) {
        return LockedTraversal::run<false>(c, traversal_detail::first_that_fn,
                                           std::forward<Pred>(pred), std::forward<Args>(args)...);
    }
};

class RFirstThatOp {
public:
    template <class Self, class Pred, class... Args>
        requires ReverseTraversable<Self>
    decltype(auto) RFirstThat(this Self& c, Pred&& pred, Args&&... args) {
        return LockedTraversal::run<true>(c, traversal_detail::first_that_fn,
                                          std::forward<Pred>(pred), std::forward<Args>(args)...);
    }
};

class CallOp {
public:
    template <class Self, class Func, class... Args>
    decltype(auto) call(this Self& c, Func&& func, Args&&... args) {
        return LockedTraversal::run<false>(c, traversal_detail::legacy_call_fn,
                                           std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

class RCallOp {
public:
    template <class Self, class Func, class... Args>
        requires ReverseTraversable<Self>
    decltype(auto) rcall(this Self& c, Func&& func, Args&&... args) {
        return LockedTraversal::run<true>(c, traversal_detail::legacy_call_fn,
                                          std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

class ApplyFunctionOp {
public:
    template <class Self, class Func, class... Args>
    void ApplyFunction(this Self& c, Func&& func, Args&&... args) {
        LockedTraversal::run<false>(c, traversal_detail::for_each_fn,
                                    std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

class PrintableOp {
    template <container_stream::PrintableContainer Container>
    friend std::ostream& operator<<(std::ostream& os, const Container& container) {
        return container_stream::write(os, container);
    }
};

class ReadableOp {
    template <container_stream::ReadableContainer Container>
    friend std::istream& operator>>(std::istream& is, Container& container) {
        return container_stream::read(is, container);
    }
};

template <class... Ops>
class TraversalOps : public LockableOp, public Ops... {};

using FullTraversalOps = TraversalOps<CallOp, RCallOp, FirstThatOp,
                                      RFirstThatOp, ApplyFunctionOp,
                                      PrintableOp, ReadableOp>;

using SearchTraversalOps = TraversalOps<CallOp, RCallOp, FirstThatOp,
                                         RFirstThatOp, PrintableOp,
                                         ReadableOp>;

#endif // TRAVERSALOPS_H
