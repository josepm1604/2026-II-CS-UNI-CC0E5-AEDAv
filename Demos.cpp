
#include <iostream>
#include <fstream>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <unistd.h>
#include "containers/aliases.h"
#include "Demos.h"

// ---------------------------------------------------------------------------
// Utilidades generales
// ---------------------------------------------------------------------------

// Pares (valor, ref) que se insertan en un contenedor
template <typename Container>
using Values = std::vector<std::pair<typename Container::value_type, Ref>>;

void AddOne(GeneralNode<TX>& node) {
    node.value() += 1;
}

template <typename T> void AddX(GeneralNode<T>& node, T x) {
    node.value() += x;
}

void Square(GeneralNode<TX>& node) {
    node.value() *= node.value();
}

template <typename Node> void PrintNode(const Node& node, std::ostream& os) {
    os << node << " ";
}
const int NThreads = 5;

template <typename Value>
void PrintColor(const Value& value,
                const std::string& color = "\033[1;36m",
                std::ostream& os = std::cout) {
    const bool isTerminal = (&os == &std::cout && isatty(fileno(stdout))) ||
                            (&os == &std::cerr && isatty(fileno(stderr)));
    const bool useColor = isTerminal && std::getenv("NO_COLOR") == nullptr;
    if (useColor) os << color;
    os << value;
    if (useColor) os << "\033[0m";
    os << '\n';
}

template <typename Function> void WithDemoFile(const std::string& filename, Function function) {
    auto removeFile = [&filename] {
        if (std::remove(filename.c_str()) != 0 && errno != ENOENT)
            std::cerr << "No se pudo borrar " << filename << '\n';
    };
    try {
        function();
    } catch (...) {
        removeFile();
        throw;
    }
    removeFile();
}

template <typename Container> void SaveTo(const std::string& filename, const Container& container) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file) throw std::runtime_error("No se pudo crear " + filename);
    file << container << '\n';
    if (!file) throw std::runtime_error("No se pudo escribir " + filename);
}

template <typename Container> void LoadFrom(const std::string& filename, Container& container) {
    std::ifstream file(filename);
    if (!file) throw std::runtime_error("No se pudo abrir " + filename);
    if (!(file >> container)) throw std::runtime_error("No se pudo leer " + filename);
}

template <typename Container> void PushAll(Container& container, const Values<Container>& values) {
    for (const auto& [value, ref] : values)
        container.push_back(value, ref);
}

template <typename Container> void InsertAll(Container& container, const Values<Container>& values) {
    for (const auto& [value, ref] : values)
        container.insert(value, ref);
}

// ---------------------------------------------------------------------------
// Rangos e impresion de recorridos
// ---------------------------------------------------------------------------

// Par de iteradores usable en un range-for
template <typename Iterator> struct Range {
    Iterator first, last;
    Iterator begin() const { return first; }
    Iterator end() const { return last; }
};

template <typename Container> auto ReverseRange(const Container& container) {
    return Range{container.rbegin(), container.rend()};
}

// Imprime "label: [ ... ]" dejando que body escriba el contenido
template <typename Body> void PrintBracketed(const std::string& label, Body body) {
    std::cout << label << ": [";
    body();
    std::cout << "]\n";
}

template <typename Traversal>
void PrintTraversal(const std::string& label, const Traversal& traversal) {
    PrintBracketed(label, [&] {
        for (auto&& node : traversal)
            std::cout << node << " ";
    });
}

template <typename Traversal> std::vector<Ref> CollectRefs(const Traversal& traversal) {
    std::vector<Ref> refs;
    for (auto&& node : traversal)
        refs.push_back(node.getRef());
    return refs;
}

template <typename Forward, typename Backward>
void CheckReverse(const std::string& name, const Forward& forward, const Backward& backward) {
    std::vector<Ref> refs = CollectRefs(backward);
    std::reverse(refs.begin(), refs.end());
    if (refs != CollectRefs(forward))
        throw std::runtime_error(name + ": el recorrido inverso no coincide");
}

template <typename Iterator> void PrintCircularTraversal(Iterator begin, Iterator end) {
    if (begin == end) {
        std::cout << "[]\n";
        return;
    }

    std::cout << *begin;
    auto it = begin;
    for (++it; it != end; ++it)
        std::cout << " -> " << *it;
    std::cout << " -> (vuelve a " << *begin << ")\n";
}

// ---------------------------------------------------------------------------
// Pruebas genericas para cualquier contenedor
// ---------------------------------------------------------------------------

template <typename Container>
void TestContainer(Container& container, const Values<Container>& values,
                   const std::string& filename) {
    WithDemoFile(filename, [&] {
        PrintColor("TestContainer (" + filename + ")");
        PushAll(container, values);

        if constexpr (requires { container.write(std::cout); }) {
            std::cout << "Container using write(): ";
            container.write(std::cout);
            std::cout << '\n';
        }

        SaveTo(filename, container);

        std::cout << "Container using cout directly: ";
        PrintColor(container);
    });
}

