//
// Created by mich on 07/03/19.
// Implementation file for the class SudokuSolver
//

#include "SudokuSolver.h"

SudokuSolver::SudokuSolver(std::ifstream &input_file) : _is_solvable{true} {
   std::vector<unsigned int> input_numbers;
   unsigned int n;
   while (input_file >> n) {
      input_numbers.push_back(n);
   }
   _num_free_tiles = static_cast<unsigned int>(input_numbers.size());
   if (not is_positive_square(_num_free_tiles)) {
      throw std::invalid_argument("The input file does not have a number of values in form n^4 for n > 0 integer");
   }
   _size = static_cast<unsigned int>(std::sqrt(_num_free_tiles));
   if (not is_positive_square(_size)) {
      throw std::invalid_argument("The input file does not have a number of values in form n^4 for n > 0 integer");
   }
   _minisize = static_cast<unsigned int>(std::sqrt(_size));

   _matrix = std::vector<std::vector<Tile>>(_size, std::vector<Tile>(_size, Tile(_size)));
   Coord cd;
   for (cd.first = 0; cd.first != _size; ++cd.first) {
      for (cd.second = 0; cd.second != _size; ++cd.second) {
         if (input_numbers[cd.first * _size + cd.second] != 0) {
            _matrix[cd.first][cd.second].set_from_input(true);
            if (not set_value(cd, input_numbers[cd.first * _size + cd.second])) {
               _is_solvable = false;
               _matrix[cd.first][cd.second].set_is_conflictual(true);
               return;
            }
         }
      }
   }
}

bool SudokuSolver::has_legal_solution() const {
   for (unsigned int idx_row = 0; idx_row != _size; ++idx_row) {
      for (unsigned int idx_col = 0; idx_col != _size; ++idx_col) {
         if (not _matrix[idx_row][idx_col].has_legal_value()) {
            return false;
         }
      }
   }

   std::vector<bool> contained_values(_size, false);
   auto reset_contained_values = [&]() {
      for (unsigned int idx = 0; idx != _size; ++idx) {
         contained_values[idx] = false;
      }
   };
   auto all_values_are_contained = [&]() -> bool {
      for (const auto &val : contained_values) {
         if (not val) {
            return false;
         }
      }
      return true;
   };

   for (unsigned int idx_row = 0; idx_row != _size; ++idx_row) {
      reset_contained_values();
      for (unsigned int idx_col = 0; idx_col != _size; ++idx_col) {
         contained_values[_matrix[idx_row][idx_col].value() - 1] = true;
      }
      if (not all_values_are_contained()) {
         return false;
      }
   }
   for (unsigned int idx_col = 0; idx_col != _size; ++idx_col) {
      reset_contained_values();
      for (unsigned int idx_row = 0; idx_row != _size; ++idx_row) {
         contained_values[_matrix[idx_row][idx_col].value() - 1] = true;
      }
      if (not all_values_are_contained()) {
         return false;
      }
   }
   for (unsigned int idx_ext = 0; idx_ext != _size; ++idx_ext) {
      reset_contained_values();
      for (unsigned int idx_int = 0; idx_int != _size; ++idx_int) {
         contained_values[_matrix[(idx_ext / _minisize) * _minisize + (idx_int / _minisize)]
                          [(idx_ext % _minisize) * _minisize + (idx_int % _minisize)].value() - 1] = true;
      }
      if (not all_values_are_contained()) {
         return false;
      }
   }
   return true;
}

