#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>

struct cell_t {
    bool alive;  // current state
    bool future; // future state
};

using board_t = cell_t**;

int count_alive_neighbours(const board_t &board, size_t row, size_t col, size_t rows, size_t cols) {
    int n_alive{0};

    // offsets
    const int sr{row > 0 ? -1 : 0};       // start row
    const int sc{col > 0 ? -1 : 0};       // start column
    const int er{row < rows - 1 ? 1 : 0}; // end row
    const int ec{col < cols - 1 ? 1 : 0}; // end column

    for (int i = sr; i <= er; ++i)
        for (int j = sc; j <= ec; ++j)
            n_alive += board[row + i][col + j].alive;

    n_alive -= board[row][col].alive; // remove myself from the count

    return n_alive;
}

bool compute_future(bool alive, int alive_neighbours) {
    if (alive_neighbours < 2 || alive_neighbours > 3)
        return 0;
    else if (alive_neighbours == 3)
        return 1;
    else
        return alive;
}

void update(const board_t &board, size_t rows, size_t cols) {
    // compute future state
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            int alive_neighbours = count_alive_neighbours(board, i, j, rows, cols);
            board[i][j].future = compute_future(board[i][j].alive, alive_neighbours);
        }
    }

    // update state
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            board[i][j].alive = board[i][j].future;
}

void print(const board_t &board, size_t rows, size_t cols) {
    std::string border(cols + 2, '-');

    std::cout << border << std::endl;
    for (size_t i = 0; i < rows; ++i) {
        std::cout << '|';
        for (size_t j = 0; j < cols; ++j)
            std::cout << (board[i][j].alive ? '*' : ' ');
        std::cout << '|' << std::endl;
    }
    std::cout << border << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

int main(int argc, char const *argv[]) {
    // TODO: from command line
    const size_t rows{1000}, cols{1000};
    const unsigned long generations{1000};

    // board allocation
    board_t board = new cell_t*[rows];
    for (size_t i = 0; i < rows; ++i)
        board[i] = new cell_t[cols];

    // board initialization
    std::srand(std::time(nullptr));
    for (size_t i = 0; i < 1; ++i)
        for (size_t j = 0; j < cols; ++j)
            board[i][j].alive = true; //std::rand() % 3;
    
    std::cout << "0/" << generations << std::endl;
    print(board, rows, cols);

    for (unsigned long it = 0; it < generations; ++it) {
        update(board, rows, cols);

        std::cout << std::string(20, '\n'); // "clear" the screen
        std::cout << it + 1 << "/" << generations << std::endl;
        print(board, rows, cols);
    }

    for (size_t i = 0; i < rows; ++i)
        delete[] board[i];
    delete[] board;

    return 0;
}
