//
// Created by mich on 07/03/19.
// Implementation file for the class SudokuSolver
//

#include "SudokuSolver.h"

const unsigned int SudokuSolver::FREE = 0;
const int SudokuSolver::AVAILABLE = -1;

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
   _geo_blocks.reserve(_size);
   for (unsigned int value = 1; value <= _size; ++value) {
      _geo_blocks.emplace_back(std::array<std::vector<GeoBlock>, 3>{});
      for (unsigned int dir = 0; dir != 3; ++dir) {
         _geo_blocks.back()[dir].reserve(_size);
         for (unsigned int idx = 0; idx != _size; ++idx) {
            _geo_blocks.back()[dir].emplace_back(*this, static_cast<GeoDir>(dir), idx);
         }
      }
   }

   Coord cd;
   for (cd.row_idx = 0; cd.row_idx != _size; ++cd.row_idx) {
      for (cd.col_idx = 0; cd.col_idx != _size; ++cd.col_idx) {
         if (input_numbers[cd.row_idx * _size + cd.col_idx] != 0) {
            tile(cd).set_from_input(true);
            if (not set_value(cd, input_numbers[cd.row_idx * _size + cd.col_idx])) {
               _is_solvable = false;
               tile(cd).set_is_conflictual(true);
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
   if (not tile(coord).can_set_to(value)) {
      return false;
   }
   if (tile(coord).is_fixed()) {
      return true;
   }

   tile(coord).set_to_value(value, turn());
   for (unsigned int forb_value = 1; forb_value <= _size; ++forb_value) {
      lock_all_geo_blocks(coord, forb_value);
   }
   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (idx != coord.row_idx) {
         lock_all_geo_blocks(Coord{idx, coord.col_idx}, value);
      }
      if (idx != coord.col_idx) {
         lock_all_geo_blocks(Coord{coord.row_idx, idx}, value);
      }
   }
   for (unsigned int row_idx = (coord.row_idx / _minisize) * _minisize;
        row_idx != (coord.row_idx / _minisize + 1) * _minisize; ++row_idx) {
      if (row_idx == coord.row_idx) continue;
      for (unsigned int col_idx = (coord.col_idx / _minisize) * _minisize;
           col_idx != (coord.col_idx / _minisize + 1) * _minisize; ++col_idx) {
         if (col_idx == coord.col_idx) continue;
         lock_all_geo_blocks(Coord{row_idx, col_idx}, value);
      }
   }
   --_num_free_tiles;

   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (idx != coord.row_idx) {
         if (not _matrix[idx][coord.col_idx].lock_possibile_value(value, turn())) {
            return false;
         }
         if (_matrix[idx][coord.col_idx].num_possibilities() == 1 and not _matrix[idx][coord.col_idx].is_fixed()) {
            if (not set_value(Coord{idx, coord.col_idx}, _matrix[idx][coord.col_idx].first_choice_available())) {
               return false;
            }
         }
      }
      if (idx != coord.col_idx) {
         if (not _matrix[coord.row_idx][idx].lock_possibile_value(value, turn())) {
            return false;
         }
         if (_matrix[coord.row_idx][idx].num_possibilities() == 1 and not _matrix[coord.row_idx][idx].is_fixed()) {
            if (not set_value(Coord{coord.row_idx, idx}, _matrix[coord.row_idx][idx].first_choice_available())) {
               return false;
            }
         }
      }
   }
   for (unsigned int idx_row = _minisize * (coord.row_idx / _minisize);
        idx_row != _minisize * (coord.row_idx / _minisize + 1); ++idx_row) {
      if (idx_row != coord.row_idx) {
         for (unsigned int idx_col = _minisize * (coord.col_idx / _minisize);
              idx_col != _minisize * (coord.col_idx / _minisize + 1); ++idx_col) {
            if (idx_col != coord.col_idx) {
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
   Coord tile_to_guess_coord = free_tile_with_smaller_freedom();
   Tile &tile_to_guess = tile(tile_to_guess_coord);
   GeoCoord smaller_free_geo_block_coord = free_geo_block_with_smaller_freedom();
   const GeoBlock &geo_block_to_fix = geo_block(smaller_free_geo_block_coord);
   if (tile_to_guess.freedom_index() <= geo_block_to_fix.freedom_index()) {
      for (unsigned int candidate_value = 1; candidate_value <= _size; ++candidate_value) {
         if (tile_to_guess.can_set_to(candidate_value)) {
            _guesses_list.push_back(tile_to_guess_coord);
            if (set_value(tile_to_guess_coord, candidate_value) and guess()) {
               return true;
            }
            remove_guess();
            _guesses_list.pop_back();
            if (not lock_possible_value(tile_to_guess_coord, candidate_value)) {
               return false;
            }
         }
      }
   } else {
      for (Coord candidate_coord : geo_block_to_fix.avaliable_coordinates()) {
         Tile &candidate_tile = tile(candidate_coord);
         if (candidate_tile.can_set_to(smaller_free_geo_block_coord.value)) {
            _guesses_list.push_back(tile_to_guess_coord);
            if (set_value(candidate_coord, smaller_free_geo_block_coord.value) and guess()) {
               return true;
            }
            remove_guess();
            _guesses_list.pop_back();
         }
      }
   }
   return false;
}

void SudokuSolver::remove_guess() {
   Coord coord;
   for (coord.row_idx = 0; coord.row_idx != _size; ++coord.row_idx) {
      for (coord.col_idx = 0; coord.col_idx != _size; ++coord.col_idx) {
         if (tile(coord).reset_from_turn(turn())) {
            ++_num_free_tiles;
         }
      }
   }
   for (auto &block : _geo_blocks) {
      for (auto &dir : block) {
         for (auto &geo_unity : dir) {
            geo_unity.reset_from_turn(turn());
         }
      }
   }
}

SudokuSolver::Coord SudokuSolver::free_tile_with_smaller_freedom() const {
   Coord tile_min_freedom{0, 0}, candidate_tile{0, 0};
   auto min_freedom = std::numeric_limits<unsigned int>::max();
   for (candidate_tile.row_idx = 0; candidate_tile.row_idx != _size; ++candidate_tile.row_idx) {
      for (candidate_tile.col_idx = 0; candidate_tile.col_idx != _size; ++candidate_tile.col_idx) {
         unsigned int candidate_freedom = tile(candidate_tile).freedom_index();
         if (candidate_freedom < min_freedom) {
            tile_min_freedom = candidate_tile;
            min_freedom = candidate_freedom;
         }
      }
   }
   return tile_min_freedom;
}

SudokuSolver::GeoCoord SudokuSolver::free_geo_block_with_smaller_freedom() const {
   GeoCoord geo_coord_min_freedom{0, 0, 0}, candidate_geo_coord{0, 0, 0};
   unsigned int min_freedom = std::numeric_limits<unsigned int>::max(), candidate_freedom = 0;
   for (candidate_geo_coord.value = 1; candidate_geo_coord.value <= _size; ++candidate_geo_coord.value) {
      for (unsigned int dir_idx = 0; dir_idx != 3; ++dir_idx) {
         candidate_geo_coord.dir = static_cast<GeoDir>(dir_idx);
         for (candidate_geo_coord.idx = 0; candidate_geo_coord.idx != _size; ++candidate_geo_coord.idx) {
            candidate_freedom = _geo_blocks[candidate_geo_coord.value -
                                            1][dir_idx][candidate_geo_coord.idx].freedom_index();
            if (candidate_freedom < min_freedom) {
               geo_coord_min_freedom = candidate_geo_coord;
               min_freedom = candidate_freedom;
            }
         }
      }
   }
   return geo_coord_min_freedom;
}

bool SudokuSolver::lock_possible_value(Coord coord, unsigned int val) {
   if (not tile(coord).lock_possibile_value(val, turn())) {
      return false;
   }

   bool should_propagate = false;
   unsigned int other_entry = _size;
   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (_matrix[coord.row_idx][idx].can_set_to(val)) {
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
      if (not set_value(Coord{coord.row_idx, other_entry}, val)) {
         return false;
      }
   }

   should_propagate = false;
   other_entry = _size;
   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (_matrix[idx][coord.col_idx].can_set_to(val)) {
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
      if (not set_value(Coord{other_entry, coord.col_idx}, val)) {
         return false;
      }
   }

   should_propagate = false;
   other_entry = _size;
   for (unsigned int idx = 0; idx != _size; ++idx) {
      if (_matrix[(coord.row_idx / _minisize) * _minisize + (idx / _minisize)][(coord.row_idx % _minisize) * _minisize +
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
      if (not set_value(Coord{(coord.row_idx / _minisize) * _minisize + (other_entry / _minisize),
                              (coord.row_idx % _minisize) * _minisize + (other_entry % _minisize)}, val)) {
         return false;
      }
   }

   return true;
}

void SudokuSolver::lock_all_geo_blocks(SudokuSolver::Coord coord, unsigned int value) {
   _geo_blocks[value - 1][0][coord.row_idx].lock_possible_value(coord.col_idx, turn());
   _geo_blocks[value - 1][1][coord.col_idx].lock_possible_value(coord.row_idx, turn());
   _geo_blocks[value - 1][2][(coord.row_idx / _minisize) * _minisize +
                             (coord.col_idx / _minisize)].lock_possible_value(
         (coord.row_idx % _minisize) * _minisize + (coord.col_idx % _minisize), turn());
}

bool SudokuSolver::is_positive_square(unsigned int n) {
   unsigned int sr = 1;
   while (n > sr * sr) {
      ++sr;
   }
   return n == sr * sr;
}

////////////////////////////////////////                  Tile                  ////////////////////////////////////////

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

////////////////////////////////////////                GeoBlock                ////////////////////////////////////////

SudokuSolver::GeoBlock::GeoBlock(SudokuSolver &ss, SudokuSolver::GeoDir dir_, unsigned int position_) :
      _elements(ss._size), _num_free{ss._size}, _dir{dir_}, _position{position_},
      _minisize{static_cast<unsigned int>(std::sqrt(ss._size))} {
   for (unsigned other_idx = 0; other_idx != ss._size; ++other_idx) {
      switch (dir_) {
         case ROW: {
            _elements[other_idx].tile = &ss.matrix()[position_][other_idx];
            break;
         }
         case COL: {
            _elements[other_idx].tile = &ss.matrix()[other_idx][position_];
            break;
         }
         case MINISQUARE: {
            _elements[other_idx].tile = &ss.matrix()[position_ / ss._minisize + other_idx / ss._minisize]
            [position_ % ss._minisize + other_idx % ss._minisize];
            break;
         }
      }
   }
}

bool SudokuSolver::GeoBlock::lock_possible_value(unsigned int idx, unsigned int turn) {
   if (idx >= _elements.size()) {
      throw std::logic_error("Cannot lock value in invalid index");
   }
   if (_elements[idx].turn == AVAILABLE) {
      _elements[idx].turn = turn;
      --_num_free;
      return true;
   }
   return false;
}

void SudokuSolver::GeoBlock::reset_from_turn(unsigned int turn) {
   for (auto &el : _elements) {
      if (el.turn >= turn) {
         el.turn = AVAILABLE;
         ++_num_free;
      }
   }
}

std::vector<SudokuSolver::Coord> SudokuSolver::GeoBlock::avaliable_coordinates() const {
   std::vector<SudokuSolver::Coord> output_vec;
   switch (_dir) {
      case ROW: {
         for (unsigned int inner_idx = 0; inner_idx != size(); ++inner_idx) {
            if (_elements[inner_idx].turn == AVAILABLE) {
               output_vec.emplace_back(Coord{_position, inner_idx});
            }
         }
         break;
      }
      case COL: {
         for (unsigned int inner_idx = 0; inner_idx != size(); ++inner_idx) {
            if (_elements[inner_idx].turn == AVAILABLE) {
               output_vec.emplace_back(Coord{inner_idx, _position});
            }
         }
         break;
      }
      case MINISQUARE: {
         for (unsigned int inner_idx = 0; inner_idx != size(); ++inner_idx) {
            if (_elements[inner_idx].turn == AVAILABLE) {
               output_vec.emplace_back(Coord{(_position / _minisize) * _minisize + (inner_idx / _minisize),
                                             (_position % _minisize) * _minisize + (inner_idx % _minisize)});
            }
         }
         break;
      }
   }
   return output_vec;
}

////////////////////////////////////////          non-member functions          ////////////////////////////////////////

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