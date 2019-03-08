//
// Created by mich on 07/03/19.
// Template class SudokuSolver
//

#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <array>
#include <vector>
#include <fstream>
#include <cmath>

// This class will represent a sudoku of arbitrary size
// NOTE if you use MINISIZE > 3, you should think of a different encoding for the numbers.
// This would require to update the constructor and the operator<<
template<unsigned int MINISIZE = 3>
class SudokuSolver {
public:
   const static unsigned int SIZE = MINISIZE * MINISIZE;  // the size of an edge. Usually == 9

private:
   class Tile {
      static const int FREE = 0;  // denotes the tile doesn't have a value yet
      static const int AVAILABLE = -1;  // denotes a possible value for the tile is still available
   public:
      Tile();

      // Set the tile to the required value, at time @p turn
      // The operation fails if the tile already contains a value, or if @p val is locked
      // Returns true iff the @p val was set successfully
      bool set_to_value(unsigned int val, unsigned int turn);

      // locks the required @p val at time @p turn
      // it will fail if _value == @p val, or if this would lock all possibilities
      bool lock_possibile_value(unsigned int val, unsigned int turn);

      bool is_fixed() const { return _value != FREE; }  // tells if the tile contains a value

      // tells if the tile can be set to the required value. In particular, it will succeed if _value == @p val
      bool can_set_to(unsigned int value) const;

      int value() const { return _value; }  // the current value

      bool has_legal_value() const {
         return _value > FREE and _value <= SIZE;
      }  // tells if the contained value is legal

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
      std::array<int, SIZE> _locking_turn;  // tells in which turn each possibility was locked. It is AVAILABLE if was never deleted
      unsigned int _value;  // the current value of the tile. FREE if the value is not set
      bool _from_input;  // tells if the tile was fixed as input
      bool _is_conflictual;  // tells if the tile created a conflict at time 0 (making the puzzle impossible)
   };

   typedef std::array<std::array<Tile, SIZE>, SIZE> Matrix;  // The sudoku matrix
   typedef std::pair<int, int> Coord;  // two coordinates for a sudoku entry in the matrix

public:
   // @p input_file is a file from which to read the sudoku
   // currently specific for the case MINISIZE == 3
   explicit SudokuSolver(std::ifstream &input_file);

   // solves the input sudoku
   // Returns true if the sudoku was solved successfully, or false if it failed (meaning there are no solutions)
   bool solve() { return _is_solvable and guess(); }

   // Checks if the current solution is legal (this should be redundant, but it is a security check)
   bool has_legal_solution() const;

   const Matrix &matrix() const { return _matrix; }

   bool is_solvable() const { return _is_solvable; }

private:

   // Set a required value @p val in the entry of _matrix at coordinated @p coord
   // This will call set_value for subsequent tiles that are forced by this set_value call
   // Returns true if set_value added a legal value (also considering the propagation)
   // NOTE this will effect also _num_free_tiles.
   // NOTE the value will be set in the tile also if its propagation brings to conflicts
   bool set_value(Coord coord, unsigned int value);

   // Guesses a value for a free tile. Only called when all propagations are done.
   // This will effect both _guesses_list and the entries of the matrix
   bool guess();

   // Remove the last guess (deleting all
   void remove_guess();

   // the tile with less freedom among those that are free
   Coord free_tile_with_smaller_freedom() const;

   // locks @p val in the tile in coordinate @p coord
   // returns false if the locking brings to unfeasable solution
   bool lock_possible_value(Coord coord, unsigned int val);

   // the current turn
   unsigned int turn() const { return static_cast<unsigned int>(_guesses_list.size()); }

   Matrix _matrix;  // The matrix of Tiles. Represents the Sudoku matrix
   int _num_free_tiles;  // The number of tiles for which the value has not been fixed yet
   std::vector<Coord> _guesses_list;  // A list of the coordinates where we made "active" guesses. The list is sorted
   bool _is_solvable;  // False if we proved there is no solution for the puzzle
};

// currently specific for the case MINISIZE == 3
template<unsigned int MINISIZE>
std::ostream &operator<<(std::ostream &os, const SudokuSolver<MINISIZE> &sudoku);

#include "SudokuSolver.tpp"

#endif //SUDOKU_SUDOKUSOLVER_H
