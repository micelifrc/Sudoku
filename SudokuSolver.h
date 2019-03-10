//
// Created by mich on 07/03/19.
// class SudokuSolver
//

#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <fstream>
#include <cmath>
#include <array>
#include <vector>

// This class will represent a sudoku of arbitrary size
class SudokuSolver {
   class Tile;

   class GeoBlock;

   typedef std::vector<std::vector<Tile>> Matrix;  // The Sudoku matrix

   enum GeoDir : unsigned int {
      ROW = 0, COL = 1, REGION = 2
   };
public:
   static const unsigned int FREE;  // denotes the tile doesn't have a value yet
   static const int AVAILABLE;  // denotes a possible value for the tile is still available

   struct Coord {
      unsigned int row_idx, col_idx;

      explicit Coord(unsigned int row_idx_ = 0, unsigned int col_idx_ = 0) : row_idx{row_idx_}, col_idx{col_idx_} {}
   };

   struct GeoCoord {
      unsigned int value;
      GeoDir dir;
      unsigned int idx;

      GeoCoord(unsigned int value_, unsigned int dir_, unsigned int idx_) : value{value_}, dir{dir_}, idx{idx_} {}
   };

   // @p input_file is a file from which to read the sudoku
   explicit SudokuSolver(std::ifstream &input_file);

   // solves the input sudoku
   // Returns true if the sudoku was solved successfully, or false if it failed (meaning there are no solutions)
   bool solve() { return _is_solvable and guess(); }

   // Checks if the current solution is legal (this should be redundant, but it is a security check)
   bool has_legal_solution() const;

   const Matrix &matrix() const { return _matrix; }

   unsigned int get_region_size() const { return _region_size; }

   unsigned int get_size() const { return _size; }

private:
   // The procedures called by the constructor
   void constructor_function(std::ifstream &input_file);

   // Set a required value @p val in the entry of _matrix at coordinated @p coord
   // This will call set_value for subsequent tiles that are forced by this set_value call
   // Returns true if set_value added a legal value (also considering the propagation)
   // NOTE this will effect also _num_free_tiles.
   // NOTE the value will be set in the tile also if its propagation brings to conflicts
   bool set_value(Coord coord, unsigned int value);

   // Guesses a value for a free tile. Only called when all propagation are done.
   // This will effect both _guesses_list and the entries of the matrix
   bool guess();

   // Remove the last guess (deleting all the locks)
   void remove_guess();

   // the tile with less freedom among those that are free
   Coord free_tile_with_smaller_freedom() const;

   // the geometric block with smaller freedom among those that are free
   GeoCoord free_geo_block_with_smaller_freedom() const;

   // locks @p val in the tile in coordinate @p coord
   // returns false if the locking brings to unfeasible solution
   bool lock_possible_value(Coord coord, unsigned int val);

   // locks all the points in _geo_blocks with coordinates @p cd and required @p value
   void lock_all_geo_blocks(Coord coord, unsigned int value);

   // the current turn
   unsigned int turn() const { return static_cast<unsigned int>(_guesses_list.size()); }

   const Tile &tile(Coord cd) const { return _matrix[cd.row_idx][cd.col_idx]; }

   Tile &tile(Coord cd) { return const_cast<Tile &>(static_cast<const SudokuSolver &>(*this).tile(cd)); }

   const GeoBlock &geo_block(GeoCoord cd) const { return _geo_blocks[cd.value - 1][cd.dir][cd.idx]; }

   GeoBlock &geo_block(GeoCoord cd) {
      return const_cast<GeoBlock &>(static_cast<const SudokuSolver &>(*this).geo_block(cd));
   }

   // The two indices with respect to the subdivision of the grid in regions
   std::pair<unsigned int, unsigned int> region_indices(Coord coord) const {
      return std::pair<unsigned int, unsigned int>{
            (coord.row_idx / _region_size) * _region_size + (coord.col_idx / _region_size),
            (coord.row_idx % _region_size) * _region_size + (coord.col_idx % _region_size)};
   }

   // Read the numbers in input file, and records them in the output vector
   static std::vector<unsigned int> read_input_file(std::ifstream &input_file);

   // Tell if the input number @p n is a perfect square and n != 0
   static bool is_positive_square(unsigned int n);

