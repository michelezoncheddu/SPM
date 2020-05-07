#include <algorithm>
#include <chrono>
#include <future>
#include <iostream>
#include <numeric>
#include <vector>

using TIN = std::vector<int>;
using TOUT = int;

template <typename Tin, typename Tout>
Tout dc(Tin input,
        bool (*basecase)(Tin),
        Tout (*solve)(Tin),
        std::vector<Tin> (*divide)(Tin),
        Tout (*conquer)(std::vector<Tout>)) {
    if (basecase(input)) {
        return solve(input);
    } else {
        auto subproblems = divide(input);
        std::vector<Tout> subresults;

        for (auto subproblem : subproblems)
            subresults.push_back(dc(subproblem, basecase, solve, divide, conquer));

        auto result = conquer(subresults);
        return result;
    }
}

template <typename Tin, typename Tout>
Tout dc_par(Tin input,
        bool (*basecase)(Tin),
        Tout (*solve)(Tin),
        std::vector<Tin> (*divide)(Tin),
        Tout (*conquer)(std::vector<Tout>),
        int level_workers,
        int max_workers) {
    if (basecase(input)) {
        return solve(input);
    } else {
        auto subproblems = divide(input);
        std::vector<Tout> subresults;

        if (level_workers * (int)subproblems.size() <= max_workers) { // parallel version
            std::vector<std::future<Tout>> futures;
            for (auto subproblem : subproblems) {
                futures.push_back(std::async(std::launch::async,
                                             dc_par<Tin, Tout>,
                                             subproblem,
                                             basecase,
                                             solve,
                                             divide,
                                             conquer,
                                             level_workers * subproblems.size(),
                                             max_workers));
            }
            for (auto &future : futures) {
                subresults.push_back(future.get());
            }
        } else { // sequential version
            for (auto subproblem : subproblems)
                subresults.push_back(dc(subproblem, basecase, solve, divide, conquer));
        }

        auto result = conquer(subresults);
        return result;
    }
}

bool basecase(TIN v) {
    return v.size() == 1;
}

TOUT solve(TIN v) {
    return v[0];
}

std::vector<TIN> divide(TIN v) {
    TIN v1(v.begin(), v.begin() + v.size() / 2),
        v2(v.begin() + v.size() / 2, v.end());
    return {v1, v2};
}

TOUT conquer(std::vector<TOUT> v) {
    return std::accumulate(v.begin(), v.end(), 0);
}

int main(int argc, char const *argv[]) {
    if (argc < 3) {
        std::cout << "Usage is: " << argv[0] << " n nw" << std::endl;
        return -1;
    }

    const int n  = std::stoi(argv[1]);
    const int nw = std::stoi(argv[2]);

    TIN v(n);
    std::iota(v.begin(), v.end(), 0);

    auto t0 = std::chrono::system_clock::now();
    auto res = dc_par(v, basecase, solve, divide, conquer, 1, nw);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - t0)
                       .count();

    std::cout << res << std::endl;
    std::cout << "Parallel execution took " << elapsed << " msecs" << std::endl;
    return 0;
}
