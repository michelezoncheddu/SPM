#include <iomanip>
#include <iostream>

#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

int main(int argc, char const *argv[]) {
    if (argc != 3) {
        std::cout << "Usage is: " << argv[0] << " n_steps n_workers " << std::endl;
        return -1;
    }

    const long n_steps = atoi(argv[1]);
    const int n_workers = atoi(argv[2]);

    const double step = 1.0 / n_steps;
    double sum = 0.0;

    ff::ParallelForReduce<double> pfr(n_workers);
    
    ff::ffTime(ff::START_TIME);
    pfr.parallel_reduce(sum, 0.0,
                        0, n_steps,
                        [&](const long i, double &sum) {
                            const double x = (i + 0.5) * step;
                            sum += 4.0 / (1.0 + x * x);
                        },
                        [](double &total, const double partial) {
                            total += partial;
                        });
    ff::ffTime(ff::STOP_TIME);
    const double time = ff::ffTime(ff::GET_TIME);
    
    const double pi = sum * step;

    std::cout << "pi = " << std::setprecision(14) << pi << std::endl;    
    std::cout << "time: " << std::setprecision(4) << time << std::endl;

    return 0;
}
