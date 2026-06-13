#include <iostream>
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <cassert>

using namespace std;

uint64_t FNV_OFFSET = 14695981039346656037ULL;
uint64_t FNV_PRIME  = 1099511628211ULL;
uint8_t  MAX_COUNT  = 15;

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

class CountingBloomFilter {
public:
    size_t m, k, n_inserted;
    vector<uint8_t> counters;

    CountingBloomFilter(size_t n, double p) {
        m = (size_t)ceil(-1.0 * n * log(p) / (log(2) * log(2)));
        k = (size_t)round((double)m / n * log(2));
        if (k < 1) k = 1;
        n_inserted = 0;
        counters.assign((m + 1) / 2, 0);
    }

    void insert(string key) {
        uint64_t h1 = fnv1a(key, FNV_OFFSET);
        uint64_t h2 = fnv1a(key, h1);
        for (size_t i = 0; i < k; i++) increment(get_hash(h1, h2, i));
        n_inserted++;
    }

    bool remove(string key) {
        if (!contains(key)) return false;
        uint64_t h1 = fnv1a(key, FNV_OFFSET);
        uint64_t h2 = fnv1a(key, h1);
        for (size_t i = 0; i < k; i++) decrement(get_hash(h1, h2, i));
        if (n_inserted > 0) n_inserted--;
        return true;
    }

    bool contains(string key) {
        uint64_t h1 = fnv1a(key, FNV_OFFSET);
        uint64_t h2 = fnv1a(key, h1);
        for (size_t i = 0; i < k; i++)
            if (get_counter(get_hash(h1, h2, i)) == 0) return false;
        return true;
    }

    uint8_t get_counter(size_t idx) {
        uint8_t byte = counters[idx / 2];
        return (idx % 2 == 0) ? (byte & 0x0F) : (byte >> 4);
    }

    void increment(size_t idx) {
        uint8_t val = get_counter(idx);
        if (val == MAX_COUNT) return;
        val++;
        if (idx % 2 == 0) counters[idx / 2] = (counters[idx / 2] & 0xF0) | val;
        else               counters[idx / 2] = (counters[idx / 2] & 0x0F) | (val << 4);
    }

    void decrement(size_t idx) {
        uint8_t val = get_counter(idx);
        if (val == 0 || val == MAX_COUNT) return;
        val--;
        if (idx % 2 == 0) counters[idx / 2] = (counters[idx / 2] & 0xF0) | val;
        else               counters[idx / 2] = (counters[idx / 2] & 0x0F) | (val << 4);
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

int passed = 0, failed = 0;

void check(bool condition, string test_name) {
    if (condition) {
        cout << "[PASS] " << test_name << endl;
        passed++;
    } else {
        cout << "[FAIL] " << test_name << endl;
        failed++;
    }
}

int main() {
    cout << "=== Bloom Filter Tests ===" << endl << endl;

    // BloomFilter tests
    {
        BloomFilter bf(1000, 0.01);
        bf.insert("apple"); bf.insert("banana"); bf.insert("cherry");

        check(bf.contains("apple"),   "BF: inserted element found");
        check(bf.contains("banana"),  "BF: inserted element found 2");
        check(!bf.contains("ghost"),  "BF: non-inserted element not found");
        check(bf.n_inserted == 3,     "BF: element count correct");

        // FPR test
        BloomFilter bf2(10000, 0.01);
        for (int i = 0; i < 10000; i++) bf2.insert("key_" + to_string(i));
        int fp = 0;
        for (int i = 10000; i < 20000; i++) if (bf2.contains("key_" + to_string(i))) fp++;
        double fpr = fp / 10000.0;
        check(fpr < 0.03, "BF: FPR within 3x target");
    }

    cout << endl;

    // CountingBloomFilter tests
    {
        CountingBloomFilter cbf(1000, 0.01);
        cbf.insert("hello");
        check(cbf.contains("hello"),        "CBF: inserted element found");
        check(!cbf.contains("world"),       "CBF: non-inserted element not found");

        cbf.remove("hello");
        check(!cbf.contains("hello"),       "CBF: element removed successfully");
        check(!cbf.remove("ghost"),         "CBF: remove returns false for missing key");

        // duplicate insert needs duplicate remove
        cbf.insert("dup"); cbf.insert("dup");
        cbf.remove("dup");
        check(cbf.contains("dup"),          "CBF: still present after one remove of duplicate");
        cbf.remove("dup");
        check(!cbf.contains("dup"),         "CBF: gone after second remove");

        // other keys unaffected
        cbf.insert("keep"); cbf.insert("bye");
        cbf.remove("bye");
        check(cbf.contains("keep"),         "CBF: other keys unaffected by remove");
    }

    cout << endl;
    cout << "Results: " << passed << " passed, " << failed << " failed" << endl;

    return failed > 0 ? 1 : 0;
}