   Matrix _matrix;  // The matrix of Tiles. Represents the Sudoku matrix
   std::vector<std::array<std::vector<GeoBlock>, 3>> _geo_blocks;
   unsigned int _num_free_tiles;  // The number of tiles for which the value has not been fixed yet
   std::vector<Coord> _guesses_list;  // A list of the coordinates where we made "active" guesses. The list is sorted
   bool _is_solvable;  // False if we proved there is no solution for the puzzle
   unsigned int _region_size;  // The length of a small tile (usually 3)
   unsigned int _size;  // The length of the matrix (usually 9)
};

class SudokuSolver::Tile {
public:
   explicit Tile(unsigned int grid_size_ = 0);

   // Set the tile to the required value, at time @p turn
   // The operation fails if the tile already contains a value, or if @p val is locked
   // Returns true iff the @p val was set successfully
   bool set_to_value(unsigned int val, unsigned int turn);

   // locks the required @p val at time @p turn
   // it will fail if _value == @p val, or if this would lock all possibilities
   bool lock_possible_value(unsigned int val, unsigned int turn);

   // tells if the tile contains a value
   bool is_fixed() const { return _value != FREE; }

   // tells if the tile can be set to the required value. In particular, it will succeed if _value == @p val
   bool can_set_to(unsigned int value) const;

   // the current value
   int value() const { return _value; }

   // tells if the contained value is legal
   bool has_legal_value() const {
      return _value > FREE and _value <= _grid_size;
   }

   // The number of free possibilities for the tile. It is 1 if _value is fixed
   unsigned int num_possibilities() const;

   // An index used for choosing which tile to guess.
   // Big if the _value is fixed already, or the number of free possibilities of it is free
   unsigned int freedom_index() const {
      return is_fixed() ? std::numeric_limits<unsigned int>::max() : num_possibilities();
   }

   // Removes all the locks (and eventually _value) fixed in time >= @p turn
   // Returns true if this removes a value from _value
   bool reset_from_turn(unsigned int turn);

   // The smaller value for which the tile is not locked.
   // Will throw if no tile is available (for example because _value is set)
   unsigned int first_choice_available() const;

   bool get_from_input() const { return _from_input; }

   void set_from_input(bool from_input_) { _from_input = from_input_; }

   bool get_is_conflictual() const { return _is_conflictual; }

   void set_is_conflictual(bool is_conflictual_) { _is_conflictual = is_conflictual_; }

private:
   std::vector<int> _locking_turn;  // tells in which turn each possibility was locked. It is AVAILABLE if was never deleted. Will have size = _grid_size
   unsigned int _value;  // the current value of the tile. FREE if the value is not set
   bool _from_input;  // tells if the tile was fixed as input
   bool _is_conflictual;  // tells if the tile created a conflict at time 0 (making the puzzle impossible)
   unsigned int _grid_size;  // the size of the grid (usually 9)
};

// A geometric block in the grid (of one GeoDir) for a fixed value
class SudokuSolver::GeoBlock {
public:
   GeoBlock(SudokuSolver &ss, GeoDir dir_, unsigned int position_);

   // locks the represented value of entry with index @p idx at required @p turn
   bool lock_possible_value(unsigned int idx, unsigned int turn);

   // remove all locks at time >= @p turn
   void reset_from_turn(unsigned int turn);

   // Represent the level of freedom for the geometric block
   unsigned int freedom_index() const {
      return _num_free == 0 ? std::numeric_limits<unsigned int>::max() : _num_free;
   }

   // The coordinates of tiles in the geometric block that are still free for the represented value
   std::vector<Coord> available_coordinates() const;

   // The number of tiles in the geometric block
   unsigned int size() const { return static_cast<unsigned int>(_elements.size()); }

private:
   // a tile, together with the time in which the represented value was locked
   struct Element {
      const Tile *tile;
      int turn;

      explicit Element(Tile *tile_ = nullptr, int turn_ = AVAILABLE) : tile{tile_}, turn{turn_} {}
   };

   std::vector<Element> _elements;  // all the tiles in the geometric block
   unsigned int _num_free;  // the number of tiles that can take the represented value
   GeoDir _dir;  // the direction of the geometric block
   unsigned int _position;  // the position of the geometric block (a number in [0, _size))
   unsigned int _region_size;  // the minsize of the associated SudokuSolver
};

std::ostream &operator<<(std::ostream &os, const SudokuSolver &sudoku);

#endif //SUDOKU_SUDOKUSOLVER_H