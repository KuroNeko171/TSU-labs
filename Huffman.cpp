
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
    Node() { left = right = NULL; }
    Node(Node* L, Node* R) { left = L; right = R; a = L->a + R->a; }
};

struct MyCompare {
    bool operator()(const Node* l, const Node* r) const { return l->a < r->a; }
};

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

// === Сохраняем таблицу частот в начало закодированного файла ===
void SaveFrequencyTable(const map<char, int>& freq, ofstream& encodedFile) {
    int count = freq.size();
    encodedFile.write((char*)&count, sizeof(int));  // сколько символов
    for (auto& p : freq) {
        encodedFile.write(&p.first, 1);              // символ
        encodedFile.write((char*)&p.second, sizeof(int)); // частота
    }
}

// === Читаем таблицу частот из файла ===
map<char, int> LoadFrequencyTable(ifstream& f) {
    map<char, int> freq;
    int count;
    f.read((char*)&count, sizeof(int));
    for (int i = 0; i < count; i++) {
        char c;
        int fr;
        f.read(&c, 1);
        f.read((char*)&fr, sizeof(int));
        freq[c] = fr;
    }
    return freq;
}

// Кодирование (теперь сохраняет таблицу частот)
void CreateEncodedFile(const map<char, int>& freq) {
    ofstream encodedFile("encoded.txt", ios::binary);
    SaveFrequencyTable(freq, encodedFile);  // ← вот и всё, что добавили

    ifstream f("text.txt", ios::binary);
    int count = 0;
    char buf = 0;
    while (!f.eof()) {
        char c = f.get();
        if (f.eof()) break;
        vector<bool> x = mappingtable[c];
        for (bool bit : x) {
            buf = buf | (bit << (7 - count));
            count++;
            if (count == 8) {
                encodedFile.write(&buf, 1);
                count = 0;
                buf = 0;
            }
        }
    }
    if (count != 0) {
        buf <<= (8 - count);
        encodedFile.write(&buf, 1);
    }
    f.close();
    encodedFile.close();
}

// Декодирование
void CreateDecodedFile() {
    ifstream encodedFile("encoded.txt", ios::binary);
    if (!encodedFile) {
        cout << "Error: encoded.txt file not found\n";
        return;
    }

    // 1. Читаем таблицу частот
    map<char, int> freq = LoadFrequencyTable(encodedFile);

    // 2. По ней строим дерево заново
    list<Node*> t;
    for (auto& p : freq) {
        Node* pnode = new Node;
        pnode->c = p.first;
        pnode->a = p.second;
        t.push_back(pnode);
    }
    while (t.size() != 1) {
        t.sort(MyCompare());
        Node* L = t.front(); t.pop_front();
        Node* R = t.front(); t.pop_front();
        Node* parent = new Node(L, R);
        t.push_back(parent);
    }
    Node* root = t.front();

    // 3. Строим таблицу кодов
    mappingtable.clear();
    Table(root);

    // 4. Декодируем 
    ofstream decodedFile("decoded.txt", ios::binary);
    Node* p = root;
    int count = 0;
    char byte;
    while (encodedFile.get(byte)) {
        for (int i = 7; i >= 0; i--) {
            bool b = byte & (1 << i);
            p = b ? p->right : p->left;
            if (p->left == NULL && p->right == NULL) {
                decodedFile.put(p->c);
                p = root;
            }
        }
    }
    decodedFile.close();
    encodedFile.close();
}

int main() {
    cout << "1 - Encode text.txt\n";
    cout << "2 - Decode encoded.txt\n";
    cout << "Choice: ";
    int mode; cin >> mode;

    if (mode == 1) {
        // === Подсчёт частот и построение дерева ===
        ifstream f("text.txt", ios::binary);
        map<char, int> m;
        while (!f.eof()) {
            char c = f.get();
            if (f.eof()) break;
            m[c]++;
        }
        f.close();

        list<Node*> t;
        for (auto& p : m) {
            Node* pnode = new Node;
            pnode->c = p.first;
            pnode->a = p.second;
            t.push_back(pnode);
        }
        while (t.size() != 1) {
            t.sort(MyCompare());
            Node* L = t.front(); t.pop_front();
            Node* R = t.front(); t.pop_front();
            Node* parent = new Node(L, R);
            t.push_back(parent);
        }
        Node* root = t.front();
        Table(root);
        CreateEncodedFile(m);  // ← передаём частоты
        cout << "Encoding completed → encoded.txt\n";
    }
    else if (mode == 2) {
        CreateDecodedFile();
        cout << "Decoding completed → decoded.txt\n";
    }
    else {
        cout << "Invalid mode\n";
    }
    return 0;
}
