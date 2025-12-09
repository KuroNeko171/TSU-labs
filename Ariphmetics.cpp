#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cstdint>
#include <chrono>

using namespace std;

// Параметры диапазона (16-бит)
static const uint32_t MAX_RANGE = 0xFFFF;
static const uint32_t HALF = (MAX_RANGE + 1) / 2;
static const uint32_t QUARTER = HALF / 2;
static const uint32_t THREEQ = QUARTER * 3;

// Побитовая запись
class BitWriter {
    ofstream& out;
    uint8_t buffer = 0;
    int filled = 0;
public:
    BitWriter(ofstream& o) : out(o) {}
    void putbit(int b) {
        buffer = (buffer << 1) | (b & 1);
        filled++;
        if (filled == 8) {
            out.put((char)buffer);
            buffer = 0;
            filled = 0;
        }
    }
    void putfollowed(int b, uint32_t count) {
        for (uint32_t i = 0; i < count; i++) putbit(b);
    }
    void flush() {
        if (filled) {
            buffer <<= (8 - filled);
            out.put((char)buffer);
            filled = 0;
            buffer = 0;
        }
    }
};

// Побитовое чтение
class BitReader {
    ifstream& in;
    uint8_t buffer = 0;
    int left = 0;
public:
    BitReader(ifstream& i) : in(i) {}
    int getbit() {
        if (left == 0) {
            int c = in.get();
            if (c == EOF) return -1;
            buffer = (uint8_t)c;
            left = 8;
        }
        int bit = (buffer >> (left - 1)) & 1;
        left--;
        return bit;
    }
    int getbits(int n) {
        int v = 0;
        for (int i = 0; i < n; i++) {
            int b = getbit();
            if (b == -1) return -1;
            v = (v << 1) | b;
        }
        return v;
    }
};

// --------------------- Кодирование ---------------------
bool encode_file(double& ratio) {
    ifstream fin("text.txt", ios::binary);
    if (!fin.is_open()) {
        cout << "text.txt not found\n";
        return false;
    }

    array<uint32_t, 256> freq{};
    uint64_t total_symbols = 0;
    int c;

    while ((c = fin.get()) != EOF) {
        freq[(unsigned char)c]++;
        total_symbols++;
    }

    ofstream fout("encoded.txt", ios::binary);
    if (!fout.is_open()) {
        cout << "Cannot create encoded.txt\n";
        return false;
    }

    // Заголовок
    fout.write((char*)&total_symbols, sizeof(total_symbols));
    fout.write((char*)freq.data(), sizeof(uint32_t) * 256);

    if (total_symbols == 0) {
        ratio = 1.0;
        return true;
    }

    // Кумулятивные суммы
    vector<uint32_t> cum(257, 0);
    for (int i = 0; i < 256; i++) cum[i + 1] = cum[i] + freq[i];
    uint32_t total = cum[256];

    fin.clear();
    fin.seekg(0, ios::beg);

    BitWriter bw(fout);

    uint32_t low = 0, high = MAX_RANGE;
    uint32_t follow = 0;

    while ((c = fin.get()) != EOF) {
        unsigned char sym = (unsigned char)c;
        uint32_t range = high - low + 1;

        uint32_t new_low =
            low + (uint64_t)range * cum[sym] / total;
        uint32_t new_high =
            low + (uint64_t)range * cum[sym + 1] / total - 1;

        low = new_low;
        high = new_high;

        for (;;) {
            if (high < HALF) {
                bw.putbit(0);
                bw.putfollowed(1, follow);
                follow = 0;
            }
            else if (low >= HALF) {
                bw.putbit(1);
                bw.putfollowed(0, follow);
                follow = 0;
                low -= HALF;
                high -= HALF;
            }
            else if (low >= QUARTER && high < THREEQ) {
                follow++;
                low -= QUARTER;
                high -= QUARTER;
            }
            else break;

            low = (low << 1) & MAX_RANGE;
            high = ((high << 1) | 1) & MAX_RANGE;
        }
    }

    follow++;
    if (low < QUARTER) {
        bw.putbit(0);
        bw.putfollowed(1, follow);
    }
    else {
        bw.putbit(1);
        bw.putfollowed(0, follow);
    }

    bw.flush();

    fin.seekg(0, ios::end);
    fout.seekp(0, ios::end);
    double in_sz = fin.tellg();
    double out_sz = fout.tellp();
    ratio = out_sz / in_sz;

    return true;
}

