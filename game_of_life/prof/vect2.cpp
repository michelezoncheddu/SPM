//
// compile with
// g++ -g -ftree-vectorize -fopt-info-vec -O3 vec.cpp -o vec -fopenmp 2>&1 > /dev/null | grep vec.cpp
//
//
// compile with -DCLASSIC for condition variable based barrier
// by default we use the atomic based active wait barriers
//
// compile -DSEQ to see sequential run estimate before parallel run
//
// compile -DTRACETIMES to see all partial times
//

#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <mutex>
#include <algorithm>
#include <utility>

// int or short int 
#define INT short int

// uncomment to see all phase times printed
//#define TRACETIMES

#ifdef TRACETIMES
std::mutex mut;
#endif

using namespace std;

#ifdef CLASSIC
#include "Barrier.cpp"
#else
#include "abar.cpp"
#endif

#ifdef TRACETIMES
#include "utimer.cpp"
#endif

void dumpw(vector< vector<INT>> a, int rows, int cols, bool print) {
  if(print) {
    for(int i=0; i<rows; i++) {
      for(int j=0; j<cols; j++)
	cout << (a[i][j]==0 ? " " : "O");
      cout << endl;
    }
    cout << "------------------------------------------------------------------"
	 << endl;
    this_thread::sleep_for(chrono::milliseconds(25));
  }
  return;
}

void dumpe(vector< vector<INT>> a, int rows, int cols, bool print) {
  if(print) {
    for(int i=0; i<rows; i++) {
      for(int j=0; j<cols; j++)
	cout << a[i][j] ;
      cout << endl;
    }
    cout << "------------------------------------------------------------------"
	 << endl;
    this_thread::sleep_for(chrono::seconds(1));
  }
  return;
}

void init1(vector<vector<INT>> &y, const int n, const int m, const int seed) {
  y[3][1]=1; y[3][2]=1; y[3][3]=1; y[2][3]=1; y[1][1]=1;
  return;
}

void fill_e(vector<vector<INT>> y, vector<vector<INT>> &e, const int n, const int m, const int from, const int to) {
  // compute just a portion of the matrix from line from to line to-1
  for(int i=from; i<to; i++)
#pragma GCC ivdep // without : versioning
    for(int j=1; j<m-1; j++)
      {
	// compute neighbourhood
	e[i][j] = y[i-1][j-1] + y[i-1][j] + y[i-1][j+1] +
	  y[i][j-1] + y[i][j+1] +
	  y[i+1][j-1] + y[i+1][j] + y[i+1][j+1];
      }
  return;
}

void update_y(vector<vector<INT>> &y, vector<vector<INT>> e, const int n, const int m, const int from, const int to) {
  // update just a portion of the matrix from line from to line to-1
  for(int i=from; i<to; i++)
#pragma GCC ivdep  // without : versioning
    for(int j=1; j<m-1; j++)
      y[i][j] = (e[i][j]==3) || (e[i][j]==2 && y[i][j]==1);
  // 3 neighb (stay alive or new indidual) || 2 neighb and alive stay alive
  return;
}

#define START(timename) auto timename = std::chrono::system_clock::now();
#define STOP(timename,elapsed)  auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - timename).count();

