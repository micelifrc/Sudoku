/*
 * This program will solve a sudoku problem
 */

#include <iostream>
#include <fstream>
#include "SudokuSolver.h"

int main(int argc, char *argv[]) {
   std::ifstream input_file;
   if (argc < 2) {
      throw std::invalid_argument("Error! Need the input files as arguments");
   }
   std::cout << "These are the solutions for the required Sudoku puzzles:\n\n";
   for (unsigned int idx = 1; idx != argc; ++idx) {
      try {
         input_file.open(argv[idx]);
         SudokuSolver sudoku(input_file);
         if (input_file.is_open()) {
            input_file.close();
         }
         if (sudoku.solve() and sudoku.has_legal_solution()) {
            std::cout << "The puzzle in file \"" << argv[idx] << "\" has solution:\n" << sudoku << std::endl;
         } else {
            std::cout << "The puzzle in file \"" << argv[idx] << "\" cannot be solved" << std::endl;
            std::cout << "This is a partial solution:\n" << sudoku << std::endl;
         }
      } catch (std::exception &err) {
         std::cerr << err.what() << std::endl;
      }
   }
   std::cout << "Thank you for playing with me" << std::endl;
   return 0;
}