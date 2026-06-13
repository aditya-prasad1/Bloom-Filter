#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <iostream>

using namespace std;

uint64_t FNV_OFFSET = 14695981039346656037ULL;
uint64_t FNV_PRIME  = 1099511628211ULL;
uint8_t  MAX_COUNT  = 15;  // max value for a 4-bit counter

class CountingBloomFilter {
public:
    size_t m;           // number of counters
    size_t k;           // number of hash functions
    size_t n_inserted;
    vector<uint8_t> counters;  // two 4-bit counters packed per byte

    CountingBloomFilter(size_t expected_elements, double false_positive_rate) {
        if (expected_elements == 0 || false_positive_rate <= 0 || false_positive_rate >= 1)
            throw invalid_argument("invalid parameters");

        m = (size_t)ceil(-1.0 * expected_elements * log(false_positive_rate) / (log(2) * log(2)));
        k = (size_t)round((double)m / expected_elements * log(2));
        if (k < 1) k = 1;

        n_inserted = 0;
        counters.assign((m + 1) / 2, 0);
    }

    void insert(string key) {
        uint64_t h1 = fnv1a(key, FNV_OFFSET);
        uint64_t h2 = fnv1a(key, h1);

        for (size_t i = 0; i < k; i++)
            increment(get_hash(h1, h2, i));

        n_inserted++;
    }

    bool remove(string key) {
        if (!contains(key)) return false;

        uint64_t h1 = fnv1a(key, FNV_OFFSET);
        uint64_t h2 = fnv1a(key, h1);

        for (size_t i = 0; i < k; i++)
            decrement(get_hash(h1, h2, i));

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

    double estimated_fpr() {
        return pow(1.0 - exp(-1.0 * k * n_inserted / m), k);
    }

    // even index -> lower nibble, odd index -> upper nibble
    uint8_t get_counter(size_t idx) {
        uint8_t byte = counters[idx / 2];
        return (idx % 2 == 0) ? (byte & 0x0F) : (byte >> 4);
    }

    void increment(size_t idx) {
        uint8_t val = get_counter(idx);
        if (val == MAX_COUNT) return;  // saturate, don't overflow
        val++;
        if (idx % 2 == 0)
            counters[idx / 2] = (counters[idx / 2] & 0xF0) | val;
        else
            counters[idx / 2] = (counters[idx / 2] & 0x0F) | (val << 4);
    }

    void decrement(size_t idx) {
        uint8_t val = get_counter(idx);
        if (val == 0 || val == MAX_COUNT) return;
        val--;
        if (idx % 2 == 0)
            counters[idx / 2] = (counters[idx / 2] & 0xF0) | val;
        else
            counters[idx / 2] = (counters[idx / 2] & 0x0F) | (val << 4);
    }

    uint64_t fnv1a(string key, uint64_t seed) {
        uint64_t hash = seed;
        for (char c : key) {
            hash ^= (unsigned char)c;
            hash *= FNV_PRIME;
        }
        return hash;
    }

    size_t get_hash(uint64_t h1, uint64_t h2, size_t i) {
        return (h1 + i * h2) % m;
    }
};
