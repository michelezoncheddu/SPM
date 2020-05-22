#include <atomic>
#include <iostream>

template <typename T>
class buffer {
   private:
    T item;
    std::atomic<int> pbit;

   public:
    buffer() { pbit = 0; }

    void send(T i) {
        while (pbit != 0)
            ;
        item = i;
        pbit = 1;
    }

    T receive() {
        while (pbit == 0)
            ;
        T v = item;
        pbit = 0;
        return v;
    }

    bool empty() {
        if (pbit == 0)
            return true;
        else
            return false;
    }
};
