#include <iostream>
#include <iomanip>
#include <omp.h>

using namespace std;

int main(int argc, char * argv[]) {
  if(argc != 2) {
     std::cout << "Usage is: " << argv[0] << " num_steps\n";
     return(-1);
  }

  long num_steps = atoi(argv[1]);
  double step;
  int i;
  double x, pi, sum = 0.0;

  auto start = omp_get_wtime();
  step = 1.0/(double) num_steps;

  
  for (i=0;i< num_steps; i++) {
    x = (i+0.5)*step;
    sum = sum + 4.0/(1.0+x*x);
  }
  pi = step * sum;
  auto elapsed = omp_get_wtime() - start;
  
  std::cout << "Pi = " << std::setprecision(24) << pi << " (Computed in "
	    << std::setprecision(4) << elapsed*1000.0 << " ms)"  << std::endl; 
  return(0);
}
