
#include <iostream>
#include <fstream> // ofstream para escribir en archivo
#include <string>
#include <sstream>
#include <thread>
#include <vector>
#include "foreach.h"
#include "containers/vector.h"
#include "containers/linkedlist.h"
#include "Demos.h"
using namespace std;

void AddOne(GeneralNode<TX> &node) {
    node.value() += 1;
}

template <typename T>
void AddX(GeneralNode<T> &node, T x) {
    node.value() += x;
}

void Square(GeneralNode<TX> &node) {
    node.value() *= node.value();
}

// Predicado generico para FirstThat/call: true si el valor del nodo es
// mayor que 'threshold'. Generico en Node y T para servir tanto a
// Vector<VectorAscTraits<TX>> (int) como a Vector<VectorAscTraits<string>>.
template <typename Node, typename T>
bool IsGreaterThan(Node &node, T threshold) {
    return node.getValue() > threshold;
}

template <typename Node>
void PrintNode(Node &node, ostream &os) {
    os << node << " ";
}
const int NThreads = 5;

// Inserta secuencialmente cada pareja (valor, ref) de 'values' en el
// Container, una por una. La insercion concurrente queda aislada en
// DemoRaceCondition(), que es donde se estudia esa problematica.
template <typename Container>
void InsertElements(Container &container,
                     const vector<pair<typename Container::value_type, Ref>> &values) {
    for (const auto &v : values)
        container.push_back(v.first, v.second);
}

template <typename Container>
void TestContainer(Container &container,
                    const vector<pair<typename Container::value_type, Ref>> &values,
                    const string &filename) {
    InsertElements(container, values);

    // Impresion usando write()
    cout << "Container using write(): ";
    container.write(cout);
    cout << endl;

    // Escritura hacia un archivo, en modo append para acumular cada estado
    // (el archivo se deja vacio una vez al inicio, ver DemoVector)
    ofstream of(filename, ios::app);
    container.write(of);
    of << endl;
    of.close();

    // Escritura en pantalla usando cout directamente (operator<<)
    cout << "Container using cout directly: ";
    cout << container << endl;
}

// Prueba los recorridos del Container hacia adelante (begin/end) y hacia
// atras (rbegin/rend) solamente, imprimiendo los elementos en cada sentido.
template <typename Container>
void TestTraversal(Container &container) {
    using Node = typename Container::Node;

    cout << "Forward traversal:  [";
    container.call(PrintNode<Node>, cout);
    cout << "]" << endl;

    cout << "Backward traversal: [";
    container.rcall(PrintNode<Node>, cout);
    cout << "]" << endl;

    // Prueba de call()/FirstThat() con valor de retorno: busca el primer
    // nodo cuyo valor sea mayor que el del primer elemento del container.
    if (!container.empty()) {
        auto threshold = (*container.begin()).getValue();
        Node &found = container.FirstThat(IsGreaterThan<Node, decltype(threshold)>, threshold);
        cout << "FirstThat (primer valor > " << threshold << "): " << found << endl;
    }
}

void DemoVector() {
    // Dejamos los archivos vacios para que TestContainer acumule (append)
    // el estado del container tras cada paso
    ofstream("vector.txt", ios::trunc).close();

    // Cada elemento es una pareja (valor, ref) que se guarda en un Node;
    // el constructor initializer_list arma el Vector inicial de una vez
    Vector<VectorAscTraits<TX>> vec({{0, 10}, {1, 11}, {2, 12}, {3, 13}, {4, 14}});
    TestContainer(vec, {{5, 15}, {6, 16}, {7, 17}, {8, 18}, {9, 19}}, "vector.txt");
    TestTraversal(vec);

    ofstream("vector_str.txt", ios::trunc).close();
    Vector<VectorAscTraits<string>> strVec;
    TestContainer(strVec, {{"Hello", 1}, {"World", 2}}, "vector_str.txt");
    TestTraversal(strVec);
}