template <typename Container>
void TestTraversal(Container& container, const std::string& name) {
    PrintColor("TestTraversal (" + name + ")");
    using Node = typename Container::Node;

    PrintBracketed("Forward traversal ", [&] { container.call(PrintNode<Node>, std::cout); });
    PrintTraversal("Forward iterators ", container);

    if constexpr (ReverseTraversable<Container>) {
        PrintBracketed("Backward traversal", [&] { container.rcall(PrintNode<Node>, std::cout); });
        PrintTraversal("Backward iterators", ReverseRange(container));
    }

    auto it = container.begin();
    if (it != container.end() && ++it != container.end()) {
        Ref target = (*it).getRef();
        const Node& found = container.FirstThat(
            [](const Node& node, Ref ref) { return node.getRef() == ref; }, target);
        std::cout << "FirstThat (ref " << target << "): " << found << '\n';
    }
}

template <typename Container>
void TestGenericContainer(Container& container, const std::string& filename = "container.txt") {
    WithDemoFile(filename, [&] {
        PrintColor("TestGenericContainer (" + filename + ")");
        std::cout << "Vacio: " << container.empty() << '\n';

        PushAll(container, {{1, 101}, {3, 103}, {7, 107}});
        std::cout << "Luego de push_back: ";
        PrintColor(container);

        if constexpr (requires(Container& c) { c.insert(5, 105); }) {
            InsertAll(container, {{5, 105}, {2, 102}, {9, 109}});
            std::cout << "Luego de insert: ";
            PrintColor(container);

            Container copia(container);
            Container asignada;
            asignada = container;
            copia.insert(4, 104);
            std::cout << "Original: ";
            PrintColor(container);
            std::cout << "Copia modificada: ";
            PrintColor(copia);
            std::cout << "Asignada: ";
            PrintColor(asignada);
        }

        SaveTo(filename, container);
        std::cout << "Escrito en " << filename << '\n';

        Container leida;
        LoadFrom(filename, leida);
        std::cout << "Leido desde " << filename << ": " << leida << '\n';

        Container reutilizada;
        reutilizada.push_back(1, 101);
        reutilizada.clear();
        std::cout << "Luego de clear, vacia: " << reutilizada.empty() << '\n';
        reutilizada.push_back(10, 110);
        std::cout << "Luego de reutilizarla: ";
        PrintColor(reutilizada);
    });
}

template <typename Container>
void TestInvalidRead(Container& container, const std::string& name) {
    std::ostringstream before;
    before << container;
    std::istringstream invalid("[(7,70),broken]");
    if (invalid >> container) throw std::runtime_error(name + ": lectura invalida aceptada");
    std::ostringstream after;
    after << container;
    if (before.str() != after.str())
        throw std::runtime_error(name + ": la lectura invalida cambio los datos");
}

template <typename Container>
void TestCopyAndClear(Container& container, const std::string& name) {
    PrintColor("TestCopyAndClear (" + name + ")");
    container.clear();
    container.push_back(8, 80);
    container.clear();
    container.clear();
    if (!container.empty() || container.begin() != container.end())
        throw std::runtime_error(name + ": clear no dejo el contenedor vacio");

    container.push_back(3, 30);
    if constexpr (requires { container.insert(1, 10); })
        InsertAll(container, {{1, 10}, {5, 50}});
    Container copy(container);
    container = container;
    if (copy.empty() || container.empty()) throw std::runtime_error(name + ": fallo la copia");

    container.clear();
    container.push_back(9, 90);
    if (container.empty() || (*container.begin()).getValue() != 9)
        throw std::runtime_error(name + ": no se pudo reutilizar el contenedor");
    std::cout << name << ": clear y copia OK\n";
}

template <typename Container>
void TestRaceCondition(const std::string& name, const Values<Container>& values) {
    PrintColor("TestRaceCondition (" + name + ")");
    Container container;

    size_t n = values.size();
    std::vector<std::thread> workers;
    for (int t = 0; t < NThreads; ++t) {
        workers.emplace_back([&container, &values, n, t]() {
            for (size_t i = t; i < n; i += NThreads)
                container.push_back(values[i].first, values[i].second);
        });
    }
    for (auto& worker : workers)
        worker.join();

    Ref expectedSum = 0;
    for (auto& v : values)
        expectedSum += v.first;

    size_t actualCount = 0;
    Ref actualSum = 0;
    for (auto&& node : container) {
        ++actualCount;
        actualSum += node.getValue();
    }

    std::cout << "DemoRaceCondition (" << name << "): se esperaban " << n
              << " elementos, el Container quedo con " << actualCount << '\n';
    std::cout << "  suma esperada = " << expectedSum << ", suma obtenida = " << actualSum << '\n';

    if (actualCount != n || actualSum != expectedSum)
        std::cout << "  *** Race condition detectada: se perdieron inserciones (push_back sin "
                     "sincronizar) ***\n";
    else
        std::cout << "  No se perdio ningun elemento: el mutex de push_back "
                  << "sincroniza correctamente las inserciones concurrentes\n";
}