// --------------------- Декодирование ---------------------
bool decode_file() {
    ifstream fin("encoded.txt", ios::binary);
    if (!fin.is_open()) {
        cout << "encoded.txt not found\n";
        return false;
    }

    uint64_t total_symbols = 0;
    array<uint32_t, 256> freq{};
    fin.read((char*)&total_symbols, sizeof(total_symbols));
    fin.read((char*)freq.data(), sizeof(uint32_t) * 256);

    vector<uint32_t> cum(257, 0);
    for (int i = 0; i < 256; i++) cum[i + 1] = cum[i] + freq[i];
    uint32_t total = cum[256];

    ofstream fout("decoded.txt", ios::binary);
    if (!fout.is_open()) {
        cout << "Cannot create decoded.txt\n";
        return false;
    }

    BitReader br(fin);
    int first16 = br.getbits(16);
    if (first16 < 0) first16 = 0;

    uint32_t value = first16;
    uint32_t low = 0, high = MAX_RANGE;

    for (uint64_t i = 0; i < total_symbols; i++) {
        uint32_t range = high - low + 1;
        uint32_t scaled = (uint64_t)(value - low + 1) * total - 1;
        scaled /= range;

        int L = 0, R = 256;
        while (R - L > 1) {
            int M = (L + R) / 2;
            if (cum[M] <= scaled) L = M;
            else R = M;
        }

        int sym = L;
        fout.put((char)sym);

        uint32_t new_low =
            low + (uint64_t)range * cum[sym] / total;
        uint32_t new_high =
            low + (uint64_t)range * cum[sym + 1] / total - 1;

        low = new_low;
        high = new_high;

        for (;;) {
            if (high < HALF) {
                // nothing
            }
            else if (low >= HALF) {
                value -= HALF;
                low -= HALF;
                high -= HALF;
            }
            else if (low >= QUARTER && high < THREEQ) {
                value -= QUARTER;
                low -= QUARTER;
                high -= QUARTER;
            }
            else break;

            low = (low << 1) & MAX_RANGE;
            high = ((high << 1) | 1) & MAX_RANGE;

            int bit = br.getbit();
            if (bit < 0) bit = 0;
            value = ((value << 1) | bit) & MAX_RANGE;
        }
    }

    return true;
}

// --------------------- Меню ---------------------
int main() {
    cout << "1 - Encode text.txt → encoded.txt\n";
    cout << "2 - Decode encoded.txt → decoded.txt\n";
    cout << "Choice: ";

    int mode;
    cin >> mode;

    if (mode == 1) {
        auto t0 = chrono::high_resolution_clock::now();

        double ratio = 0.0;
        if (!encode_file(ratio)) return 0;

        auto t1 = chrono::high_resolution_clock::now();
        double time_s = chrono::duration<double>(t1 - t0).count();

        cout << "Encoding completed.\n";
        cout << "Compression ratio = " << ratio << endl;
        cout << "Time = " << time_s << " sec\n";
    }
    else if (mode == 2) {
        auto t0 = chrono::high_resolution_clock::now();

        if (!decode_file()) return 0;

        auto t1 = chrono::high_resolution_clock::now();
        double time_s = chrono::duration<double>(t1 - t0).count();

        cout << "Decoding completed.\n";
        cout << "Time = " << time_s << " sec\n";
    }
    else {
        cout << "Invalid mode\n";
    }

    return 0;
}
