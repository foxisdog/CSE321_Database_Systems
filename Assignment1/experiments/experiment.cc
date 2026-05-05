#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include "../src/storage.cc"
#include "../src/btree.cc"
#include "../src/bplus_tree.cc"
#include "../src/bstar_tree.cc"

using namespace std;
using namespace std::chrono;

// ── Free all tree nodes (tree classes have no destructor) ─────────────────
template<typename Node>
void free_nodes(Node* n) {
    if (!n) return;
    if (!n->is_leaf)
        for (auto c : n->children) free_nodes(c);
    delete n;
}

// ── Tree statistics ───────────────────────────────────────────────────────
struct Stats {
    long   nodes     = 0;
    long   keys      = 0;
    int    height    = 0;
    size_t mem_bytes = 0;
};

template<typename Node>
void collect(Node* n, Stats& s, int depth = 1) {
    if (!n) return;
    s.nodes++;
    s.keys   += n->keys.size();
    s.height  = max(s.height, depth);
    // struct size (incl. vector control blocks) + actual heap data per vector
    s.mem_bytes += sizeof(*n);
    s.mem_bytes += n->keys.size()     * sizeof(int);
    s.mem_bytes += n->rids.size()     * sizeof(int);
    s.mem_bytes += n->children.size() * sizeof(void*);
    if (!n->is_leaf)
        for (auto c : n->children) collect(c, s, depth + 1);
}

// ── Point search ─────────────────────────────────────────────────────────
template<typename Tree>
double measure_search_us(Tree& t, const vector<int>& qs) {
    volatile long long checksum = 0;
    auto a = high_resolution_clock::now();
    for (int k : qs) {
        checksum += t.search(k);
    }
    auto b = high_resolution_clock::now();
    (void)checksum;
    return duration_cast<nanoseconds>(b - a).count() / (double)qs.size() / 1000.0;
}

// ── Range query: B-tree / B*-tree (per-key search, median of N_REPS runs) ─
static constexpr int N_REPS = 7;

template<typename Tree>
double measure_range_ms(Tree& t, Storage& st) {
    vector<double> samples;
    samples.reserve(N_REPS);
    for (int r = 0; r < N_REPS; r++) {
        auto a = high_resolution_clock::now();
        volatile double g = 0;
        for (int id = 202000000; id <= 202100000; id++) {
            int rid = t.search(id);
            if (rid != -1) {
                const auto& s = st.get(rid);
                if (s.gender == "Male") g += s.gpa + s.height;
            }
        }
        (void)g;
        samples.push_back(
            duration_cast<microseconds>(high_resolution_clock::now() - a).count() / 1000.0);
    }
    sort(samples.begin(), samples.end());
    return samples[0];  // minimum (eliminates OS scheduling outliers)
}

// ── Range query: B+tree (linked-leaf scan, median of N_REPS runs) ─────────
double measure_range_bplus_ms(bplus_tree& t, Storage& st) {
    vector<double> samples;
    samples.reserve(N_REPS);
    for (int r = 0; r < N_REPS; r++) {
        auto a = high_resolution_clock::now();
        volatile double g = 0;
        for (auto& [k, rid] : t.range_search(202000000, 202100000)) {
            const auto& s = st.get(rid);
            if (s.gender == "Male") g += s.gpa + s.height;
        }
        (void)g;
        samples.push_back(
            duration_cast<microseconds>(high_resolution_clock::now() - a).count() / 1000.0);
    }
    sort(samples.begin(), samples.end());
    return samples[0];  // minimum (eliminates OS scheduling outliers)
}

// ── All experiments for one (Tree, order) ────────────────────────────────
struct Result {
    double insert_ms;
    long   splits;
    long   nodes;
    double util_pct;
    double memory_kb;
    double search_us;
    double range_ms;
    vector<double> del_ms; // cumulative time at r05, r10, ..., r50
};