template <typename Container>
void TestStructure(Container& container, const std::string& name) {
    PrintColor(name);
    const Values<Container> values{{5, 50}, {1, 10}, {4, 40}, {2, 20}, {7, 70}, {5, 51}};

    TestContainer(container, values, name + ".txt");
    TestTraversal(container, name);

    if constexpr (requires {
                      container.top();
                      container.pop();
                  }) {
        std::cout << "Top: " << container.top() << '\n';
        std::cout << "Orden de extraccion: ";
        while (!container.empty())
            std::cout << container.pop() << ' ';
        std::cout << '\n';
    }

    container.clear();
    TestGenericContainer(container, name + "_persist.txt");
    TestInvalidRead(container, name);

    if constexpr (std::is_copy_constructible_v<Container> && std::is_copy_assignable_v<Container>) {
        Container copyAndClear;
        TestCopyAndClear(copyAndClear, name);
    }

    TestRaceCondition<Container>(name, values);
}

template <typename Container> void TestStructure(const std::string& name) {
    Container container;
    TestStructure(container, name);
}

// Inserta N elementos desde NThreads hilos y verifica que no se pierda ninguno
template <typename Container> void DemoRaceCondition(const std::string& name) {
    PrintColor("DemoRaceCondition (" + name + ")");
    const size_t N = 200000;
    Values<Container> values(N);
    for (size_t i = 0; i < N; ++i)
        values[i] = {static_cast<TX>(i), static_cast<Ref>(i)};

    std::cout << "DemoRaceCondition (" << NThreads << " hilos, push_back):\n";
    TestRaceCondition<Container>(name, values);
}

// ---------------------------------------------------------------------------
// Vector
// ---------------------------------------------------------------------------

void DemoRaceCondition() { DemoRaceCondition<VectorAsc<TX>>("Vector"); }

void DemoVector() {
    PrintColor("DemoVector");

    VectorAsc<TX> vec;
    TestStructure(vec, "Vector");

    VectorAsc<std::string> strVec;
    TestContainer(strVec, {{"Hello", 1}, {"World", 2}}, "vector_str.txt");
    TestTraversal(strVec, "Vector<string>");

    const auto& constVec = vec;
    CheckReverse("Vector const", constVec, ReverseRange(constVec));
    if (CollectRefs(constVec).size() != vec.size())
        throw std::runtime_error("Vector const: recorridos inconsistentes");

    VectorAsc<TX> emptyVector;
    if (emptyVector.rbegin() != emptyVector.rend())
        throw std::runtime_error("Vector vacio: recorrido inverso incorrecto");
    const auto& constEmpty = emptyVector;
    if (constEmpty.begin() != constEmpty.end() || constEmpty.rbegin() != constEmpty.rend())
        throw std::runtime_error("Vector const vacio: recorrido incorrecto");
    std::cout << "Vector: recorrido const y vacio OK\n";
}

// ---------------------------------------------------------------------------
// Listas enlazadas: una sola demo para las 4 variantes
// ---------------------------------------------------------------------------

template <typename List> constexpr bool IsCircular = false;
template <typename Traits, bool Double>
constexpr bool IsCircular<LinkedList<Traits, Double, true>> = true;

// Muestra el ciclo de una lista circular en los sentidos que soporte
template <typename List> void PrintCycles(const List& list, const std::string& label) {
    if constexpr (IsCircular<List>) {
        if constexpr (ReverseTraversable<List>) {
            std::cout << "Ciclo" << label << " hacia adelante: ";
            PrintCircularTraversal(list.begin(), list.end());
            std::cout << "Ciclo" << label << " hacia atras: ";
            PrintCircularTraversal(list.rbegin(), list.rend());
        } else {
            std::cout << "Ciclo" << label << ": ";
            PrintCircularTraversal(list.begin(), list.end());
        }
    }
}

template <typename AscList, typename DescList> void DemoList(const std::string& name) {
    PrintColor("Demo" + name);
    AscList lista;
    TestStructure(lista, name);
    PrintCycles(lista, "");

    lista.ApplyFunction(AddOne);
    std::cout << "Tras AddOne: ";
    PrintColor(lista);
    lista.ApplyFunction(AddX<TX>, TX(10));
    std::cout << "Tras AddX(10): ";
    PrintColor(lista);

    DescList descendente;
    InsertAll(descendente, {{5, 105}, {3, 103}, {7, 107}});
    std::cout << "Lista descendente: ";
    PrintColor(descendente);
    PrintCycles(descendente, " descendente");

    DemoRaceCondition<AscList>(name);
}

