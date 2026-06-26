Future Changes : 
1) Implementation in Go
2) Concurrency support  

# Bloom Filter & Spell Checker

A C++ implementation of a Bloom Filter and Counting Bloom Filter, applied to build a dictionary-backed spell checker.

## What is a Bloom Filter?

A Bloom Filter is a probabilistic data structure that answers one question: **is this element in the set?**

It can say:
- **"Definitely not in the set"** — always correct
- **"Probably in the set"** — occasionally wrong (false positive)

It never produces false negatives. The trade-off is a small, tunable false positive rate in exchange for using far less memory than storing the actual elements.

### Where is it used?

- **Databases** — avoid disk lookups for keys that don't exist (used in LevelDB, Cassandra, HBase)
- **Browsers** — Chrome used a Bloom Filter to check malicious URLs before querying a server
- **Caches** — avoid caching one-hit-wonder items
- **Spell checkers** — quick membership check against a dictionary

## How it works

Instead of storing elements, a Bloom Filter maintains a **bit array** of size `m`. To insert an element, it is hashed `k` times and the corresponding `k` bits are set to 1. To check membership, the same `k` positions are checked — if any bit is 0, the element was never inserted.

```
insert("hello"):
  h1("hello") = 3  → set bit 3
  h2("hello") = 7  → set bit 7
  h3("hello") = 1  → set bit 1

contains("hello"):
  check bits 3, 7, 1 → all 1 → probably in set

contains("world"):
  check bits ... → bit 5 is 0 → definitely not in set
```

The false positive rate depends on `m` (bit array size), `k` (hash functions), and `n` (elements inserted). Given `n` and a target FPR `p`, the optimal values are:

```
m = -n * ln(p) / (ln 2)^2
k = (m / n) * ln 2
```

## Counting Bloom Filter

A standard Bloom Filter doesn't support deletions — once a bit is set, you can't unset it without affecting other elements.

The Counting Bloom Filter replaces each bit with a small counter (4 bits in this implementation). Inserting increments the counters, deleting decrements them. An element is present if all its counters are > 0.

The trade-off: ~4x more memory than a standard Bloom Filter.

## Hashing

Both implementations use **FNV-1a** (64-bit) with the **Kirsch-Mitzenmacher trick** — instead of computing `k` independent hashes, only two base hashes are computed and the rest are derived as:

```
g_i(x) = h1(x) + i * h2(x)   (mod m)
```

This gives effectively independent hash functions with just two hash computations per key.

## Spell Checker

The spell checker loads a dictionary into a Bloom Filter and checks each word in an input file or sentence against it.

```bash
# compile
g++ -std=c++17 -O2 src/spell_checker.cpp -o spell_checker

# run
./spell_checker dictionary.txt input.txt
```

**Example output:**
```
Dictionary loaded: 9981 words 

Line 2: "gret" not found in dictionary
Line 3: "mountan" not found in dictionary
Line 4: "sentance" not found in dictionary

Total words checked: 47
Possibly misspelled: 3
Note: false positives possible at ~1% rate
```

The dictionary file should have one word per line. A sample `dictionary.txt` with 10,000 common English words is included.

## Running Tests and Spell Checker

```bash
g++ -std=c++17 -O2 tests/test_bloom.cpp -o tests
./tests
```

```bash
g++ -std=c++17 -O2 src/spell_checker.cpp -o spell_checker
./spell_checker dictionary.txt input.txt
```

## Project Structure

```
bloom-filter/
├── src/
│   ├── bloom_filter.cpp          # Bloom Filter
│   ├── counting_bloom_filter.cpp # Counting Bloom Filter (supports deletion)
│   └── spell_checker.cpp         # Spell checker built on top of BloomFilter
├── tests/
│   └── test_bloom.cpp            # Unit tests
└── dictionary.txt                # 10,000 common English words
```
