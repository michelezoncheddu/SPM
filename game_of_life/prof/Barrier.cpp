#include <mutex>
#include <condition_variable>

class Barrier {

private:
  std::mutex lock;
  std::condition_variable cv;
  int nt;
  int now;

public:
  Barrier():nt(1),now(0) {}
  Barrier(int nt):nt(nt),now(0) {}

  // initially set number of threads to await
  void set_t(int n) {
    nt = n;
  }
  
  void BWait() {
    std::unique_lock<std::mutex> lk(lock);
    now++;
    if(now == nt) {
      cv.notify_all();
    } else {
      auto pred = [&]() { return (now==nt); };
      cv.wait(lk, pred);
    }
  }
}; 