int main(int argc, char * argv[]) {

  if(argc==1) {
    cout << "Usage is: " << argv[0] << " n m iter seed nw " << endl;
    return(-1);
  }
  int n = atoi(argv[1]);
  int m = atoi(argv[2]);
  int iter = atoi(argv[3]);
  int seed = atoi(argv[4]);
  int nw = atoi(argv[5]);

  std::cout << "Using " << sizeof(INT) << " byte(s) ints. ";
#ifdef CLASSIC
  std::cout << "Barriers implemented using condition variables. ";
#else
  std::cout << "Barriers implemented using atomic and active wait. ";
#endif
#ifdef TRACETIMES
  std::cout << "Tracing times " << std::endl;
#else
  std::cout << "NOT tracing times " << std::endl;
#endif
  
  vector<INT> x(n);
  vector< vector<INT> > source(n, vector<INT>(m));
  vector< vector<INT> > dest  (n, vector<INT>(m));

  const bool print = false; 
  const bool rnd = true; 
  if(rnd) {
    srand(1234);
    for(int i=1; i<n-1; i++)
      for(int j=1; j<m-1; j++)
	source[i][j] = rand() % 2;
  } else {
    init1(source,n,m,seed);
  }

  long seqt;
#ifdef SEQ
  {
    auto psource = &source;
    auto pdest = &dest;
    START(start);
    for(int its=0; its<iter; its++) {
      for(int i=1; i<n-1; i++) {
	for(int j=1; j<m-1; j++) {
	  auto nes =
	    (*psource)[i-1][j-1] + (*psource)[i-1][j] + (*psource)[i-1][j+1] +
	    (*psource)[i][j-1]                        + (*psource)[i][j+1] +
	    (*psource)[i+1][j-1] + (*psource)[i+1][j] + (*psource)[i+1][j+1];
	  auto p1 = (nes == 3);
	  auto p2 = (nes == 2);
	  auto p3 = (*psource)[i][j]==1;
	  auto p4 = p2 && p3;
	  auto p5 = p1 || p4;
	  (*pdest)[i][j] = p5;
	}
      }
      std::swap(psource,pdest);
    }
    STOP(start,elapsedseq);
    cout << "Seq    time:        " << ((float) elapsedseq)/((float) iter)
	 << " usec per iteration" << std::endl;
    cout << "With nw = " << nw << " this means "
	 << (((float) elapsedseq)/((float) iter))/((float) nw)
	 << " usec per thread per iteration (ideal)" << std::endl;
    seqt = elapsedseq;
  }
#endif

  START(start);
#ifdef CLASSIC
  vector<Barrier> vbf(iter);
#else
  vector<abar> vbf(iter);
#endif
  // assign number of rows per nw threads
  for(auto &b : vbf)
    b.set_t(nw);
  
  auto thr =
    [&](int t) {
#ifdef TRACETIMES
      long us0 = 0L, us1=0L, us2=0L, us3=0L;
      long temp;
#endif
      // compute chunk boundaries
      auto delta = n / nw;
      auto li = (t==0 ? 1 : delta*t); // parte da 1 lasciando il bordo
      auto le  = (t == nw-1 ? n-1 : delta*(t+1));
      vector<vector<INT>> * psource = &source, * pdest = &dest;
      // cout << "Thread " << t << " computing rows from " << li << " to " << le-1 << endl;
      // then execute all iterations
      for(int i=0; i<iter; i++) {
	// cout << "Thread " << t << " computing e " << endl;
	// operate on source and compute dest vector
	for(int i=li; i<le; i++) { // for all assigned lines
#pragma GCC ivdep // without : versioning	  
	  for(int j=0; j<m; j++) { // for all columns
	    auto nes =
	      (*psource)[i-1][j-1] + (*psource)[i-1][j] + (*psource)[i-1][j+1] +
	      (*psource)[i][j-1]                        + (*psource)[i][j+1] +
	      (*psource)[i+1][j-1] + (*psource)[i+1][j] + (*psource)[i+1][j+1];
	    // this vectorizes, begin computed piecewise
	    auto p1 = (nes == 3);
	    auto p2 = (nes == 2);
	    auto p3 = (*psource)[i][j]==1;
	    auto p4 = p2 && p3;
	    auto p5 = p1 || p4;
	    (*pdest)[i][j] = p5;
	    // the following does not vectorize due to "control flow in the loop"
	    // (*pdest)[i][j] = (nes == 3) || ((nes == 2) && ((*psource)[i][j]==1));
		    
	  }
	}
	// at iteration end, barrier
	vbf[i].BWait();
	// then swap source and dest
	std::swap(psource,pdest);
      }
	
    };
      
  vector<thread *> tids(nw);
  
  for(int t=0; t<nw; t++) {
    // cout << "Creating thread " << t << endl;
    tids[t] = new thread(thr, t);
  }

  for(int t=0; t<nw; t++) {
    // cout << "Waiting thread " << t << endl;
    tids[t]->join();
  }
  
  STOP(start,elapsed);
  cout << "Total time:        " << elapsed << " usec " 
       << "\twith\t" << nw << " threads\t"; 
  cout << "Average iteration: " << ((float) elapsed) / ((float) iter) <<endl;
#ifdef SEQ
  cout << "Achieved speedup is " << ((float) seqt)/((float) elapsed)
       << std::endl;
#endif
  return(0);
}
