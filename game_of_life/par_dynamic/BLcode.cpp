// this is the business logic code
#include <chrono>
#include <iostream>
#include <thread>

#include "BLcode.hpp"

using namespace std;

void print(const vector<vector<int>> &board) {
    string border(board[0].size() + 2, '-');

    cout << border << endl;
    for (size_t i = 0; i < board.size(); ++i) {
        cout << '|';
        for (size_t j = 0; j < board[i].size(); ++j)
            cout << (board[i][j] ? '*' : ' ');
        cout << '|' << endl;
    }
    cout << border << endl;
    this_thread::sleep_for(chrono::milliseconds(50));
}

// tasks to be computed: stream of rows, provided as iterator
class MySource : public Source<pair<int, int>> {
   private:
    vector<vector<int>> &board;
    int msec;
    size_t row;
    int chunk_size;

   public:
    MySource(vector<vector<int>> &board, int ms, int nw, int chunk_size)
        : board(board), msec(ms), row(1), chunk_size(chunk_size) {
        //print(board); // initial configuration
    }

    // NOTE: it doesn't divide equally in the last partition
    pair<int, int> next() {
        pair<int, int> next;
        if (row + chunk_size < board.size() - 1)
            next = {row, chunk_size};
        else
            next = {row, (board.size() - 1) - row}; // remaining
        row += chunk_size;
        return next;
    }

    bool hasNext() {
        return row < board.size() - 1;
    }

    void feedback_notify() {
        row = 1; // start from the beginning
    }
};

// business logic to compute a task
class MyWorker : public Worker<pair<int, int>, int> {
   private:
    const vector<vector<int>> &board;
    vector<vector<int>> &future;
    int msec;

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

    int compute(pair<int, int> pair) {
        const int start{pair.first}, chunk_size{pair.second};
        for (int i = start; i < start + chunk_size; ++i) {
            #pragma GCC ivdep
            for (size_t j = 1; j < board[i].size() - 1; ++j) {
                int alive_neighbours =
                    board[i - 1][j - 1] + board[i - 1][j] + board[i - 1][j + 1] +
                    board[i][j - 1] + board[i][j + 1] +
                    board[i + 1][j - 1] + board[i + 1][j] + board[i + 1][j + 1];
                future[i][j] = compute_future(board[i][j], alive_neighbours);
            }
        }
        return chunk_size; // number of rows computed
    }
};

// processing the results: accumulate the stream contents
class MyDrain : public Drain<int, bool> {
   private:
    vector<vector<int>> &board, &future;
    int msec;
    int remaining;

   public:
    MyDrain(vector<vector<int>> &board, vector<vector<int>> &future, int ms)
        : board(board), future(future), msec(ms) {
        remaining = board.size() - 2;
    }

    /**
     * par x:  # of rows computed
     * return: feedback
     */
    bool process(int x) {
        if (x < 0) // not a valid row
            return false;

        remaining -= x;
        //cout << "remaining " << remaining << " rows" << endl;

        // workers have finished
        if (remaining == 0) {
            remaining = board.size() - 2;
            swap(board, future);
            //print(board);
            return true; // send feedback
        }
        return false;
    }
};
