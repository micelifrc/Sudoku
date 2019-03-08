//
// Created by mich on 07/03/19.
//

#include "SudokuSolver.h"

SudokuSolver::SudokuSolver(std::ifstream &input_file) : _num_free_tiles{SIZE * SIZE}, _is_solvable{true} {
   _guesses.reserve(SIZE * SIZE);
   std::vector<unsigned int> input_numbers;
   char c;
   while (input_file >> c) {
      if (c >= '0' and c <= '9') {
         input_numbers.emplace_back(static_cast<int>(c - '0'));
      }
   }
   if (input_numbers.size() != SIZE * SIZE) {
      throw std::invalid_argument(
            "The input file has the wrong number of numbers: " + std::to_string(input_numbers.size()) + " != " +
            std::to_string(SIZE * SIZE));
   }

   Coord cd;
   for (cd.first = 0; cd.first != SIZE; ++cd.first) {
      for (cd.second = 0; cd.second != SIZE; ++cd.second) {
         if (input_numbers[cd.first * SIZE + cd.second] != 0) {
            _matrix[cd.first][cd.second].set_from_input(true);
            if (not set_value(cd, input_numbers[cd.first * SIZE + cd.second])) {
               _is_solvable = false;
               _matrix[cd.first][cd.second].set_is_conflictual(true);
               return;
            }
         }
      }
   }
}

bool SudokuSolver::solve() {
   return _is_solvable and guess();
}

// currently not working accordingly
bool SudokuSolver::set_value(SudokuSolver::Coord coord, unsigned int value) {
   if (not _matrix[coord.first][coord.second].can_set_to(value)) {
      return false;
   }
   if (not _matrix[coord.first][coord.second].is_fixed()) {
      _matrix[coord.first][coord.second].set_to_value(value, turn());
      --_num_free_tiles;
   }
   for (int idx = 0; idx != SIZE; ++idx) {
      if (idx != coord.first) {
         if (not _matrix[idx][coord.second].lock_possibile_value(value, turn())) {
            return false;
         }
         if (_matrix[idx][coord.second].num_possibilities() == 1 and not _matrix[idx][coord.second].is_fixed()) {
            if (not set_value(Coord{idx, coord.second}, _matrix[idx][coord.second].first_choice_available())) {
               return false;
            }
         }
      }
      if (idx != coord.second) {
         if (not _matrix[coord.first][idx].lock_possibile_value(value, turn())) {
            return false;
         }
         if (_matrix[coord.first][idx].num_possibilities() == 1 and not _matrix[coord.first][idx].is_fixed()) {
            if (not set_value(Coord{coord.first, idx}, _matrix[coord.first][idx].first_choice_available())) {
               return false;
            }
         }
      }
   }
   for (unsigned int idx_row = MINISIZE * (coord.first / MINISIZE);
        idx_row != MINISIZE * (coord.first / MINISIZE + 1); ++idx_row) {
      if (idx_row != coord.first) {
         for (unsigned int idx_col = MINISIZE * (coord.second / MINISIZE);
              idx_col != MINISIZE * (coord.second / MINISIZE + 1); ++idx_col) {
            if (idx_col != coord.second) {
               if (not _matrix[idx_row][idx_col].lock_possibile_value(value, turn())) {
                  return false;
               }
               if (_matrix[idx_row][idx_col].num_possibilities() == 1 and not _matrix[idx_row][idx_col].is_fixed()) {
                  if (not set_value(Coord{idx_row, idx_col}, _matrix[idx_row][idx_col].first_choice_available())) {
                     return false;
                  }
               }
            }
         }
      }
   }
   return true;
}

bool SudokuSolver::guess() {
   if (_num_free_tiles == 0) {
      return true;
   }
   Coord tile_to_guess = free_tile_with_smaller_freedom();
   _guesses.push_back(tile_to_guess);
   for (unsigned int candidate_value = 1; candidate_value <= SIZE; ++candidate_value) {
      if (_matrix[tile_to_guess.first][tile_to_guess.second].can_set_to(candidate_value)) {
         if (set_value(tile_to_guess, candidate_value) and guess()) {
            return true;
         }
         remove_guess();
         _matrix[tile_to_guess.first][tile_to_guess.second].lock_possibile_value(candidate_value, turn() - 1);
      }
   }
   _guesses.pop_back();
   return false;
}

