/* ***************************************************************************
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as 
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  As a special exception, you may use this file as part of a free software
 *  library without restriction.  Specifically, if other files instantiate
 *  templates or use macros or inline functions from this file, or you compile
 *  this file and link it with other files to produce an executable, this
 *  file does not by itself cause the resulting executable to be covered by
 *  the GNU General Public License.  This exception does not however
 *  invalidate any other reasons why the executable file might be covered by
 *  the GNU General Public License.
 *
 ****************************************************************************
 */

#include <iostream>

#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;

using ull = unsigned long long;

static bool is_prime(ull n) {
    if (n <= 3) return n > 1; // 1 is not prime!
    
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (ull i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) 
            return false;
    }
    return true;
}

int main(int argc, char *argv[]) {    
    if (argc < 4) {
        std::cout << "Usage is: " << argv[0] << " start end nw [print=off|on]" << std::endl;
        return -1;
    }

    const ull start = std::stoll(argv[1]);
    const ull end   = std::stoll(argv[2]);
    const auto nw = atoi(argv[3]);
    bool print_primes = false;
    if (argc >= 5) 
        print_primes = (std::string(argv[4]) == "on");
    
    ffTime(START_TIME);
    const auto cache_size = cache_line_size();
    auto nprimes = (size_t)((end - start) / log(start));
    auto nprimes_w = nprimes / nw;
    const auto padding = cache_size - (nprimes_w % cache_size); // to avoid false sharing
    nprimes += (padding * nw);
    nprimes_w += padding;
    std::vector<ull> primes(nprimes);
    
    ParallelFor pf(nw);
    pf.parallel_for_idx(start, end + 1, // start, stop indexes
                        1,              // step size
                        0,              // chunk size (0=static, >0=dynamic)
                        [&](const ull begin, const ull end, const int thid) {
                            ull i = nprimes_w * thid;
                            for (ull number = begin; number < end; ++number)
                                if (is_prime(number))
                                    primes[i++] = number;
                        });

    primes.erase(
        std::remove_if(primes.begin(), primes.end(), [](auto v) { return v == 0; }),
        primes.end());

    std::cout << primes.size() << " primes found" << std::endl;
    ffTime(STOP_TIME);

    if (print_primes)
        for (auto x : primes)
            std::cout << x << " ";

    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)" << std::endl;
    return 0;
}
