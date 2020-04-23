#include <iostream>
#include <atomic>

class abar {
private:
  std::atomic<int> n;
  
public:
  abar() {}
  abar(int n) :n(n) {}

  void set_t(int in) {
    //std::cerr << "Set to " << in << std::endl; 
    n = in;
    return;
  }
  
  void dec() {
    n--;
  }

  void barrier() {
    while(n!=0);
    return;
  }

  void BWait(){
    //std::cerr << "Decreasing " << n << " to " << n-1 << std::endl;
    n--;
    //std::cerr << "Waiting ... " << std::endl;
    while(n!=0);
    //std::cerr << "Passed!" << std::endl;
    return;
  }

};
