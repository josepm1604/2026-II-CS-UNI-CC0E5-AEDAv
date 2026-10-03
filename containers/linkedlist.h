#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <mutex>
#include <string>
#include <vector>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "vector.h"
#include "../foreach.h"

template <typename T>
class LinkedListNode : public GeneralNode<T> {
    using Node    = LinkedListNode<T>;
    using NodePtr = Node *;
  
    template <typename Traits>
    friend class LinkedList;

    Node *m_pNext = nullptr; // puntero al siguiente nodo
public:
    LinkedListNode(const T& value, Ref ref, NodePtr pNext) : GeneralNode<T>(value, ref), m_pNext(pNext){}
    // TODO: El operator<< deberia ir en GeneralNode, no en LinkedListNode, para que sea generico y reusable.
    // friend std::ostream &operator <<(std::ostream &os, const LinkedListNode<T> &node) {
    //     os << "(" << node.getValue() << "," << node.getRef() << ")";
    //     return os;
    // }
    NodePtr next() const { return m_pNext; } 
};

template <typename T>
class LinkedListForwardIterator : public GeneralIterator<LinkedListForwardIterator<T>, LinkedListNode<T>> {
public:
    using value_type        = LinkedListNode<T>;
    using MySelf            = LinkedListForwardIterator<T>;
    using Parent            = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    MySelf& operator++() { Parent::m_ptr = Parent::m_ptr->next(); return *this; } // Falta el tipo de retorno
};

template <typename T, typename _Compare = std::less<T>>
struct AscendingTraits {
    using value_type        = T;
    using Compare           = _Compare;
};
template <typename T>
struct LinkedListAscTraits : public AscendingTraits<T, std::less<T>> {
    using Node              = LinkedListNode<T>;
    using ForwardIterator   = LinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
};

template <typename T>
struct LinkedListDescTraits : public AscendingTraits<T, std::greater<T>> {
    using Node              = LinkedListNode<T>;
    using ForwardIterator   = LinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
};
template <typename Traits>
class LinkedList {
public:
    using value_type        = typename Traits::value_type;
    using Node              = typename Traits::Node;
    using NodePtr           = Node *;
    using ForwardIterator   = typename Traits::ForwardIterator;
    using Compare           = typename Traits::Compare;

private:
    NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
    NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada
    
    Compare m_comp; // comparador para ordenar los nodos de la lista
    // TODO: agregar mutex para sincronización de acceso concurrente
    mutable std::mutex m_mutex;             // mutex para sincronización, es necesario que sea mutable para poder usarlo en métodos const

    NodePtr NodePtrGetRoot() const { return m_pRoot; } // Falta el tipo de retorno
public:
    LinkedList() {}
    LinkedList(const LinkedList&);
    LinkedList& operator=(const LinkedList&);

    // TODO: implementar la destruccion en un metodo clear()
    void clear();
    // TODO: implementar destructor para liberar memoria de forma segura
    virtual ~LinkedList();

    // TODO: implementar métodos de iteración, push_back, etc.
    void push_back(const value_type& value, Ref ref);

private:
    void internalInsert(const value_type& value, Ref ref, NodePtr&rParent);
public:
    // TODO: implementar insert() para LinkedList
    void insert(const value_type& value, Ref ref){ 
        std::lock_guard<std::mutex> lock(m_mutex);  
        internalInsert(value, ref, m_pRoot); 
    } 
    
    // TODO: persistencia: write() y read() para LinkedList
    std::ostream &write(std::ostream &os) { return os << *this; }
    std::istream &read(std::istream &is)  { return is >> *this; }

    friend std::ostream &operator <<(std::ostream &os, const LinkedList<Traits> &list) {
        std::lock_guard<std::mutex> lock(list.m_mutex); 
        NodePtr current = list.m_pRoot;
        os << "[";
        while (current != nullptr) {
            // Asegura que sea GeneralNode para poder usar el operador<< definido en GeneralNode
            os << static_cast<const GeneralNode<typename Traits::value_type>&>(*current);
            current = current->next();
            if (current != nullptr) os << ",";
        }
        return os << "]";
    }

