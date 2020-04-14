#include <assert.h>

#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#include "BLcode.cpp"
#include "queue.cpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc < 6) {
        cout << "Usage is " << argv[0]
             << " rows cols generations seed nw" << endl;
        return -1;
    }

    const size_t rows = atol(argv[1]);
    const size_t cols = atol(argv[2]);
    const unsigned long generations = atol(argv[3]);
    const int seed = atoi(argv[4]);
    const int nw = atoi(argv[5]);

    // boards allocation
    vector<vector<int>> board(rows, vector(cols, 0));
    vector<vector<int>> future(rows, vector(cols, 0));

    // board initialization
    srand(seed);
    for (size_t i = 1; i < board.size() - 1; ++i)
        for (size_t j = 1; j < board[i].size() - 1; ++j)
            board[i][j] = rand() % 2;

    // business logic code components
    MySource s{board, 0, nw};
    MyWorker f{board, future, 0};
    MyDrain d{board, future, 0};

    // implementing flow control
    const pair<int, int> EOS{-1, -1};
    const int GOON = 1;

    // declare a queue for the input tasks
    syque<pair<int, int>> t_queue;
    // one for the results
    syque<int> r_queue;
    // and one for the feedback
    syque<int> f_queue;

    // kind of three concurrent activities
    // place input tasks into the input queue
    auto emit_task = [&](MySource s) {
        for (unsigned long i = 0; i < generations; ++i) {
            while (s.hasNext()) {
                auto t = s.next();
                t_queue.push(t);
            }
            // wait feedback
            int feedback = f_queue.pop();
            if (feedback == GOON)
                s.feedback_notify();
            else
                cout << "Impossible case" << endl;
        }
        // for as many workers to terminate, push EOS
        for (int i = 0; i < nw; i++)
            t_queue.push(EOS);
        return;
    };

    // process results
    auto proc_res = [&](MyDrain d, int nw) {
        while (true) {
            auto t = r_queue.pop();
            if (t == EOS.first && (--nw) == 0)
                break;
            if (d.process(t)) // send feedback
                f_queue.push(GOON);
        }
    };

    // processing tasks to results in parallel
    auto body = [&](MyWorker w, int wn) {
        while (true) {
            auto t = t_queue.pop();
            if (t == EOS) {
                r_queue.push(EOS.first);
                break; 
            }
            auto r = w.compute(t);
            r_queue.push(r);
        }
        return;
    };

    auto t0 = chrono::system_clock::now();
    auto e_thr = new thread(emit_task, s);
    auto c_thr = new thread(proc_res, d, nw);

    vector<thread*> tids(nw);
    for (int i = 0; i < nw; i++)
        tids[i] = new thread(body, f, i);

    // now await termination
    e_thr->join();
    for (int i = 0; i < nw; i++)
        tids[i]->join();
    c_thr->join();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(
                       chrono::system_clock::now() - t0)
                       .count();

    cout << "Parallel execution with " << nw << " threads took " << elapsed
         << " msecs" << endl;
        // << "speedup is " << ((float)tseq) / ((float)elapsed) << endl;
    return 0;
}
