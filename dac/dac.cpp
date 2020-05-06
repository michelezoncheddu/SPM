#include <algorithm>
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
    return std::accumulate(v.begin(), v.end(), 0);
}

int main(int argc, char const *argv[]) {
    TIN v = {1, 2, 3};
    std::cout << dc(v, basecase, solve, divide, conquer) << std::endl;
    return 0;
}
