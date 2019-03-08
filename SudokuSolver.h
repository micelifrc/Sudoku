//
// Created by mich on 07/03/19.
//

#ifndef SUDOKU_SUDOKUSOLVER_H
#define SUDOKU_SUDOKUSOLVER_H

#include <array>
#include <vector>
#include <fstream>
#include <cmath>

class SudokuSolver {
public:
   const static unsigned int MINISIZE = 3;
   const static unsigned int SIZE = MINISIZE * MINISIZE;

private:
   class Tile {
      static const int FREE = 0;
      static const int AVAILABLE = -1;
   public:
      Tile();

      bool set_to_value(unsigned int val, unsigned int turn);

      bool lock_possibile_value(unsigned int val, unsigned int turn);  // will return false if the locking is illegal

      bool is_fixed() const { return _value != FREE; }

      bool can_set_to(unsigned int value) const;

      int value() const { return _value; }

      unsigned int num_possibilities() const;

      unsigned int freedom_index() const { return is_fixed() ? SIZE + 1 : num_possibilities(); }

      void reset_from_turn(unsigned int turn);

      unsigned int first_choice_available() const;

   private:
      std::array<int, SIZE> _locking_turn;  // tells in which turn each possibility was locked. It is AVAILABLE if was never deleted
      unsigned int _value;  // the current value of the tile. FREE if the value is not set
   };

   typedef std::array<std::array<Tile, SIZE>, SIZE> Matrix;
   typedef std::pair<int, int> Coord;

public:
   explicit SudokuSolver(std::ifstream &input_file);

   bool solve();  // solves the input sudoku

   const Matrix &matrix() const { return _matrix; }

   bool is_solvable() const { return _is_solvable; }

private:

   bool set_value(Coord coord, unsigned int value);

   bool guess();

   void remove_guess();

   Coord free_tile_with_smaller_freedom() const;

   unsigned int turn() const { return static_cast<unsigned int>(_guesses.size()); }

   Matrix _matrix;
   std::vector<Coord> _guesses;
   int _num_free_tiles;
   bool _is_solvable;
};

std::ostream &operator<<(std::ostream &os, const SudokuSolver &sudoku);

#endif //SUDOKU_SUDOKUSOLVER_H
