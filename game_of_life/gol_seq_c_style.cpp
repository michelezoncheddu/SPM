#include <cstdlib>
#include <ctime>
#include <iostream>
#include <thread>

int count_alive_neighbours(bool **board, size_t row, size_t col, size_t rows, size_t cols) {
    int n_alive{0};

    for (int i = -1; i <= 1; ++i)
        for (int j = -1; j <= 1; ++j)
            n_alive += board[row + i][col + j];

    n_alive -= board[row][col]; // remove myself from the count

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

void update(bool **board, bool **future, size_t rows, size_t cols) {
    for (size_t i = 1; i < rows - 1; ++i) {
        #pragma GCC ivdep
        for (size_t j = 1; j < cols - 1; ++j) {
            int alive_neighbours = count_alive_neighbours(board, i, j, rows, cols);
            future[i][j] = compute_future(board[i][j], alive_neighbours);
        }
    }
}

void print(bool **board, size_t rows, size_t cols) {
    std::string border(cols + 2, '-');

    std::cout << border << std::endl;
    for (size_t i = 0; i < rows; ++i) {
        std::cout << '|';
        for (size_t j = 0; j < cols; ++j)
            std::cout << (board[i][j] ? '*' : ' ');
        std::cout << '|' << std::endl;
    }
    std::cout << border << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
}

int main(int argc, char const *argv[]) {
    // TODO: from command line
    const size_t rows{70}, cols{150};
    const unsigned long generations{1000};

    // boards allocation
    bool **board = new bool*[rows];
    bool **future = new bool*[rows];
    for (size_t i = 0; i < rows; ++i) {
        board[i] = new bool[cols];
        future[i] = new bool[cols];
    }

    // board initialization
    std::srand(std::time(nullptr));
    for (size_t i = 0; i < rows; ++i)
        for (size_t j = 0; j < cols; ++j)
            board[i][j] = false;
    for (size_t i = 1; i < 2; ++i)
        for (size_t j = 1; j < cols - 1; ++j)
            board[i][j] = true; //std::rand() % 3;
    
    std::cout << "0/" << generations << std::endl;
    print(board, rows, cols);

    for (unsigned long it = 0; it < generations; ++it) {
        update(board, future, rows, cols);
        std::swap(board, future);

        std::cout << std::string(20, '\n'); // "clear" the screen
        std::cout << it + 1 << "/" << generations << std::endl;
        print(board, rows, cols);
    }

    for (size_t i = 0; i < rows; ++i) {
        delete[] board[i];
        delete[] future[i];
    }
    delete[] board;
    delete[] future;

    return 0;
}
