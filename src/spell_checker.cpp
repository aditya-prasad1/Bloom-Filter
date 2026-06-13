#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <algorithm>

using namespace std;

uint64_t FNV_OFFSET = 14695981039346656037ULL;
uint64_t FNV_PRIME  = 1099511628211ULL;

class BloomFilter {
public:
    size_t m, k, n_inserted;
    vector<bool> bits;

    BloomFilter(size_t n, double p) {
        m = (size_t)ceil(-1.0 * n * log(p) / (log(2) * log(2)));
        k = (size_t)round((double)m / n * log(2));
        if (k < 1) k = 1;
        n_inserted = 0;
        bits.assign(m, false);
    }

    void insert(string key) {
        uint64_t h1 = fnv1a(key, FNV_OFFSET);
        uint64_t h2 = fnv1a(key, h1);
        for (size_t i = 0; i < k; i++)
            bits[get_hash(h1, h2, i)] = true;
        n_inserted++;
    }

    bool contains(string key) {
        uint64_t h1 = fnv1a(key, FNV_OFFSET);
        uint64_t h2 = fnv1a(key, h1);
        for (size_t i = 0; i < k; i++)
            if (!bits[get_hash(h1, h2, i)]) return false;
        return true;
    }

    uint64_t fnv1a(string key, uint64_t seed) {
        uint64_t hash = seed;
        for (char c : key) { hash ^= (unsigned char)c; hash *= FNV_PRIME; }
        return hash;
    }

    size_t get_hash(uint64_t h1, uint64_t h2, size_t i) {
        return (h1 + i * h2) % m;
    }
};

// lowercase and strip punctuation from a word
string clean(string word) {
    string result = "";
    for (char c : word)
        if (isalpha(c)) result += tolower(c);
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage: spell_checker <dictionary.txt> <input.txt>" << endl;
        return 1;
    }

    string dict_file  = argv[1];
    string input_file = argv[2];

    // load dictionary into bloom filter
    ifstream dict(dict_file);
    if (!dict.is_open()) {
        cout << "Error: could not open dictionary file: " << dict_file << endl;
        return 1;
    }

    // count words first to size the filter
    int word_count = 0;
    string word;
    while (getline(dict, word)) if (!word.empty()) word_count++;
    dict.clear();
    dict.seekg(0);

    BloomFilter bf(max(word_count, 1), 0.01);  // 1% FPR

    while (getline(dict, word)) {
        word = clean(word);
        if (!word.empty()) bf.insert(word);
    }
    dict.close();

    cout << "Dictionary loaded: " << bf.n_inserted << " words ("
         << bf.m / 8 / 1024.0 << " KB)" << endl << endl;

    // read input file and check each word
    ifstream input(input_file);
    if (!input.is_open()) {
        cout << "Error: could not open input file: " << input_file << endl;
        return 1;
    }

    vector<string> misspelled;
    int total_words = 0;
    string line;
    int line_num = 0;

    while (getline(input, line)) {
        line_num++;
        istringstream iss(line);
        string token;
        while (iss >> token) {
            string w = clean(token);
            if (w.empty()) continue;
            total_words++;
            if (!bf.contains(w)) {
                misspelled.push_back(w);
                cout << "Line " << line_num << ": \"" << w << "\" not found in dictionary" << endl;
            }
        }
    }
    
    input.close();

    cout << endl;
    cout << "Total words checked: " << total_words << endl;
    cout << "Possibly misspelled: " << misspelled.size() << endl;
    cout << "Note: false positives possible at ~1% rate" << endl;

    return 0;
}
