// this is the business logic code
#include "BLcode.hpp"

#include <chrono>
#include <iostream>
#include <thread>

using namespace std;

// tasks to be computed: stream of integers, provided as iterator
class MySource : public Source<int> {
   private:
    vector<vector<int>> &board;
    int msec;
    size_t row;

   public:
    MySource(vector<vector<int>> &board, int ms)
        : board(board), msec(ms), row(1) {}

    int next() {
        return row++;
    }

    bool hasNext() {
        return row < board.size() - 1;
    }
};

// business logic to compute a task
class MyWorker : public Worker<int, int> {
   private:
    const vector<vector<int>> &board;
    vector<vector<int>> &future;
    int msec;

    int count_alive_neighbours(int row, int col) {
        int n_alive{0};

        for (int i = -1; i <= 1; ++i)
            for (int j = -1; j <= 1; ++j)
                n_alive += board[row + i][col + j];

        n_alive -= board[row][col]; // remove myself from the count

        return n_alive;
    }

    int compute_future(int alive, int alive_neighbours) {
        if (alive_neighbours < 2 || alive_neighbours > 3)
            return 0;
        else if (alive_neighbours == 3)
            return 1;
        else
            return alive;
    }

   public:
    MyWorker(const vector<vector<int>> &board, vector<vector<int>> &future, int ms)
        : board(board), future(future), msec(ms) {}

    int compute(int row) {
        //#pragma GCC ivdep
        for (size_t j = 1; j < board[row].size() - 1; ++j) {
            int alive_neighbours = count_alive_neighbours(row, j);
            future[row][j] = compute_future(board[row][j], alive_neighbours);
        }
        return 0;
    }
};

// processing the results: accumulate the stream contents
class MyDrain : public Drain<int> {
   private:
    vector<vector<int>> &board, &future;
    int msec;

   public:
    MyDrain(vector<vector<int>> &board, vector<vector<int>> &future, int ms)
        : board(board), future(future), msec(ms) {}

    void process(int x) {
        this_thread::sleep_for(chrono::milliseconds(msec));
        return;
    }
};