    friend std::istream &operator >>(std::istream &is, LinkedList<Traits> &list) {
        std::lock_guard<std::mutex> lock(list.m_mutex);
        LinkedList<Traits> tempList;
        char separator;
        auto fail = [&]() -> std::istream& { is.setstate(std::ios::failbit); return is; };

        if (!(is >> separator) || separator != '[') return fail();
        is >> std::ws;
        if (is.peek() == ']') {
            is.get();
        } else {
            do {
                GeneralNode<value_type> node;
                if (!(is >> node)) return fail();
                tempList.push_back(node.getValue(), node.getRef());
                if (!(is >> separator) || (separator != ',' && separator != ']')) return fail();
            } while (separator == ',');
        }

        if (!is) return fail();
        std::swap(list.m_pRoot, tempList.m_pRoot);
        std::swap(list.m_pTail, tempList.m_pTail);
        return is;
    }
    // Iterators
    ForwardIterator begin() { return ForwardIterator(m_pRoot); }
    ForwardIterator end()   { return ForwardIterator(nullptr); }

    

    // TODO: implementar ApplyFunction(), FirstThat(), call, rcall para LinkedList
    // Chequear que hago para evitar codigo repetido
    template <typename Func, typename... Args>
    void ApplyFunction(Func&& func, Args&&... args) {
      call(func,std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    Node& FirstThat(Func&& func, Args&&... args) {
      return call(func,std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    decltype(auto) call(Func&& func, Args&&... args) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if constexpr (std::is_void_v<std::invoke_result_t<Func,Node&,Args...>>)
            ::call(begin(), end(), func, std::forward<Args>(args)...);
        else
            return ::call(begin(), end(), func, std::forward<Args>(args)...);
    }

};


template <typename Traits>
LinkedList<Traits>& LinkedList<Traits>::operator=(const LinkedList<Traits>& other) {
    if (this == &other) return *this;

    LinkedList<Traits> tempList(other);
    std::lock_guard<std::mutex> lock(m_mutex);

    std::swap(m_pRoot, tempList.m_pRoot);
    std::swap(m_pTail, tempList.m_pTail);
    return *this;
}

template <typename Traits>
LinkedList<Traits>::LinkedList(const LinkedList<Traits>& other) : LinkedList() {
    std::lock_guard<std::mutex> lock(other.m_mutex);
    for (NodePtr current = other.m_pRoot; current != nullptr; current = current->next()) {
        push_back(current->getValue(), current->getRef());
    }
}


// push_back 
template <typename Traits>
void LinkedList<Traits>::push_back(const value_type& value, Ref ref){
    std::lock_guard<std::mutex> lock(m_mutex); 
    NodePtr newNode = new Node(value, ref, nullptr);
    
    if (m_pTail != nullptr) {
        m_pTail->m_pNext = newNode;
    } else {
        m_pRoot = newNode; 
    }

    m_pTail = newNode;
}


// clear 
template <typename Traits>
void LinkedList<Traits>::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    while(m_pRoot != nullptr) {
        NodePtr current = m_pRoot;
        m_pRoot = current->m_pNext;
        delete current;
    }

    m_pTail = nullptr; 
}

// destructor 
template <typename Traits>
LinkedList<Traits>::~LinkedList() {
    clear();
}

// TODO: explicar recursividad de cola de llamadas en insert() y internalInsert()
// La recursividad de cola, hace que la ultima llamada de internatInsert() sea la que inserta el nodo no queda trabajo despues de eso
template <typename Traits>
void LinkedList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr &rParent){
    if( rParent == nullptr || value < rParent->getValue() ) {
        NodePtr newNode = new Node(value, ref, rParent);

        if(rParent == nullptr) 
            m_pTail = newNode;
    
        rParent = newNode;
        return;
    } 
    internalInsert(value, ref, rParent->m_pNext);
}
#endif // __LINKEDLIST_H__
// Extender LinkedList, hacer todos los TODOS para hacer merge
// Eso es necesario para el sabado
