#include <iostream>
#include <thread>
#include <vector>

using namespace std;

int count_alive_neighbours(const vector<vector<int>> &board, size_t row, size_t col) {
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

void update(const vector<vector<int>> &board, vector<vector<int>> &future) {
    for (size_t i = 1; i < board.size() - 1; ++i) {
        #pragma GCC ivdep
        for (size_t j = 1; j < board[i].size() - 1; ++j) {
            int alive_neighbours = count_alive_neighbours(board, i, j);
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
    // TODO: from command line
    const size_t rows{1000}, cols{1000};
    const unsigned long generations{1000};

    // boards allocation
    vector<vector<int>> board(rows, vector(cols, 0));
    vector<vector<int>> future(rows, vector(cols, 0));

    // board initialization
    srand(time(nullptr));
    for (size_t i = 1; i < board.size() - 1; ++i)
        for (size_t j = 1; j < board[i].size() - 1; ++j)
            board[i][j] = rand() % 2;
    
    /* cout << "0/" << generations << endl;
    print(board); */

    for (unsigned long it = 0; it < generations; ++it) {
        update(board, future);
        board.swap(future);

        /* cout << string(20, '\n'); // "clear" the screen
        cout << it + 1 << "/" << generations << endl;
        print(board); */
    }

    return 0;
}
