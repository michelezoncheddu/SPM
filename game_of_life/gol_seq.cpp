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

    for (size_t i = row + sr; i <= row + er; ++i)
        for (size_t j = col + sc; j <= col + ec; ++j)
            if (board[i][j].alive)
                ++n_alive;

    n_alive -= board[row][col].alive; // remove myself from the count

    return n_alive;   
}

void update(const board_t &board, size_t rows, size_t cols) {
    // compute future state
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j) {
            int alive_neighbours = count_alive_neighbours(board, i, j, rows, cols);
            if (alive_neighbours < 2 || alive_neighbours > 3)
                board[i][j].future = false;
            else if (alive_neighbours == 3)
                board[i][j].future = true;
            else
                board[i][j].future = board[i][j].alive;
        }
    }

    // update state
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            board[i][j].alive = board[i][j].future;
}

void print(const board_t &board, size_t rows, size_t cols) {
    for (size_t i = 0; i < rows; ++i) {
        for (size_t j = 0; j < cols; ++j)
            std::cout << (board[i][j].alive ? '*' : ' ');
        std::cout << std::endl;
    }
    std::cout << "---------------------------------------" << std::endl;
}

int main(int argc, char const *argv[]) {
    // TODO: from command line
    const size_t rows{20}, cols{20};
    const unsigned long generations{200};

    // board allocation
    board_t board = new cell_t*[rows];
    for (size_t i = 0; i < rows; ++i)
        board[i] = new cell_t[cols];

    // board initialization
    std::srand(std::time(nullptr));
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            board[i][j].alive = std::rand() % 2;
    
    for (unsigned long it = 0; it < generations; ++it) {
        update(board, rows, cols);
        print(board, rows, cols);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    for (size_t i = 0; i < rows; ++i)
        delete[] board[i];
    delete[] board;

    return 0;
}
