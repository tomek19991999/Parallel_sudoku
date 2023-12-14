#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <stack>
#include <algorithm>
#include <mutex>
#include <unordered_map>
#include "CycleTimer.h"
#include "pthread.h"

const int sudoku_size = 9;
int square_size = 3;
int recursion_depth; //The number of slots a thread fills before pushing it onto the stack
static int how_many_threads;

class Board {
public:
      int sudoku_board[sudoku_size][sudoku_size]; //sudoku board
      Board() {};

      void show_board()
      {
          for (int i = 0; i < sudoku_size; i++) {
              for (int j = 0; j < sudoku_size; j++)
                  std::cout << sudoku_board[i][j] << " ";
              std::cout << std::endl;
          }
          std::cout << std::endl;
      }

      int how_many_empty_cells() {
          int cnt = 0;
          for (int i = 0; i < sudoku_size; i++)
              for (int j = 0; j < sudoku_size; j++)
                  cnt += (sudoku_board[i][j] == 0);
          return cnt;
      }
};

bool is_no_conflict(int matrix[sudoku_size][sudoku_size], int row, int col, int number) {

    for (int i = 0; i < sudoku_size; i++) {
        if (matrix[i][col] == number)
            return false;
    }

    for (int j = 0; j < sudoku_size; j++) {
        if (matrix[row][j] == number)
            return false;
    }

    for (int i = 0; i < square_size; i++) {
        for (int j = 0; j < square_size; j++) {
            if (matrix[(row / square_size) * square_size + i][(col / square_size) * square_size + j] == number)
                return false;
        }
    }

    return true;
}

int next_empty_cell_index(int matrix[sudoku_size][sudoku_size], int start) {
    int i;
    for (i = start; i < sudoku_size * sudoku_size; i++) {
        if (matrix[i / sudoku_size][i % sudoku_size] == 0) {
            return i;
        }
    }
    return i;
}
std::mutex mtx;

bool backtracking(std::stack < std::pair < int, Board >>& stk, Board b, int index, int depth) {
    int row = index / sudoku_size;
    int col = index % sudoku_size;
    //b.show_board();
    if (index >= sudoku_size * sudoku_size) { //if got end of board (solved sudoku)
        mtx.lock();
        stk.push(std::pair < int, Board >(index, b));
        mtx.unlock();
        return true;
    }

    if (!depth) { // if got max depth, than push and exit
        mtx.lock();
        stk.push(std::pair < int, Board >(index, b));
        mtx.unlock();
        return false;
    }

    for (int k = 1; k <= sudoku_size; k++) {
        if (is_no_conflict(b.sudoku_board, row, col, k)) {
            b.sudoku_board[row][col] = k;
            if (backtracking(stk, b, next_empty_cell_index(b.sudoku_board, index + 1), depth - 1))
                return true;
        }
    }

    return false;
}

void local_backtracking(Board& plansza_sudoku) {
    std::stack < std::pair < int, Board >> stk;
    std::vector < std::thread > threads;

    bool czy_ukonczono = false;

    Board tmp(plansza_sudoku);

    stk.push(std::pair < int, Board >(next_empty_cell_index(tmp.sudoku_board, 0), tmp));
    int index = next_empty_cell_index(tmp.sudoku_board, 0);

    for (int id = 0; id < how_many_threads; id++) {
            threads.push_back(std::thread([&czy_ukonczono, id, &stk, &plansza_sudoku]() {
                while (!czy_ukonczono) {
                    mtx.lock();
                    if (stk.size()) {
                        int index = stk.top().first; //take index of "0"
                        Board b = stk.top().second; //take sudoku map
                        stk.pop(); //remove first element
                        mtx.unlock();
                        if (b.how_many_empty_cells() == 0) {
                            plansza_sudoku = b;
                            czy_ukonczono = true;
                            break;
                        }
                        backtracking(stk, b, index, recursion_depth);
                        
                    }
                    else mtx.unlock();
                }
            }));
    }
    for (auto& thread : threads)
        thread.join();
}

int main() {
    recursion_depth = 25;
    how_many_threads = 4;

    Board b;

    //get sudoku from txt file
    std::ifstream file("sudoku.txt");
    for (int i = 0; i < sudoku_size; i++)
    {
        for (int j = 0; j < sudoku_size; j++) {
            file >> b.sudoku_board[i][j];
        }
    }
    b.show_board();

    //calculations
    double startTime = CycleTimer::currentSeconds();
    local_backtracking(b);
    double end_time = CycleTimer::currentSeconds();


    std::cout << "--------------" << std::endl << "threads: " << how_many_threads << std::endl;
    std::cout << "depth: " << recursion_depth << std::endl;

    b.show_board();
    std::cout  << "calculation time: " << end_time - startTime  << std::endl << std::endl;

    return 0;
}