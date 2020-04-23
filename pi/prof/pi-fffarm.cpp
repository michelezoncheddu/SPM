/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
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
/* 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   April 2020
 *
 */
/*
 * Parallel schema:
 *
 *                | ---> Worker --> |
 *                |                 |
 *    Emitter --> | ---> Worker --> | --> Collector
 *                |                 | 
 *                | ---> Worker --> |
 *
 * The Emitter sends to each Worker a pair of values defining the 
 * portion of the iteration space (sub-range) assigned to the Worker. 
 * Each Worker computes the local sum of its sub-range, then it sends 
 * that value to the Collector which executes the final sum. 
 *
 */

#include <iostream>
#include <iomanip>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

using namespace ff;

// this is the message type used between the Emitter and the Workers
using pair_t = std::pair<int,int>;

struct Emitter:ff_monode_t<pair_t> {
    Emitter(const long num_steps, const int nw):num_steps(num_steps),nw(nw) {
        pairs = new pair_t[nw];
    }
    ~Emitter() { if (pairs) delete [] pairs; }
    
    pair_t* svc(pair_t*) {
        const long partsize{num_steps/nw};
        long remaining{num_steps%nw} , begin{0};
        
        for(int i=0;i<nw;++i) {
            long end   = begin + partsize + (remaining>0?1:0);	    
            pairs[i] = { begin, end-1 };
            ff_send_out_to(&pairs[i], i); // assigns the pair to Worker i
            --remaining;
            begin = end;
        }
        return EOS;
    }
    
    const long num_steps;
    const int  nw;
    pair_t* pairs = nullptr; // vector of pairs, one for each Worker
};
struct Worker:ff_node_t<pair_t, double> {
    Worker(const double step):step(step) {}
    double* svc(pair_t* in) {
        const int begin{in->first}, end{in->second};
        for(int i=begin; i<=end; ++i) {
            double x = (i+0.5)*step;
            sum = sum + 4.0/(1.0+x*x);
        }
        // it returns the partial sum, which will be sent to the Collector 
        return &sum; 
    }
    const double step;
    double sum=0.0;
};
struct Collector:ff_node_t<double> {
    double* svc(double* in) {
        sum += *in;
        return GO_ON;
    }
    double sum=0.0;  // it stores the final result
};

int main(int argc, char * argv[]) {
    if(argc != 3) {
        std::cout << "Usage is: " << argv[0] << " num_steps nw" << std::endl;
        return(-1);
    }
    long num_steps = atoi(argv[1]);
    int nw = atoi(argv[2]);
    
    ffTime(START_TIME);    
    Emitter E(num_steps, nw);
    Collector C;    
    double step = 1.0/(double) num_steps;
    ff_Farm<> farm( [&]() {
            std::vector<std::unique_ptr<ff_node> > W;
            for(int i=0;i<nw;++i)
                W.push_back(make_unique<Worker>(step));
            return W;
        } (),
        E,
        C);		       
    if (farm.run_and_wait_end()<0) {
        error("running farm");
        return -1;
    }
    double pi = step * C.sum;
    ffTime(STOP_TIME);
    std::cout << "Pi = " << std::setprecision(24) << pi << " (Computed in "
              << std::setprecision(4) << ffTime(GET_TIME) << " ms)"  << std::endl;
    return(0);
}
