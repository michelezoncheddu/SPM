#include <chrono>
#include <iostream>
#include <mutex>
#include <queue>
#include <thread>

#include "BLcode.cpp"
#include "queue.cpp"

using namespace std;

int main(int argc, char* argv[]) {
    if (argc == 1) {
        cout << "Usage is " << argv[0]
             << " start stop msecS msecW msecD nw [trace]" << endl;
        return (-1);
    }

    int start = atoi(argv[1]);
    int stop = atoi(argv[2]);
    int m1 = atoi(argv[3]);
    int m2 = atoi(argv[4]);
    int m3 = atoi(argv[5]);
    int nw = atoi(argv[6]);
    bool debug = (argc != 7);

    // TODO: from command line
    const size_t rows{10}, cols{10};
    const unsigned long generations{1};

    // boards allocation
    vector<vector<int>> board(rows, vector(cols, 0));
    vector<vector<int>> future(rows, vector(cols, 0));

    // board initialization
    srand(time(nullptr));
    for (size_t i = 1; i < board.size() - 1; ++i)
        for (size_t j = 1; j < board[i].size() - 1; ++j)
            board[i][j] = rand() % 2;

    // business logic code components
    MySource s{board, 0};
    MyWorker f{board, future, 0};
    MyDrain d{board, future, 0};

    // implementing flow control
    const int EOS = -1;

    // declare a queue for the input tasks
    syque<int> t_queue;
    // and one for the results
    syque<int> r_queue;

    // kind of three concurrent activities
    // place input tasks into the input queue
    auto emit_task = [&](MySource s) {
        for (unsigned long i = 0; i < generations; ++i) {
            while (s.hasNext()) {
                auto t = s.next();
                t_queue.push(t);
                if (debug)
                    cout << "Emitted " << t << endl;
            }
            // wait feedback
        }
        // for as many workers to terminate, push EOS
        for (int i = 0; i < nw; i++)
            t_queue.push(EOS);
        if (debug)
            cout << "Emitted " << nw << " EOSs" << endl;
        return;
    };

    // process results
    auto proc_res = [&](MyDrain d, int nw) {
        for (unsigned long i = 0; i < generations; ++i) {
            while (true) {
                auto t = r_queue.pop();
                if (debug)
                    cout << "Drain got " << t << endl;
                if (t == EOS && (--nw) == 0)
                    break;
                d.process(t);
                if (debug)
                    cout << "Drain processed " << t << endl;
            }
        }
    };

    // processing tasks to results in parallel
    auto body = [&](MyWorker w, int wn) {
        while (true) {
            auto t = t_queue.pop();
            if (debug)
                cout << "W" << wn << " got " << t << endl;
            if (t == EOS) {
                if (debug)
                    cout << "W" << wn << " got EOS " << endl;
                r_queue.push(EOS);
                break; 
            }
            auto r = w.compute(t);
            if (debug)
                cout << "W" << wn << " computed " << r << " out of " << t << endl;
            r_queue.push(r);
        }
        return;
    };

    auto t0 = chrono::system_clock::now();
    auto e_thr = new thread(emit_task, s);
    if (debug)
        cout << "Started source" << endl;

    auto c_thr = new thread(proc_res, d, nw);
    if (debug)
        cout << "Started Drain" << endl;

    vector<thread*> tids(nw);
    for (int i = 0; i < nw; i++) {
        if (debug)
            cout << "Going to start worker " << i << endl;
        tids[i] = new thread(body, f, i);
    }

    // now await termination
    if (debug)
        cout << "Awaiting Source " << endl;
    e_thr->join();
    for (int i = 0; i < nw; i++) {
        if (debug)
            cout << "Awaiting worker " << i << endl;
        tids[i]->join();
    }
    if (debug)
        cout << "Awaiting Drain" << endl;
    c_thr->join();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(
                       chrono::system_clock::now() - t0)
                       .count();

    auto tseq = (stop - start) * (m1 + m2 + m3);
    cout << endl
         << "Parallel execution with " << nw << " threads took " << elapsed
         << " msecs" << endl
         << "sequential computation would have taken " << tseq << " msecs"
         << endl
         << "speeup is " << ((float)tseq) / ((float)elapsed) << endl;
    return (0);
}
