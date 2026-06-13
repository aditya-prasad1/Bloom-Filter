#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <iostream>

using namespace std;

uint64_t FNV_OFFSET = 14695981039346656037ULL;
uint64_t FNV_PRIME  = 1099511628211ULL;

class BloomFilter {
public:
    size_t m;           // number of bits
    size_t k;           // number of hash functions
    size_t n_inserted;
    vector<bool> bits;

    BloomFilter(size_t expected_elements, double false_positive_rate) {
        if (expected_elements == 0 || false_positive_rate <= 0 || false_positive_rate >= 1)
            throw invalid_argument("invalid parameters");

        // optimal bit count: m = -n * ln(p) / (ln2)^2
        m = (size_t)ceil(-1.0 * expected_elements * log(false_positive_rate) / (log(2) * log(2)));

        // optimal hash count: k = (m/n) * ln2
        k = (size_t)round((double)m / expected_elements * log(2));
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

    double estimated_fpr() {
        return pow(1.0 - exp(-1.0 * k * n_inserted / m), k);
    }

    uint64_t fnv1a(string key, uint64_t seed) {
        uint64_t hash = seed;
        for (char c : key) {
            hash ^= (unsigned char)c;
            hash *= FNV_PRIME;
        }
        return hash;
    }

    // Kirsch-Mitzenmacher: derive i-th hash from two base hashes
    // g_i(x) = h1(x) + i * h2(x)
    size_t get_hash(uint64_t h1, uint64_t h2, size_t i) {
        return (h1 + i * h2) % m;
    }
};