// Insertamos muchos elementos (generados en un loop, no a mano)
// concurrentemente y comparamos cuantos deberian haber entrado contra
// cuantos entraron realmente. Si push_back no estuviera sincronizado
// (sin el mutex/lock_guard actual) esto perderia inserciones o crashearia;
// con el mutex protegiendo push_back/resize, deberia dar siempre 0 perdidas.
void DemoRaceCondition() {
    const size_t N = 200000;

    vector<pair<TX, Ref>> values(N);
    for (size_t i = 0; i < N; ++i)
        values[i] = {static_cast<TX>(i), static_cast<Ref>(i)};

    Vector<VectorAscTraits<TX>> vec;

    // Insercion concurrente: NThreads workers insertando en paralelo sobre
    // el mismo Vector, cada uno con un subconjunto entrelazado (stride)
    size_t n = values.size();
    vector<thread> workers;
    for (int t = 0; t < NThreads; ++t) {
        workers.emplace_back([&vec, &values, n, t](){
            for (size_t i = t; i < n; i += NThreads)
                vec.push_back(values[i].first, values[i].second);
        });
    }
    for (auto &worker : workers)
        worker.join();

    long long expectedSum = 0;
    for (auto &v : values) expectedSum += v.first;

    long long actualSum = 0;
    for (size_t i = 0; i < vec.size(); ++i) actualSum += vec[i].getValue();

    cout << "DemoRaceCondition: se esperaban " << N << " elementos, "
         << "el Vector quedo con " << vec.size() << endl;
    cout << "  suma esperada = " << expectedSum
         << ", suma obtenida = " << actualSum << endl;

    if (vec.size() != N || actualSum != expectedSum)
        cout << "  *** Race condition detectada: se perdieron inserciones (push_back / resize sin sincronizar) ***" << endl;
    else
        cout << "  No se perdio ningun elemento: el mutex de push_back/resize "
             << "sincroniza correctamente las inserciones concurrentes" << endl;
}

void DemoLinkedList()
{
    using List = LinkedList<LinkedListAscTraits<TX>>;
    using Node = List::Node;

    cout << "\n--- Demo LinkedList ---" << endl;

    // Lista vacia e insercion al final: conserva el orden de llegada.
    List list;
    cout << "Lista vacia: " << list << endl;
    list.push_back(10, 1);
    list.push_back(30, 3);
    cout << "push_back: " << list << endl;

    // insert mantiene el orden si la lista ya esta ordenada.
    list.insert(5, 0);   // Al principio.
    list.insert(20, 2);  // En el medio.
    list.insert(40, 4);  // Al final.
    list.push_back(50, 5); // Comprueba que la cola se actualizo.
    cout << "Despues de insert y push_back: " << list << endl;

    // Recorrido manual con begin(), end(), * y ++.
    cout << "Con iteradores: ";
    for (auto it = list.begin(); it != list.end(); ++it)
        cout << *it << " ";
    cout << endl;

    cout << "call (hacia adelante): ";
    list.call(PrintNode<Node>, cout);
    cout << "\nrcall (hacia atras): ";
    list.rcall(PrintNode<Node>, cout);
    cout << endl;

    // Modificar valores y buscar el primero que cumple una condicion.
    list.ApplyFunction(AddX<TX>, 1);
    cout << "Sumar 1 a cada valor: " << list << endl;
    Node &found = list.FirstThat(IsGreaterThan<Node, TX>, 20);
    cout << "Primer valor mayor que 20: " << found << endl;
    Node &last = list.rcall(IsGreaterThan<Node, TX>, 20);
    cout << "Desde el final, primer valor mayor que 20: " << last << endl;

    try {
        list.FirstThat(IsGreaterThan<Node, TX>, 100);
    } catch (const runtime_error &error) {
        cout << "Busqueda sin coincidencias: " << error.what() << endl;
    }

    // Guardar en un archivo y recuperar los mismos nodos.
    {
        ofstream output("linkedlist.txt");
        if (!output || !list.write(output)) {
            cerr << "No se pudo guardar linkedlist.txt" << endl;
            return;
        }
    } // Cierra el archivo antes de leerlo.

    List recovered;
    ifstream input("linkedlist.txt");
    if (!input || !recovered.read(input)) {
        cerr << "No se pudo leer linkedlist.txt" << endl;
        return;
    }
    cout << "Lista recuperada del archivo: " << recovered << endl;

    // operator>> reemplaza el contenido; una entrada invalida lo conserva.
    istringstream valid("[(7,70),(8,80)]");
    valid >> recovered;
    cout << "Lectura con operator>>: " << recovered << endl;
    istringstream invalid("[(9,90)"); // Falta el cierre de la lista.
    invalid >> recovered;
    if (invalid.fail())
        cout << "Entrada invalida, lista conservada: " << recovered << endl;

    // clear libera los nodos y permite reutilizar la lista.
    list.clear();
    cout << "Despues de clear: " << list << endl;
    cout << "begin() == end(): " << (list.begin() == list.end()) << endl;
    list.push_back(99, 9);
    cout << "Lista reutilizada: " << list << endl;

    // La misma estructura tambien admite strings.
    LinkedList<LinkedListAscTraits<string>> words;
    words.push_back("Hola", 1);
    words.push_back("Mundo", 2);
    cout << "Lista de strings: " << words << endl;
    istringstream text("[(Hola mundo,3),(Lista enlazada,4)]");
    words.read(text);
    cout << "Strings leidos con espacios: " << words << endl;
    // Al terminar, los destructores liberan los nodos de cada lista.
}
