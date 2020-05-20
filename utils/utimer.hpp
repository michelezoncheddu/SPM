#ifndef UTIMER_H
#define UTIMER_H

class utimer {
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point stop;
    std::string message;
    using usecs = std::chrono::microseconds;
    using msecs = std::chrono::milliseconds;

   private:
    long* us_elapsed;

   public:
    utimer(const std::string m);

    utimer(const std::string m, long* us);

    ~utimer();
};

#endif