void DemoLinkedList() {
    DemoList<LinkedAscList<TX>, LinkedDescList<TX>>("LinkedList");
}

void DemoCircularLinkedList() {
    DemoList<CircularLinkedAscList<TX>, CircularLinkedDescList<TX>>("CircularLinkedList");
}

void DemoDoubleLinkedList() {
    DemoList<DoubleLinkedAscList<TX>, DoubleLinkedDescList<TX>>("DoubleLinkedList");
}

void DemoCircularDoubleLinkedList() {
    DemoList<CircularDoubleLinkedAscList<TX>, CircularDoubleLinkedDescList<TX>>(
        "CircularDoubleLinkedList");
}

// ---------------------------------------------------------------------------
// Heap
// ---------------------------------------------------------------------------

void DemoHeap() {
    TestStructure<MinHeap<int>>("MinHeap");
    TestStructure<MaxHeap<int>>("MaxHeap");
}

// ---------------------------------------------------------------------------
// Arboles
// ---------------------------------------------------------------------------

template <typename Tree> void CheckTree(const Tree& tree, const std::string& name) {
    std::vector<TX> values;
    for (auto&& node : tree.InOrder())
        values.push_back(node.getValue());
    if (!std::is_sorted(values.begin(), values.end()) || values.size() != tree.size())
        throw std::runtime_error(name + ": el arbol perdio su orden");
}

template <typename Tree> void TestTreeTraversals(const Tree& tree, const std::string& name) {
    PrintColor("TestTreeTraversals (" + name + ")");
    using Node = typename Tree::Node;
    PrintTraversal("InOrder         ", tree.InOrder());
    PrintTraversal("ReverseInOrder  ", tree.ReverseInOrder());
    PrintTraversal("PreOrder        ", tree.PreOrder());
    PrintTraversal("ReversePreOrder ", tree.ReversePreOrder());
    PrintTraversal("PostOrder       ", tree.PostOrder());
    PrintTraversal("ReversePostOrder", tree.ReversePostOrder());

    PrintBracketed("for_each PreOrder", [&] {
        auto preOrder = tree.PreOrder();
        traversal_detail::for_each(preOrder.begin(), preOrder.end(), PrintNode<Node>, std::cout);
    });

    CheckReverse(name + " InOrder", tree.InOrder(), tree.ReverseInOrder());
    CheckReverse(name + " PreOrder", tree.PreOrder(), tree.ReversePreOrder());
    CheckReverse(name + " PostOrder", tree.PostOrder(), tree.ReversePostOrder());
    std::cout << name << ": los 6 recorridos OK\n";
}

template <typename Tree> void TestTreeRemove(Tree& tree, const std::string& name) {
    PrintColor("TestTreeRemove (" + name + ")");
    for (TX value : {20, 60, 50, 99}) {
        bool removed = tree.remove(value);
        std::cout << "remove(" << value << "): " << removed << '\n';
        if (tree.contains(value))
            throw std::runtime_error(name + ": remove no borro el valor");
        CheckTree(tree, name);
    }
    tree.print_sideways(std::cout);

    std::stringstream stream;
    stream << tree;
    Tree leido;
    stream >> leido;
    std::cout << "Persistencia luego de remove: " << leido << '\n';
    CheckTree(leido, name);
}

template <typename Tree> void TestTreeHeight(const std::string& name) {
    PrintColor("TestTreeHeight (" + name + ")");
    Tree tree;
    for (TX value = 1; value <= 15; ++value)
        tree.insert(value, value * 10);
    std::cout << "Insertando 1..15 en orden, altura = " << tree.height() << '\n';
    tree.print_sideways(std::cout);
    CheckTree(tree, name);
}

template <typename Tree> void TestBinaryTree(const std::string& name) {
    TestStructure<Tree>(name);

    Tree tree{{50, 500}, {30, 300}, {70, 700}, {20, 200}, {40, 400},
              {60, 600}, {80, 800}, {35, 350}, {65, 650}};
    PrintColor("Visualizacion (" + name + ")");
    tree.print_sideways(std::cout);
    TestTreeTraversals(tree, name);
    TestTreeRemove(tree, name);
    TestTreeHeight<Tree>(name);
}

void DemoBinaryTree() {
    TestBinaryTree<BinarySearchTree<TX>>("BinaryTree");
}

void DemoAVL() {
    TestBinaryTree<AVLSearchTree<TX>>("AVL");
    DemoRaceCondition<AVLSearchTree<TX>>("AVL");
}
