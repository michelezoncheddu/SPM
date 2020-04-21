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

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

using namespace ff;

using ull = unsigned long long;
using pair_t = std::pair<ull, ull>;

static bool is_prime(ull n) {
    if (n <= 3)  return n > 1; // 1 is not prime!
    
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (ull i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) 
            return false;
    }
    return true;
}

struct Emitter:ff_monode_t<std::vector<ull>, pair_t> {
    Emitter(ull n1, ull n2, int nw) : n1(n1), n2(n2), nw(nw) {
        pairs = new pair_t[nw];
        primes.reserve((size_t)(n2 - n1) / log(n1));
    }
    ~Emitter() { if (pairs) delete[] pairs; }

    pair_t* svc(std::vector<ull> *v) {
        if (v == nullptr) {
            const ull range = n2 - n1 + 1;
            const ull partsize{range / (nw + 1)};
            ull remaining{range % (nw + 1)}, begin{n1 + partsize};
            
            for (int i = 0; i < nw; ++i) {
                long end = begin + partsize + (remaining > 0 ? 1 : 0);	    
                pairs[i] = {begin, end - 1};
                ff_send_out_to(&pairs[i], i);
                --remaining;
                begin = end;
            }
            broadcast_task(EOS);
            for (ull number = n1; number < n1 + partsize; ++number)
                if (is_prime(number))
                    primes.push_back(number);
            return GO_ON;
        }
        // compute v from feedback

        return GO_ON;
    }
    
    const ull n1, n2;
    const int nw;
    pair_t *pairs = nullptr;
    std::vector<ull> primes;
};

struct Worker:ff_node_t<pair_t, std::vector<ull>> {
    Worker(int nprimes) {
        primes = std::vector<ull>(nprimes);
    }

    std::vector<ull>* svc(pair_t* in) {
        const ull begin{in->first}, end{in->second};
        for (ull number = begin; number <= end; ++number)
            if (is_prime(number))
                primes.push_back(number);
        primes.push_back(0);
        return &primes;
    }
    std::vector<ull> primes;
};

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Usage is: " << argv[0] << " start end nw" << std::endl;
        return -1;
    }

    ull start = std::stoll(argv[1]);
    ull end   = std::stoll(argv[2]);
    int nw = atoi(argv[3]);
    
    ffTime(START_TIME);
    size_t nprimes = (size_t)((end - start) / log(start)) / nw;

    Emitter E(start, end, nw - 1);
    ff_Farm<> farm([&]() {
            std::vector<std::unique_ptr<ff_node>> W;
            for (int i = 0; i < nw - 1; ++i)
                W.push_back(make_unique<Worker>(nprimes));
            return W;
        } (),
        E);
    farm.remove_collector();
    farm.wrap_around();
    if (farm.run_and_wait_end() < 0) {
        error("running farm");
        return -1;
    }
    ffTime(STOP_TIME);
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)" << std::endl;
    return 0;
}
