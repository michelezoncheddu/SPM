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

// int or short int 
#define INT  short int

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
	          y[i][j-1]               + y[i][j+1] +
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
  vector< vector<INT> > y(n, vector<INT>(m));
  vector< vector<INT> > e(n, vector<INT>(m));

  const bool print = false; 
  const bool rnd = true; 
  if(rnd) {
    srand(1234);
    for(int i=1; i<n-1; i++)
      for(int j=1; j<m-1; j++)
	y[i][j] = rand() % 2;
  } else {
    init1(y,n,m,seed);
  }

#ifdef SEQ
  long seqt;
  {
    START(start);
    for(int i=0; i<iter; i++) {
      fill_e(y,e,n,m,1,n-1);
      update_y(y,e,n,m,1,n-1);
    }
    STOP(start,elapsed);
    cout << "Seq    time:        " << ((float) elapsed)/((float) iter)
	 << " usec per iteration" << std::endl;
    cout << "With nw = " << nw << " this means "
	 << (((float) elapsed)/((float) iter))/((float) nw)
	 << " usec per thread per iteration (ideal)" << std::endl;
    seqt = elapsed;
  }
#endif

  START(start);
#ifdef CLASSIC
  vector<Barrier> vbf(iter), vbu(iter);
#else
  vector<abar> vbf(iter), vbu(iter);
#endif
  // assign number of rows per nw threads
  for(auto &b : vbf)
    b.set_t(nw);
  for(auto &b : vbu)
    b.set_t(nw);
  
  auto thr =
    [&](int t) {
#ifdef TRACETIMES
      long us0 = 0L, us1=0L, us2=0L, us3=0L;
      long temp;
#endif
      auto delta = n / nw;
      auto li = (t==0 ? 1 : delta*t); // parte da 1 lasciando il bordo
      auto le  = (t == nw-1 ? n-1 : delta*(t+1));
      // cout << "Thread " << t << " computing rows from " << li << " to " << le-1 << endl; 
      for(int i=0; i<iter; i++) {
	// cout << "Thread " << t << " computing e " << endl;
	{
#ifdef TRACETIMES
	  utimer t0("b0",&temp);
#endif
	  fill_e(y,e,n,m,li,le);
	}
#ifdef TRACETIMES
	us0+=temp;
#endif
	// need to wait all other threads before updating
	// cout << "Thread " << t << " barrier 1 ...  " << endl;
	{
#ifdef TRACETIMES
	  utimer t1("b1",&temp);
#endif
	  vbf[i].BWait();
	}
#ifdef TRACETIMES
	us1+=temp;
#endif
	// cout << "Thread " << t << " computing y " << endl;
	{
#ifdef TRACETIMES
	  utimer t2("b2",&temp);
#endif
	  update_y(y,e,n,m,li,le);
	}
#ifdef TRACETIMES
	us2+=temp;
#endif
	// need to wait all other threads before starting new iteration
	// cout << "Thread " << t << " barrier 2 ... " << endl;
	{
#ifdef TRACETIMES
	  utimer t3("b3",&temp);
#endif
	  vbu[i].BWait();
	}
#ifdef TRACETIMES
	us3+=temp;
#endif

	if(print)
	  if(t == 0)
	    dumpw(y,n,m,1);
      }
#ifdef TRACETIMES
      {
	std::unique_lock<std::mutex> lk(mut);
	
	std::cout << "Avg neighbour " << ((float) us0)/((float) iter) << std::endl
		  << "Avg barrier 1 " << ((float) us1)/((float) iter) << std::endl
		  << "Avg newstate  " << ((float) us2)/((float) iter) << std::endl
		  << "Avg barrier 2 " << ((float) us3)/((float) iter) << std::endl;
      }
#endif
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