bool SudokuSolver::set_value(SudokuSolver::Coord coord, unsigned int value) {
   if (not _matrix[coord.first][coord.second].can_set_to(value)) {
      return false;
   }
   if (_matrix[coord.first][coord.second].is_fixed()) {
      return true;
   }
   _matrix[coord.first][coord.second].set_to_value(value, turn());
   --_num_free_tiles;
   for (int idx = 0; idx != _size; ++idx) {
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
   for (unsigned int idx_row = _minisize * (coord.first / _minisize);
        idx_row != _minisize * (coord.first / _minisize + 1); ++idx_row) {
      if (idx_row != coord.first) {
         for (unsigned int idx_col = _minisize * (coord.second / _minisize);
              idx_col != _minisize * (coord.second / _minisize + 1); ++idx_col) {
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
   for (unsigned int candidate_value = 1; candidate_value <= _size; ++candidate_value) {
      if (_matrix[tile_to_guess.first][tile_to_guess.second].can_set_to(candidate_value)) {
         _guesses_list.push_back(tile_to_guess);
         if (set_value(tile_to_guess, candidate_value) and guess()) {
            return true;
         }
         remove_guess();
         _guesses_list.pop_back();
         if (not lock_possible_value(tile_to_guess, candidate_value)) {
            return false;
         }
      }
   }
   return false;
}

void SudokuSolver::remove_guess() {
   for (unsigned int idx_row = 0; idx_row != _size; ++idx_row) {
      for (unsigned int idx_col = 0; idx_col != _size; ++idx_col) {
         if (_matrix[idx_row][idx_col].reset_from_turn(turn())) {
            ++_num_free_tiles;
         }
      }
   }
}

SudokuSolver::Coord SudokuSolver::free_tile_with_smaller_freedom() const {
   Coord tile_min_freedom{0, 0}, candidate_tile{0, 0};
   unsigned long min_freedom = _matrix[0][0].freedom_index();
   for (candidate_tile.first = 0; candidate_tile.first != _size; ++candidate_tile.first) {
      for (candidate_tile.second = 0; candidate_tile.second != _size; ++candidate_tile.second) {
         unsigned int candidate_freedom = _matrix[candidate_tile.first][candidate_tile.second].freedom_index();
         if (candidate_freedom < min_freedom) {
            tile_min_freedom = candidate_tile;
            min_freedom = candidate_freedom;
         }
      }
   }
   return tile_min_freedom;
}

bool SudokuSolver::lock_possible_value(Coord coord, unsigned int val) {
   if (not _matrix[coord.first][coord.second].lock_possibile_value(val, turn())) {
      return false;
   }

   bool should_propagate = false;
   unsigned int other_entry = _size;
   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (_matrix[coord.first][idx].can_set_to(val)) {
         if (other_entry < _size) {
            should_propagate = false;
            break;
         } else {
            should_propagate = true;
            other_entry = idx;
         }
      }
   }
   if (other_entry == _size) {
      return false;
   }
   if (should_propagate) {
      if (not set_value(Coord{coord.first, other_entry}, val)) {
         return false;
      }
   }

   should_propagate = false;
   other_entry = _size;
   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (_matrix[idx][coord.second].can_set_to(val)) {
         if (other_entry < _size) {
            should_propagate = false;
            break;
         } else {
            should_propagate = true;
            other_entry = idx;
         }
      }
   }
   if (other_entry == _size) {
      return false;
   }
   if (should_propagate) {
      if (not set_value(Coord{other_entry, coord.second}, val)) {
         return false;
      }
   }

   should_propagate = false;
   other_entry = _size;
   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (_matrix[(coord.first / _minisize) * _minisize + (idx / _minisize)][(coord.first % _minisize) * _minisize +
                                                                             (idx % _minisize)].can_set_to(val)) {
         if (other_entry < _size) {
            should_propagate = false;
            break;
         } else {
            should_propagate = true;
            other_entry = idx;
         }
      }
   }
   if (other_entry == _size) {
      return false;
   }
   if (should_propagate) {
      if (not set_value(Coord{(coord.first / _minisize) * _minisize + (other_entry / _minisize),
                              (coord.first % _minisize) * _minisize + (other_entry % _minisize)}, val)) {
         return false;
      }
   }

   return true;
}

bool SudokuSolver::is_positive_square(unsigned int n) {
   unsigned int sr = 1;
   while (n > sr * sr) {
      ++sr;
   }
   return n == sr * sr;
}

const unsigned int SudokuSolver::Tile::FREE = 0;
const int SudokuSolver::Tile::AVAILABLE = -1;

SudokuSolver::Tile::Tile(unsigned int grid_size_) : _value{FREE}, _from_input{false}, _is_conflictual{false},
                                                    _grid_size{grid_size_}, _locking_turn(grid_size_, AVAILABLE) {}

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
   if (val < 1 or val > _grid_size) {
      throw std::logic_error("Call of lock_possible_value for illegal value");
   }
   if (_value == val) {
      return false;
   }
   if (_locking_turn[val - 1] == AVAILABLE) {
      _locking_turn[val - 1] = turn;
   }
   return (num_possibilities() > 0 or is_fixed());
}

bool SudokuSolver::Tile::can_set_to(unsigned int value) const {
   if (_value == value) {
      return true;
   }
   return not is_fixed() and value > 0 and value <= _grid_size and _locking_turn[value - 1] == AVAILABLE;
}

unsigned int SudokuSolver::Tile::num_possibilities() const {
   if (is_fixed()) {
      return 1;
   }
   unsigned int counter = 0;
   for (unsigned int val = 1; val <= _grid_size; ++val) {
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
   for (unsigned int possibility = 1; possibility <= _grid_size; ++possibility) {
      if (can_set_to(possibility)) {
         return possibility;
      }
   }
   throw std::logic_error("Call to first choice available without any choice available");
}

std::ostream &operator<<(std::ostream &os, const SudokuSolver &sudoku) {
   unsigned int num_digits = 1;  // the number of digits of the largest number
   for (unsigned larger_num = 10; larger_num <= sudoku.get_size(); larger_num *= 10) {
      ++num_digits;
   }

   std::string horizontal_line(sudoku.get_size() * (num_digits + 1) + 1, '-');
   for (unsigned int idx_row = 0; idx_row != sudoku.get_size(); ++idx_row) {
      if (idx_row % sudoku.get_minisize() == 0) {
         os << horizontal_line << std::endl;
      }
      for (unsigned int idx_col = 0; idx_col != sudoku.get_size(); ++idx_col) {
         char separator = idx_col % sudoku.get_minisize() == 0 ? '|' : ' ';
         os << separator;
         if (sudoku.matrix()[idx_row][idx_col].get_is_conflictual()) {
            os << "\033[1;31m";
         } else if (sudoku.matrix()[idx_row][idx_col].get_from_input()) {
            os << "\033[1;33m";
         } else {
            os << "\033[1;37m";
         }
         std::string num_string = std::to_string(sudoku.matrix()[idx_row][idx_col].value());
         std::string extra_space(num_digits - num_string.size(), ' ');
         os << extra_space << num_string << "\033[0m";
      }
      os << "|\n";
   }
   return os << horizontal_line << std::endl;
}