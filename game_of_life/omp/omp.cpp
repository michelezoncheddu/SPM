#include <iostream>
#include <thread>
#include <vector>

using namespace std;

int compute_future(int alive, int alive_neighbours) {
    if (alive_neighbours < 2 || alive_neighbours > 3)
        return 0;
    else if (alive_neighbours == 3)
        return 1;
    else
        return alive;
}

void update(const vector<vector<int>> &board, vector<vector<int>> &future, int nw) {
    #pragma omp parallel for num_threads(nw)
    for (size_t i = 1; i < board.size() - 1; ++i) {
        #pragma GCC ivdep
        for (size_t j = 1; j < board[i].size() - 1; ++j) {
            int alive_neighbours =
                board[i - 1][j - 1] + board[i - 1][j] + board[i - 1][j + 1] +
                board[i][j - 1] + board[i][j + 1] +
                board[i + 1][j - 1] + board[i + 1][j] + board[i + 1][j + 1];
            future[i][j] = compute_future(board[i][j], alive_neighbours);
        }
    }
}

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

int main(int argc, char const *argv[]) {
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
    
    auto t0 = chrono::system_clock::now();
    for (unsigned long it = 0; it < generations; ++it) {
        update(board, future, nw);
        board.swap(future);

        cout << it + 1 << "/" << generations << endl;
        print(board);
    }
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(
                       chrono::system_clock::now() - t0)
                       .count();
    cout << "Parallel execution with " << nw << " workers took " << elapsed << " msecs" << endl;
    return 0;
}
