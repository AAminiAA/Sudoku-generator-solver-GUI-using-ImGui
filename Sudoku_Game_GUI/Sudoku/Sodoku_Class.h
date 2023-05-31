#pragma once
#include <cstdlib>
#include <ctime>
#include <vector>
#include <array>

using num_t = uint16_t;
using Suduko_table = std::array < std::array<num_t, 9>, 9>;
using Suduko_Notes = std::array < std::array <std::vector<num_t> , 9> ,9>;


namespace Sudoku {

	struct Pos
	{
		uint8_t i;
		uint8_t j;

		bool operator==(const Pos _p) { return (i == _p.i && j == _p.j); }
	};

	enum game_difficulty
	{
		Simple,
		Easy,
		Normal,
		Hard,
		Expert,
		Seed	//Lowest starting point used in expert
	};

	namespace functions
	{
		void Rotate(Suduko_table& table);
		void Rotate_one_pos(Suduko_table& table, Sudoku::Pos pos, num_t pos_val);
		void Swap_rows(Suduko_table& table, num_t row1, num_t row2);
		void Swap_cols(Suduko_table& table, num_t col1, num_t col2);
	}
}


class Sodoku_Class
{
public:
	Sodoku_Class();
	bool Find_Solvable(Sudoku::game_difficulty diff);
	bool Solve();
	bool Is_Solved();
	unsigned Remaining();

	void Clear();
	void Clear_Notes();

	bool New_Game(Sudoku::game_difficulty __diff);	//Generating
	bool New_Game(Suduko_table& seed, Suduko_table& solved, Sudoku::game_difficulty __diff);	//From a solved seed

	void Update_Notes(const unsigned row, const unsigned col, const unsigned num);
	inline Sudoku::game_difficulty Get_difficulty() { return _difficulty; }
	inline void Set_difficulty(Sudoku::game_difficulty _diff) {  _difficulty = _diff; }

	//Access Grid and Notes and Solved table
	inline std::array<num_t, 9>& at(const size_t index) { return Grid.at(index); }
	inline std::array<num_t, 9>& at_solved(const size_t index) { return _Solved_Grid.at(index); }
	inline std::array<num_t, 9>& at_seed(const size_t index) { return __Seed.at(index); }
	inline std::array <std::vector<num_t>, 9>& at_notes(const size_t index) { return __Notes.at(index); }

	~Sodoku_Class() = default;
private:
	bool __Is_Solved();	//This one is for computer and checkes _Solved_Grid

	//Solving teqniques
	//Last possible number
	bool Last_possible_number();
	bool Last_possible_solve(const unsigned row, const unsigned col);
	//Last remaining cell
	bool Last_remaining_number();
	bool Last_remaining_solve(const unsigned row, const unsigned col);
	//Check the available positions for a number in a single row or columb
	bool Single_row_col_check();
	bool Single_row_col_solve(const uint8_t index, const bool is_row);

	//Notes for hard and above
	void Take_notes();

	bool obvious_singles();

	bool obvious_pairs();
	bool obvious_pairs_solve(const unsigned row, const unsigned col);

	//if a number only mentioned in a row or column of a block, no other cells can be that number
	bool row_col_reserved();
	bool row_col_reserved_solve(const unsigned row, const unsigned col);

	//Hidden pairs
	bool hidden_pairs();
	bool hidden_pairs_solve(const unsigned row, const unsigned col);

private:
	
	bool check_row(const unsigned row, const unsigned num);
	bool check_col(const unsigned col, const unsigned num);
	bool check_block(const unsigned row, const unsigned col, const unsigned num);

	inline bool check_same_row(const unsigned index1, const unsigned index2) {
		if (index1 / 3 == index2 / 3) {
			return true;
		}
		return false;
	}
	inline bool check_same_col(const unsigned index1, const unsigned index2) {
		if (index1 % 3 == index2 % 3) {
			return true;
		}
		return false;
	}
	inline bool check_same_block(const Sudoku::Pos pos1, const Sudoku::Pos pos2) {
		if (pos1.i / 3 == pos2.i / 3 && pos1.j / 3 == pos2.j / 3) {
			return true;
		}
		return false;
	}
	
	bool is_solvable();
	void random_number_random_pos();

private:
	Suduko_table __Seed;
	Suduko_table Grid;
	Suduko_table _Solved_Grid;
	Suduko_Notes __Notes;

	Sudoku::game_difficulty _difficulty;
private:
	//Starting Numbers for Simple, Easy , Normal, Hard
	static constexpr unsigned starting_numbers[4] { 45, 38, 30, 24 };
};

