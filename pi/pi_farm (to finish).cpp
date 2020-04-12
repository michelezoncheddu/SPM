#include <iostream>

#include <ff/ff.hpp>
#include <ff/farm.hpp>

using namespace ff;

using pair_t = std::pair<int, int>;

struct Emitter: ff_monode_t<pair_t> {
    Emitter(const long n_steps, const int n_workers)
        : n_steps(n_steps), n_workers(n_workers) {
        pairs = new pair_t[n_workers];
    }

    ~Emitter() {
        if (pairs)
            delete[] pairs;
    }

    pair_t* svc(pair_t *) {
        const long partsize {n_steps / n_workers};
        long remaining {n_steps % n_workers}, begin {0};
        for (int i = 0; i < n_workers; ++i) {
            long end = begin + partsize + ((remaining-- > 0) ? 1 : 0);
            pairs[i] = {begin, end - 1};
            ff_send_out_to(&pairs[i], i);
        }
        return EOS;
    }

    const long n_steps;
    const int n_workers;
    pair_t *pairs = nullptr;
};

int main(int argc, char const *argv[]) {
    return 0;
}
