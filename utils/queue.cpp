#include <chrono>
#include <condition_variable>
#include <deque>
#include <iostream>
#include <mutex>

//
// needed a blocking queue
// here is a sample queue
//

template <typename T>
class syque {
   private:
    std::mutex d_mutex;
    std::condition_variable d_condition;
    std::deque<T> d_queue;

   public:
    syque(std::string s) {
        std::cout << "Created " << s << " queue " << std::endl;
    }
    syque() {}

    void push(T const& value) {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        d_queue.push_front(value);
        this->d_condition.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(this->d_mutex);
        this->d_condition.wait(lock, [this] { return !this->d_queue.empty(); });
        T rc(std::move(this->d_queue.back()));
        this->d_queue.pop_back();
        return rc;
    }
};

// lose some time
void active_delay(int msecs) {
    // read current time
    auto start = std::chrono::high_resolution_clock::now();
    auto end = false;
    while (!end) {
        auto elapsed = std::chrono::high_resolution_clock::now() - start;
        auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        if (msec > msecs)
            end = true;
    }
}
