#include <algorithm>
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <numeric>
#include <thread>
#include <vector>

using TIN = std::vector<int>;
using TOUT = int;

int active; // number of active threads
int nw;
std::mutex mutex;

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
        Tout (*conquer)(std::vector<Tout>)) {
    if (basecase(input)) {
        mutex.lock();
        active--;
        mutex.unlock();
        return solve(input);
    } else {
        auto subproblems = divide(input);
        std::vector<Tout> subresults;

        mutex.lock();
        if (active + (int)subproblems.size() - 1 <= nw) { // parallel version
            active += subproblems.size() - 1; // minus myself
            mutex.unlock();

            std::vector<std::future<Tout>> futures;
            for (auto subproblem : subproblems) {
                futures.push_back(std::async(std::launch::async,
                                             dc_par<Tin, Tout>,
                                             subproblem,
                                             basecase,
                                             solve,
                                             divide,
                                             conquer));
            }
            for (auto &future : futures)
                subresults.push_back(future.get());
            mutex.lock();
            active++; // myself
            mutex.unlock();
        } else { // sequential version
            mutex.unlock();
            for (auto subproblem : subproblems)
                subresults.push_back(dc(subproblem, basecase, solve, divide, conquer));
        }
        mutex.lock();
        active--;
        mutex.unlock();
        return conquer(subresults);
    }
}

auto basecase(TIN v) {
    return v.size() == 1;
}

auto solve(TIN v) {
    return v[0];
}

std::vector<TIN> divide(TIN v) {
    TIN v1(v.begin(), v.begin() + v.size() / 2),
        v2(v.begin() + v.size() / 2, v.end());
    return {v1, v2};
}

auto conquer(std::vector<TOUT> v) {
    return v[0] + v[1];
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cout << "Usage is: " << argv[0] << " n nw (optional)" << std::endl;
        return -1;
    }

    const int n = std::stoi(argv[1]);
    nw = argc > 2 ? std::stoi(argv[2]) : std::thread::hardware_concurrency();

    TIN v(n);
    std::iota(v.begin(), v.end(), 0);

    active = 1;
    auto t0 = std::chrono::system_clock::now();
    auto res = dc_par(v, basecase, solve, divide, conquer);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - t0)
                       .count();

    std::cout << res << std::endl;
    std::cout << "Parallel execution took " << elapsed << " msecs" << std::endl;
    return 0;
}
