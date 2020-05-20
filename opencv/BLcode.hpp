// stream source
template <typename OUT>
class Source {
   public:
    virtual OUT next() = 0;
    virtual bool hasNext() = 0;
};

// business logic to compute a task
template <typename IN>
class Worker {
   public:
    virtual void compute(IN x) = 0;
};
