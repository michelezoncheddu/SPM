#include <algorithm>
#include <chrono>
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
    return v[0] + v[1];
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        std::cout << "Usage is: " << argv[0] << " n" << std::endl;
        return -1;
    }

    const int n = std::stoi(argv[1]);

    TIN v(n);
    std::iota(v.begin(), v.end(), 0);

    auto t0 = std::chrono::system_clock::now();
    auto res = dc(v, basecase, solve, divide, conquer);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                       std::chrono::system_clock::now() - t0)
                       .count();

    std::cout << res << std::endl;
    std::cout << "Sequential execution took " << elapsed << " msecs" << std::endl;
    return 0;
}