// IsBplus = true  → linked-leaf range scan
// IsBplus = false → per-key search for range
template<typename Tree, bool IsBplus = false>
Result run_one(Storage& storage, int order,
               const vector<int>& sq,    // 10,000 search query keys
               const vector<int>& dk) {  // 50,000 deletion keys (10 batches × 5%)
    Result r;
    const int N     = storage.size();
    const int batch = N / 20;            // 5% = 5,000 keys per batch

    Tree tree(order);

    // 1. Insert: measure time
    {
        auto t0 = high_resolution_clock::now();
        for (int i = 0; i < N; i++) tree.insert(storage.get(i).id, i);
        r.insert_ms = duration_cast<microseconds>(
            high_resolution_clock::now() - t0).count() / 1000.0;
    }
    r.splits = tree.get_split_count();

    // 2. Stats + memory (measured on the full tree after insertion)
    {
        Stats s;
        collect(tree.get_root(), s);
        r.nodes     = s.nodes;
        r.util_pct  = s.nodes > 0
            ? (double)s.keys / (s.nodes * (order - 1)) * 100.0
            : 0.0;
        r.memory_kb = s.mem_bytes / 1024.0;
    }

    // 3. Point search
    r.search_us = measure_search_us(tree, sq);

    // 4. Range query
    if constexpr (IsBplus)
        r.range_ms = measure_range_bplus_ms(tree, storage);
    else
        r.range_ms = measure_range_ms(tree, storage);

    // 5. Deletion sweep: 10 batches × 5,000 keys, record cumulative time
    {
        double cum = 0.0;
        r.del_ms.reserve(10);
        for (int b = 0; b < 10; b++) {
            auto d0 = high_resolution_clock::now();
            for (int i = b * batch; i < (b + 1) * batch; i++)
                tree.delete_key(dk[i]);
            cum += duration_cast<microseconds>(
                high_resolution_clock::now() - d0).count() / 1000.0;
            r.del_ms.push_back(cum);
        }
    }

    // Free remaining nodes explicitly (tree classes have no destructor)
    free_nodes(tree.get_root());

    return r;
}

// ── main ──────────────────────────────────────────────────────────────────
int main() {
    Storage storage("../data/student.csv");
    const int N = storage.size();

    // All keys
    vector<int> all_keys;
    all_keys.reserve(N);
    for (int i = 0; i < N; i++) all_keys.push_back(storage.get(i).id);

    // Search queries: 10,000 random keys, fixed seed
    mt19937 rng_s(42);
    vector<int> sq = all_keys;
    shuffle(sq.begin(), sq.end(), rng_s);
    sq.resize(10000);

    // Deletion keys: 50% of records after fixed-seed shuffle (10 batches × 5%)
    mt19937 rng_d(123);
    vector<int> dk = all_keys;
    shuffle(dk.begin(), dk.end(), rng_d);
    dk.resize(N / 2);

    // ── Output summary CSV ───────────────────────────────────────────────
    ofstream f_csv("result.csv");

    f_csv << "tree,order,insert_ms,splits,nodes,util_pct,memory_kb,search_us,range_ms,"
             "del_r05,del_r10,del_r15,del_r20,del_r25,del_r30,del_r35,del_r40,del_r45,del_r50\n";

    // Helpers
    auto wcsv = [&](const string& nm, int d, const Result& r) {
        f_csv << nm << ',' << d << ','
              << r.insert_ms  << ',' << r.splits   << ',' << r.nodes   << ','
              << r.util_pct   << ',' << r.memory_kb << ','
              << r.search_us  << ',' << r.range_ms;
        for (double t : r.del_ms) f_csv << ',' << t;
        f_csv << '\n';
    };

    // ── Order sweep: d = 3 to 100 ────────────────────────────────────────
    for (int d = 3; d <= 100; d++) {
        auto rb = run_one<btree>(storage, d, sq, dk);
        auto rp = run_one<bplus_tree, true>(storage, d, sq, dk);
        auto rs = run_one<bstar_tree>(storage, d, sq, dk);

        wcsv("btree", d, rb);
        wcsv("bplus", d, rp);
        wcsv("bstar", d, rs);
    }
    return 0;
}