void SudokuSolver::remove_guess() {
   for (unsigned int idx_row = 0; idx_row != SIZE; ++idx_row) {
      for (unsigned int idx_col = 0; idx_col != SIZE; ++idx_col) {
         if(_matrix[idx_row][idx_col].reset_from_turn(turn())) {
            ++_num_free_tiles;
         }
      }
   }
}

SudokuSolver::Coord SudokuSolver::free_tile_with_smaller_freedom() const {
   Coord tile_min_freedom{0, 0}, candidate_tile{0, 0};
   unsigned long min_freedom = _matrix[0][0].freedom_index();
   for (candidate_tile.first = 0; candidate_tile.first != SIZE; ++candidate_tile.first) {
      for (candidate_tile.second = 0; candidate_tile.second != SIZE; ++candidate_tile.second) {
         unsigned int candidate_freedom = _matrix[candidate_tile.first][candidate_tile.second].freedom_index();
         if (candidate_freedom < min_freedom) {
            tile_min_freedom = candidate_tile;
            min_freedom = candidate_freedom;
         }
      }
   }
   return tile_min_freedom;
}

SudokuSolver::Tile::Tile() : _locking_turn{}, _value{FREE}, _from_input{false}, _is_conflictual{false} {
   for (auto &del_turn: _locking_turn) {
      del_turn = AVAILABLE;
   }
}

bool SudokuSolver::Tile::set_to_value(unsigned int val, unsigned int turn) {
   if (not can_set_to(val) or is_fixed()) {
      return false;
   }
   _value = val;
   for (auto &possibility : _locking_turn) {
      if (possibility == AVAILABLE) {
         possibility = turn;
      }
   }
   return true;
}

bool SudokuSolver::Tile::lock_possibile_value(unsigned int val, unsigned int turn) {
   if (val < 1 or val > SIZE) {
      throw std::logic_error("Call of lock_possible_value for illegal value");
   }
   if(_value == val) {
      return false;
   }
   if(_locking_turn[val - 1] == AVAILABLE) {
      _locking_turn[val - 1] = turn;
   }
   return (num_possibilities() > 0 or is_fixed());
}

bool SudokuSolver::Tile::can_set_to(unsigned int value) const {
   if (_value == value) {
      return true;
   }
   return not is_fixed() and value > 0 and value <= SIZE and _locking_turn[value - 1] == AVAILABLE;
}

unsigned int SudokuSolver::Tile::num_possibilities() const {
   if (is_fixed()) {
      return 1;
   }
   unsigned int counter = 0;
   for (unsigned int val = 1; val <= SIZE; ++val) {
      if (can_set_to(val)) {
         ++counter;
      }
   }
   return counter;
}

bool SudokuSolver::Tile::reset_from_turn(unsigned int turn) {
   bool is_freed = false;
   if (is_fixed() and _locking_turn[_value - 1] >= turn) {
      _value = FREE;
      is_freed = true;
   }
   for (auto &possibility : _locking_turn) {
      if (possibility >= turn) {
         possibility = AVAILABLE;
      }
   }
   return is_freed;
}


unsigned int SudokuSolver::Tile::first_choice_available() const {
   for (unsigned int possibility = 1; possibility <= SIZE; ++possibility) {
      if (can_set_to(possibility)) {
         return possibility;
      }
   }
   throw std::logic_error("Call to first choice available without any choice available");
}

std::ostream &operator<<(std::ostream &os, const SudokuSolver &sudoku) {
   std::string horizontal_line(SudokuSolver::SIZE + SudokuSolver::MINISIZE + 1, '-');
   for (unsigned int idx_row = 0; idx_row != SudokuSolver::SIZE; ++idx_row) {
      if (idx_row % SudokuSolver::MINISIZE == 0) {
         os << horizontal_line << std::endl;
      }
      for (unsigned int idx_col = 0; idx_col != SudokuSolver::SIZE; ++idx_col) {
         if (idx_col % SudokuSolver::MINISIZE == 0) {
            os << '|';
         }
         if(sudoku.matrix()[idx_row][idx_col].get_is_conflictual()) {
            os << "\033[1;31m";
         } else if (sudoku.matrix()[idx_row][idx_col].get_from_input()) {
            os << "\033[1;33m";
         } else {
            os << "\033[1;37m";
         }
         os << sudoku.matrix()[idx_row][idx_col].value() << "\033[0m";
      }
      os << "|\n";
   }
   return os << horizontal_line << std::endl;
}