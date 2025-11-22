#include <iostream>
#include <vector>
#include <map>
#include <list>
#include <fstream>

using namespace std;

vector<bool> bitcode;
map<char, vector<bool>> mappingtable;

class Node {

public:
    int a;
    char c;
    Node* left, * right;

    /// конструктор без параметров
    Node() {
        left = right = NULL;
    }

    ///конструктор с двумя переменными
    Node(Node* L, Node* R) {
        left = L;
        right = R;
        a = L->a + R->a;
    }
};


///структура для сравнения значений
struct MyCompare {
    bool operator()(const Node* l, const Node* r) const {
        return l->a < r->a;
    }
};

// В результате этой рекурсии словарь mappingtable заполняется таблицей кодирования, где каждый символ сопоставляется его уникальному битовому коду.
/// функция для заполнения словаря таблцей кодирования
void Table(Node* root) {
    if (root->left != NULL) {
        bitcode.push_back(0);
        Table(root->left);
        bitcode.pop_back();
    }
    if (root->right != NULL) {
        bitcode.push_back(1);
        Table(root->right);
        bitcode.pop_back();
    }
    if (root->left == NULL && root->right == NULL) {
        mappingtable[root->c] = bitcode;
    }
}


//Данная функция считывает символы из файла "text.txt", кодирует их, используя таблицу соответствия "mappingtable",
// и записывает закодированные данные как бинарные байты в файл "encoded.txt". Когда байт заполняется 8-ю битами,
// он записывается в выходной файл, а временная переменная `buf` сбрасывается для следующего байта.

/// функция для создания закодированного файла
void CreateEncodedFile() {

    ifstream f("text.txt", ios::binary);
    ofstream encodedFile("encoded.txt", ios::binary);
    int count = 0; // счётчик битов
    char buf = 0; // переменная для временного хранения байта

    while (!f.eof()) {

        char c = f.get(); // считываем символы
        vector<bool> x = mappingtable[c]; // получаем кодировку символа из словаря

        for (int n = 0; n < x.size(); n++) {
            buf = buf | x[n] << (7 - count);
            count++;
            if (count == 8) {
                count = 0;
                encodedFile.write((char*)&buf, sizeof(buf));
                buf = 0;
            }
        }
    }

    f.close();
    encodedFile.close();

}

// Этот фрагмент кода отвечает за создание закодированного файла на основе данных, считанных из файла "text.txt".
// Сначала открывается файл "text.txt" для чтения,
// а также создается файл "encoded.txt" для записи.
// Затем инициализируются переменные count и buf для отслеживания битов.

/// функция для создания раскодированного файла
void CreateDecodedFile(Node* root) {

    ifstream encodedFile("encoded.txt", ios::binary);
    ofstream decodedFile("decoded.txt", ios::binary);

    Node* p = root; // указатель на корневой узел
    int count = 0;
    char byte;
    byte = encodedFile.get(); // считываем первый байт из файла

    while (!encodedFile.eof()) {

        bool b = byte & (1 << (7 - count));

        if (b) {
            p = p->right;
        }
        else {
            p = p->left;
        }

        if (p->left == nullptr && p->right == nullptr) {
            if (p != nullptr && p->left == nullptr && p->right == nullptr) {
                decodedFile << p->c;
                p = root;
            }
        }

        count++;

        if (count == 8) {
            count = 0;
            byte = encodedFile.get();
        }
    }

    encodedFile.close();
    decodedFile.close();
}




int main(int argc, char* argv[]) {

    ifstream f("text.txt", ios::binary);
    map <char, int> m;

    while (!f.eof()) {
        char c = f.get();
        m[c]++;
    }
    list<Node*> t;
    for (map<char, int>::iterator itr = m.begin(); itr != m.end(); ++itr) {
        Node* p = new Node;
        p->c = itr->first;
        p->a = itr->second;
        t.push_back(p);
    }

    while (t.size() != 1) {
        t.sort(MyCompare());
        Node* Lchild = t.front();
        t.pop_front();
        Node* Rchild = t.front();
        t.pop_front();
        Node* parent = new Node(Lchild, Rchild);
        t.push_back(parent);
    }

    Node* root = t.front();

    Table(root);
    CreateEncodedFile();
    CreateDecodedFile(root);

    return 0;

}