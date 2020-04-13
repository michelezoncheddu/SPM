// stream source
template <typename OUT>
class Source {
   public:
    virtual OUT next() = 0;
    virtual bool hasNext() = 0;
};

// business logic to compute a task
template <typename IN, typename OUT>
class Worker {
   public:
    virtual OUT compute(IN x) = 0;
};

// processing the results: accumulate the stream contents
template <typename IN>
class Drain {
   public:
    virtual void process(IN x) = 0;
};
