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
#include <iostream>
#include <iomanip>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;

int main(int argc, char * argv[]) {
    if(argc < 3 || argc >4) {
	std::cout << "Usage is: " << argv[0] << " num_steps nw [1|0]" << std::endl;
	std::cout << "The last argument (default 0) allows to disable the FF scheduler\n";
	return(-1);
    }
    long num_steps = atoi(argv[1]); --argc;
    int nw = atoi(argv[2]); --argc;
    bool schedoff = argc;

    ffTime(START_TIME);    
#if 0
    // this version uses the ParallelForReduce object
    ParallelForReduce<double> pf(nw);
    if (schedoff) pf.disableScheduler(); // disables the use of the scheduler

    double step = 1.0/(double) num_steps;
    double sum = 0.0;
    pf.parallel_reduce(sum, 0.0,
                       0, num_steps,
                       1,
                       0,
                       [&](const long i, double& sum) {
                           double x = (i+0.5)*step;  // x must be privatized
                           sum = sum + 4.0/(1.0+x*x);
                       },
                       [](double& s, const double& d) { s+=d;}
                       );
    double pi = step * sum;
#else
    /* This is the one-shot version (no object). Useful when there is just 
     * a single parallel loop.
     * This version should not be used if the parallel loop is called many 
     * times (e.g., within a sequential loop)  or if there are several loops 
     * that can be parallelized by using the  same ParallelFor* object. 
     * If this is the case, the version with the object instance is more 
     * efficient because the Worker threads are created once and then 
     * re-used many times.
     * On the contrary, the one-shot version has a lower setup overhead but 
     * Worker threads are destroyed at the end of the loop.
     */    
    double step = 1.0/(double) num_steps;
    double sum = 0.0;
    
    parallel_reduce(sum,0.0,
                    0, num_steps,
                    1,
                    0,
                    [&](const long i, double& sum) {
                        double x = (i+0.5)*step;  // x must be privatized
                        sum = sum + 4.0/(1.0+x*x);
                    },
                    [](double& s, const double& d) { s+=d;},
                    nw);
    double pi = step * sum;
#endif
    ffTime(STOP_TIME);
    std::cout << "Pi = " << std::setprecision(24) << pi << " (Computed in "
              << std::setprecision(4) << ffTime(GET_TIME) << " ms)"  << std::endl;
    
    return(0);
}

