#include "Sodoku_Class.h"
#include <algorithm>

namespace Sudoku {
	namespace variables
	{
		static bool notes_are_uptodate{ false };
	}
}
using namespace Sudoku;


Sodoku_Class::Sodoku_Class()
	:__Seed{}, Grid{}, _Solved_Grid{}, __Notes{}, _difficulty{Easy}
{
	srand(time(NULL));
}


bool Sodoku_Class::__Is_Solved() {	//For solve function checks _Solve_Grid
	for (auto& row : _Solved_Grid) {
		for (auto& num : row) {
			if (num == 0)
				return false;
		}
	}
	return true;
}

unsigned Sodoku_Class::Remaining() {
	unsigned rem{};
	for (unsigned i{}; i < 9; i++) {
		for (unsigned j{}; j < 9; j++) {
			//a cell is remaining if its 0 or it has a wrong number in it
			if (Grid[i][j] == 0 || (Grid[i][j] != 0 && Grid[i][j] != _Solved_Grid[i][j]) )
				++rem;
		}
	}
	return rem;
}

bool Sodoku_Class::Is_Solved() {	//For in game checks Grid
	for (unsigned i{}; i < 9; i++) {
		for (unsigned j{}; j < 9; j++) {
			//a cell is remaining if its 0 or it has a wrong number in it
			if (Grid[i][j] == 0 || (Grid[i][j] != 0 && Grid[i][j] != _Solved_Grid[i][j]))
				return false;
		}
	}
	return true;
}

bool Sodoku_Class::Solve() {
	bool Solved_one{ false };
	bool solved{ false };

	static bool solve_second_call{ false };

	bool solving_method_called[4]{};

	while (!solved) {
		if (!variables::notes_are_uptodate) {
			Take_notes();
			variables::notes_are_uptodate = true;
		}
		if (!is_solvable()) {
			variables::notes_are_uptodate = false;
			return false;
		}
			
		if (Last_remaining_number())
			Solved_one = true;
		if (obvious_singles())
			Solved_one = true;

		if (Solved_one) {
			solving_method_called[Easy] = true;
			Solved_one = false;
			continue;
		}

		if (_difficulty >= Normal) {
			if (Single_row_col_check())
				Solved_one = true;

			if (obvious_pairs()) {
				Solved_one = true;
			}
		}

		if (Solved_one) {
			solving_method_called[Normal] = true;
			Solved_one = false;
			continue;
		}

		if (_difficulty >= Hard) {
			if (row_col_reserved())
				Solved_one = true;
			if (hidden_pairs())
				Solved_one = true;
		}

		if (Solved_one) {
			solving_method_called[Hard] = true;
			Solved_one = false;
			continue;
		}

		solved = __Is_Solved();
		if (solved) {
			variables::notes_are_uptodate = false;

			if (!solving_method_called[_difficulty])	//if one of the intended methods called at least one, this will pass
				return false;

			return true;
		}
		else if (!Solved_one) {

			variables::notes_are_uptodate = false;

			return false;
		}
			
		Solved_one = false;
	}

	return false;
}

bool Sodoku_Class::Last_possible_number()
{
	bool Solved_one{ false };
	for (unsigned i{}; i < 9; i++) {
		for (unsigned j{}; j < 9; j++) {
			if (_Solved_Grid[i][j] != 0)
				continue;
			if (Last_possible_solve(i, j)) {
				Solved_one = true;
			}
		}
	}
	return Solved_one;
}

bool Sodoku_Class::Last_possible_solve(const unsigned row, const unsigned col)
{
	std::vector<unsigned> possible_numbers{ 1,2,3,4,5,6,7,8,9 };
	for (auto it = possible_numbers.begin(); it != possible_numbers.end(); ) {
		if (check_row(row, *it)) {
			it = possible_numbers.erase(it);
			continue;
		}
		if (check_col(col, *it)) {
			it = possible_numbers.erase(it);
			continue;
		}
		if (check_block(row, col, *it)) {
			it = possible_numbers.erase(it);
			continue;
		}
		++it;
	}

	if (possible_numbers.size() == 1)
	{
		_Solved_Grid[row][col] = possible_numbers.front();

		Update_Notes(row, col, possible_numbers.front());
		__Notes[row][col].clear();
		__Notes[row][col].shrink_to_fit();

		return true;
	}

	return false;
}

bool Sodoku_Class::Last_remaining_number()
{
	bool Solved_one{ false };
	for (unsigned i{}; i < 9; i += 3) {
		for (unsigned j{}; j < 9; j += 3) {
			if (Last_remaining_solve(i, j))
				Solved_one = true;
		}
	}

	return Solved_one;
}

bool Sodoku_Class::Last_remaining_solve(const unsigned row, const unsigned col) {
	bool solved_one{ false };
	const unsigned Not_avalble_number{ 0 };
	const unsigned Not_avalble_position{ 69 };
	std::vector<unsigned> possible_numbers{ 1,2,3,4,5,6,7,8,9 };
	std::vector<unsigned> possible_positions{ 0,1,2,3,4,5,6,7,8 };

	std::array<std::array<unsigned, 3>, 3> block{};

	//Creating Block array
	for (unsigned i{ row }, block_i{}; block_i < 3; i++, block_i++) {
		for (unsigned j{ col }, block_j{}; block_j < 3; j++, block_j++) {
			block[block_i][block_j] = _Solved_Grid[i][j];
		}
	}

	//Mark unavailable positions and numbers
	for (unsigned i{}; i < 3; i++) {
		for (unsigned j{}; j < 3; j++) {
			if (block[i][j] != 0) {
				possible_numbers.at(block[i][j] - 1) = Not_avalble_number;
				possible_positions.at(3 * i + j) = Not_avalble_position;
			}
		}
	}

	//Delete unavailable positions and numbers
	for (auto it = possible_numbers.begin(); it != possible_numbers.end(); ) {
		if (*it == Not_avalble_number) {
			it = possible_numbers.erase(it);
			continue;
		}
		++it;
	}
	for (auto it = possible_positions.begin(); it != possible_positions.end(); ) {
		if (*it == Not_avalble_position) {
			it = possible_positions.erase(it);
			continue;
		}
		++it;
	}

	for (auto num : possible_numbers) {
		std::vector<unsigned> temp_pos{ possible_positions };
		//Checking which row of block is available for num
		for (unsigned i{}; i < 3; i++) {
			//See if num is present in this row somewhere else, if is:
			if (check_row(row + i, num)) {
				//delete every available position in that row
				for (auto it = temp_pos.begin(); it != temp_pos.end(); ) {
					if (*it / 3 == i) {
						it = temp_pos.erase(it);
						continue;
					}
					++it;
				}
			}
		}

		//Checking which column of block is available for num
		for (unsigned i{}; i < 3; i++) {
			//See if num is present in this row somewhere else, if is:
			if (check_col(col + i, num)) {
				//delete every available position in that columb
				for (auto it = temp_pos.begin(); it != temp_pos.end(); ) {
					if (*it % 3 == i) {
						it = temp_pos.erase(it);
						continue;
					}
					++it;
				}
			}
		}

		if (temp_pos.size() == 1) {
			uint8_t i = row + temp_pos.front() / 3;
			uint8_t j = col + temp_pos.front() % 3;

			_Solved_Grid.at(i).at(j) = num;

			Update_Notes(i, j, num);
			__Notes[i][j].clear();
			__Notes[i][j].shrink_to_fit();

			solved_one = true;
		}
	}
	return solved_one;
}

bool Sodoku_Class::Single_row_col_check() {
	bool Solved_one{ false };
	for (unsigned row{}; row < 9; row++) {
		if (Single_row_col_solve(row, true)) {	//row
			Solved_one = true;
		}
		if (Single_row_col_solve(row, false)) {	//col
			Solved_one = true;
		}
	}
	return Solved_one;
}

bool Sodoku_Class::Single_row_col_solve(const uint8_t index, const bool is_row) {
	bool solved_one{ false };
	const unsigned Not_avalble_number{ 0 };
	std::vector<uint8_t> possible_numbers{ 1,2,3,4,5,6,7,8,9 };
	std::vector<uint8_t> possible_positions{ 0,1,2,3,4,5,6,7,8 };

	if (is_row) {
		//Delete unavailable positions and Mark unavailable numbers
		auto it_p = possible_positions.begin();
		for (unsigned i{}; it_p != possible_positions.end(); i++) {
			if (_Solved_Grid[index][*it_p] != 0) {
				possible_numbers.at(_Solved_Grid[index][i] - 1) = Not_avalble_number;	//Mark
				it_p = possible_positions.erase(it_p);	//Delete
				continue;
			}
			++it_p;
		}
		//Delete unavailable numbers
		for (auto it = possible_numbers.begin(); it != possible_numbers.end(); ) {
			if (*it == Not_avalble_number) {
				it = possible_numbers.erase(it);
				continue;
			}
			++it;
		}

		//Meaning the row is done
		if (possible_positions.empty()) {
			return false;
		}

		//Going through all possible numbers and check how many slots are available to them
		for (auto num : possible_numbers) {
			std::vector<uint8_t> temp_pos{ possible_positions };

			//Going through all possible slots for num
			for (auto it = temp_pos.begin(); it != temp_pos.end(); ) {
				//Check if columb already has num or not
				if (check_col(*it, num)) {
					//delete position if the columb already has num
					it = temp_pos.erase(it);
					continue;
				}

				//Check if the block already has num or not
				auto temp_it = it;
				if (check_block(index, *it, num)) {
					Pos pos{ index, *it };
					//delete all positions in the same block if the block already has num
					for (; it != temp_pos.end();) {
						if (check_same_block(pos, Pos{ index , *it })) {
							it = temp_pos.erase(it);
							temp_it = it;
							continue;
						}
						++it;
					}
				}
				//after deleting all positions in the same block, 'it' could be temp_pos.end() 
				//Meaning deleted position at end, and should not be incremented after
				if (temp_it == temp_pos.end())
					break;
				it = temp_it;
				++it;
			}

			if (temp_pos.size() == 1) {
				_Solved_Grid.at(index).at(temp_pos.front()) = num;

				Update_Notes(index, temp_pos.front(), num);
				__Notes[index][temp_pos.front()].clear();
				__Notes[index][temp_pos.front()].shrink_to_fit();

				solved_one = true;
			}
		}
	}
	else {	//is_row == false
		//Delete unavailable positions and Mark unavailable numbers
		auto it_p = possible_positions.begin();
		for (unsigned i{}; it_p != possible_positions.end(); i++) {
			if (_Solved_Grid[*it_p][index] != 0) {
				possible_numbers.at(_Solved_Grid[i][index] - 1) = Not_avalble_number;	//Mark
				it_p = possible_positions.erase(it_p);	//Delete
				continue;
			}
			++it_p;
		}
		//Delete unavailable numbers
		for (auto it = possible_numbers.begin(); it != possible_numbers.end(); ) {
			if (*it == Not_avalble_number) {
				it = possible_numbers.erase(it);
				continue;
			}
			++it;
		}

		//Meaning the columb is done
		if (possible_positions.empty()) {
			return false;
		}

		//Going through all possible numbers and check how many slots are available to them
		for (auto num : possible_numbers) {
			std::vector<uint8_t> temp_pos{ possible_positions };

			//Going through all possible slots for num
			for (auto it = temp_pos.begin(); it != temp_pos.end(); ) {

				//Check if the block already has num or not
				auto temp_it = it;
				if (check_block(*it, index, num)) {
					//delete all positions in the same block if the block already has num
					Pos pos{ *it, index };
					for (; it != temp_pos.end();) {
						if (check_same_block(pos, Pos{ *it, index })) {
							it = temp_pos.erase(it);
							temp_it = it;
							continue;
						}
						++it;
					}
				}

				//after deleting all positions in the same block, 'it' could be temp_pos.end() 
				//Meaning deleted position at end, and should not be incremented after
				if (temp_it == temp_pos.end())
					break;

				//Check if columb already has num or not
				if (check_row(*temp_it, num)) {
					//delete position if the row already has num
					temp_it = temp_pos.erase(temp_it);
					it = temp_it;
					continue;
				}

				it = temp_it;
				++it;
			}

			if (temp_pos.size() == 1) {
				_Solved_Grid.at(temp_pos.front()).at(index) = num;

				Update_Notes(temp_pos.front(), index, num);
				__Notes[temp_pos.front()][index].clear();
				__Notes[temp_pos.front()][index].shrink_to_fit();

				solved_one = true;
			}
		}
	}
	return solved_one;
}

bool Sodoku_Class::check_row(const unsigned row, const unsigned num) {
	bool __is_solved = __Is_Solved();
	if (!__is_solved) {	//Not solved yet
		//if finds returns true else return false
		if (std::find(_Solved_Grid.at(row).begin(), _Solved_Grid.at(row).end(), num) != _Solved_Grid.at(row).end()) {
			return true;
		}
		else {
			return false;
		}
	}
	else {
		if (std::find(Grid.at(row).begin(), Grid.at(row).end(), num) != Grid.at(row).end()) {
			return true;
		}
		else {
			return false;
		}
	}
}

bool Sodoku_Class::check_col(const unsigned col, const unsigned num) {
	std::array<unsigned, 9> col_array{};
	bool __is_solved = __Is_Solved();
	if (!__is_solved) {	//Not solved yet
		for (unsigned i{}; i < 9; i++) {
			col_array[i] = _Solved_Grid[i][col];
		}
	}
	else {
		for (unsigned i{}; i < 9; i++) {
			col_array[i] = Grid[i][col];
		}
	}

	if (std::find(col_array.begin(), col_array.end(), num) != col_array.end()) {
		return true;
	}
	else {
		return false;
	}
}

bool Sodoku_Class::check_block(const unsigned row, const unsigned col, const unsigned num) {
	unsigned start_row{ (row / 3) * 3 };
	unsigned start_col{ (col / 3) * 3 };

	bool __is_solved = __Is_Solved();
	if (!__is_solved) {	//Not solved yet
		for (unsigned i{ start_row }; i < start_row + 3; i++) {
			for (unsigned j{ start_col }; j < start_col + 3; j++) {
				if (_Solved_Grid[i][j] == num) {
					return true;
				}
			}
		}
	}
	else {
		for (unsigned i{ start_row }; i < start_row + 3; i++) {
			for (unsigned j{ start_col }; j < start_col + 3; j++) {
				if (Grid[i][j] == num) {
					return true;
				}
			}
		}
	}
	//this->Show_Table();
	return false;
}

void Sodoku_Class::Take_notes() {
	//Clear notes first
	for (unsigned i{}; i < 9; i++) {
		for (unsigned j{}; j < 9; j++) {
			__Notes[i][j].clear();
		}
	}
	//Fill notes
	bool __is_solved = __Is_Solved();
	for (unsigned i{}; i < 9; i++) {
		for (unsigned j{}; j < 9; j++) {
			//if we are solving take notes of _Solved_Grid, if it is solved and user is solving take from Grid
			if (!__is_solved) {
				if (_Solved_Grid[i][j] == 0) {	//Take notes one non accupied cells
					for (unsigned num{ 1 }; num < 10; num++) {

						if (!check_row(i, num) && !check_col(j, num) && !check_block(i, j, num)) {
							__Notes.at(i).at(j).push_back(num);
						}

					}
				}
			}
			else {
				if (Grid[i][j] == 0) {	//Take notes one non accupied cells
					for (unsigned num{ 1 }; num < 10; num++) {

						if (!check_row(i, num) && !check_col(j, num) && !check_block(i, j, num)) {
							__Notes.at(i).at(j).push_back(num);
						}

					}
				}
			}
		}
	}
}

void Sodoku_Class::Update_Notes(const unsigned row, const unsigned col, const unsigned num) {
	__Notes[row][col].clear();	//Delete Notes in the block
	//Deleting in row
	for (unsigned c{}; c < 9; c++) {
		if (Grid[row][c] == 0) {
			auto it = std::find(__Notes[row][c].begin(), __Notes[row][c].end(), num);
			if (it != __Notes[row][c].end()) {
				__Notes[row][c].erase(it);
			}
		}
	}
	//Deleting in columb
	for (unsigned r{}; r < 9; r++) {
		if (Grid[r][col] == 0) {
			auto it = std::find(__Notes[r][col].begin(), __Notes[r][col].end(), num);
			if (it != __Notes[r][col].end()) {
				__Notes[r][col].erase(it);
			}
		}
	}

	//Deleting in columb
	unsigned start_row{ (row / 3) * 3 };
	unsigned start_col{ (col / 3) * 3 };
	for (unsigned i{ start_row }; i < start_row + 3; i++) {
		for (unsigned j{ start_col }; j < start_col + 3; j++) {
			if (Grid[i][j] == 0) {
				auto it = std::find(__Notes[i][j].begin(), __Notes[i][j].end(), num);
				if (it != __Notes[i][j].end()) {
					__Notes[i][j].erase(it);
				}
			}
		}
	}
}

bool Sodoku_Class::obvious_pairs() {
	bool Solved_one{ false };
	for (unsigned i{}; i < 9; i += 3) {
		for (unsigned j{}; j < 9; j += 3) {
			if (obvious_pairs_solve(i, j))
				Solved_one = true;
		}
	}

	return Solved_one;
}

bool Sodoku_Class::obvious_pairs_solve(const unsigned row, const unsigned col) {
	std::vector<std::vector<num_t>> notes;
	for (unsigned i{}; i < 3; i++) {
		for (unsigned j{}; j < 3; j++) {
			if (__Notes[row + i][col + j].size() == 2) {
				notes.push_back(__Notes[row + i][col + j]);
				//putting the position of found pair at the end
				notes.back().push_back(3 * i + j);
			}
		}
	}

	if (notes.size() < 2) {	//0, 1 are useless
		return false;
	}
	for (size_t i{}; i < notes.size(); i++) {
		std::sort(notes[i].begin(), notes[i].end() - 1);	//Not counting last element in sort
	}
	unsigned num1{}, num2{}, pos1{}, pos2{};
	for (size_t i{}; i < notes.size(); i++) {
		for (size_t j{ i + 1 }; j < notes.size(); j++) {
			if (std::equal(notes[i].begin(), notes[i].end() - 1, notes[j].begin())) {	//Not counting last element in equality check
				num1 = notes[i][0];
				num2 = notes[i][1];
				pos1 = notes[i][2];
				pos2 = notes[j][2];
			}
		}
	}

	bool solved_one{ false };

	for (unsigned i{}; i < 3; i++) {
		for (unsigned j{}; j < 3; j++) {
			if (i * 3 + j != pos1 && i * 3 + j != pos2) {
				std::vector<num_t>& _n = __Notes[row + i][col + j];
				for (auto it = _n.begin(); it != _n.end(); ) {
					if (*it == num1 || *it == num2) {
						it = _n.erase(it);
						solved_one = true;
						continue;
					}
					++it;
				}
			}
		}
	}
	return solved_one;
}

bool Sodoku_Class::row_col_reserved() {
	bool Solved_one{ false };
	for (unsigned i{}; i < 9; i += 3) {
		for (unsigned j{}; j < 9; j += 3) {
			if (row_col_reserved_solve(i, j))
				Solved_one = true;
		}
	}

	return Solved_one;
}

bool Sodoku_Class::row_col_reserved_solve(const unsigned row, const unsigned col) {
	bool solved_one{ false };
	//Check if a number only mentiond in single row
	std::vector<std::vector<unsigned>> rows_notes{};
	for (size_t i{}; i < 3; i++) {
		std::vector<unsigned> row_notes{};
		for (size_t j{}; j < 3; j++) {

			if (_Solved_Grid[row + i][col + j] == 0) {
				for (auto num : __Notes[row + i][col + j]) {
					if (std::find(row_notes.begin(), row_notes.end(), num) == row_notes.end()) {
						row_notes.push_back(num);
					}
				}
			}
		}
		rows_notes.push_back(row_notes);
	}

	//Check row[0] wiht row[1] & row[2]
	//Check row[1] wiht row[0] & row[2]
	//Check row[2] wiht row[0] & row[1]
	for (size_t ref{}; ref < 3; ref++) {
		//at first iteration row[0] will get empty so at second iteration only row[1] and row[2] need comparison
		unsigned other1{}, other2{};
		if (ref == 0) {
			other1 = 1;
			other2 = 2;
		}
		else if (ref == 1) {
			other1 = 0;
			other2 = 2;
		}
		else {
			other1 = 0;
			other2 = 1;
		}
		for ( ; !rows_notes[ref].empty(); ) {
			//Check row[ref] wiht row[other1] & row[other2]
			//in this part always first element of row[ref] will be checked and will be erased no matter what
			//because if its found the it should be erased from all 3 or one of the others and if its uniuqe then
			// it will be found in any other so it should be deleted to help make other searches faster
			auto it = std::find(rows_notes[other1].begin(), rows_notes[other1].end(), rows_notes[ref].front());
			auto it1 = std::find(rows_notes[other2].begin(), rows_notes[other2].end(), rows_notes[ref].front());
			if (it != rows_notes[other1].end()) {
				rows_notes[ref].erase(rows_notes[ref].begin());
				rows_notes[other1].erase(it);
				if (it1 != rows_notes[other2].end()) {
					rows_notes[other2].erase(it1);
				}
				continue;
			}
			else if (it1 != rows_notes[other2].end()) {
				rows_notes[ref].erase(rows_notes[ref].begin());
				rows_notes[other2].erase(it1);

				continue;
			}
			else {	//Not in other1, not in other2
				for (size_t c{}; c < 9; c++) {	//deleting in every other row, except for itself
					if (_Solved_Grid[row + ref][c] != 0 || c / 3 == col / 3)
						continue;
					else {
						auto it = std::find(__Notes[row + ref][c].begin(), __Notes[row + ref][c].end(), rows_notes[ref].front());
						if (it != __Notes[row + ref][c].end()) {
							__Notes[row + ref][c].erase(it);
							solved_one = true;
						}
					}
				}
				rows_notes[ref].erase(rows_notes[ref].begin());	//Deleting so wont be checked with others
			}
		}
	}


	//Check if a number only mentiond in single column
	std::vector<std::vector<unsigned>> cols_notes{};
	for (size_t i{}; i < 3; i++) {
		std::vector<unsigned> col_notes{};
		for (size_t j{}; j < 3; j++) {

			if (_Solved_Grid[row + j][col + i] == 0) {
				for (auto num : __Notes[row + j][col + i]) {
					if (std::find(col_notes.begin(), col_notes.end(), num) == col_notes.end()) {
						col_notes.push_back(num);
					}
				}
			}
		}
		cols_notes.push_back(col_notes);
	}

	//Check col[0] wiht col[1] & col[2]
	//Check col[1] wiht col[0] & col[2]
	//Check col[2] wiht col[0] & col[1]
	for (size_t ref{}; ref < 3; ref++) {
		//at first iteration col[0] will get empty so at second iteration only col[1] and col[2] need comparison
		unsigned other1{}, other2{};
		if (ref == 0) {
			other1 = 1;
			other2 = 2;
		}
		else if (ref == 1) {
			other1 = 0;
			other2 = 2;
		}
		else {
			other1 = 0;
			other2 = 1;
		}
		for (; !cols_notes[ref].empty(); ) {
			//Check row[ref] wiht row[other1] & row[other2]
			//in this part always first element of row[ref] will be checked and will be erased no matter what
			//because if its found the it should be erased from all 3 or one of the others and if its uniuqe then
			// it will be found in any other so it should be deleted to help make other searches faster
			auto it = std::find(cols_notes[other1].begin(), cols_notes[other1].end(), cols_notes[ref].front());
			auto it1 = std::find(cols_notes[other2].begin(), cols_notes[other2].end(), cols_notes[ref].front());
			if (it != cols_notes[other1].end()) {
				cols_notes[ref].erase(cols_notes[ref].begin());
				cols_notes[other1].erase(it);
				if (it1 != cols_notes[other2].end()) {
					cols_notes[other2].erase(it1);
				}
				continue;
			}
			else if (it1 != cols_notes[other2].end()) {
				cols_notes[ref].erase(cols_notes[ref].begin());
				cols_notes[other2].erase(it1);

				continue;
			}
			else {
				for (size_t r{}; r < 9; r++) {
					if (_Solved_Grid[r][col + ref] != 0 || r / 3 == row / 3)
						continue;
					else {
						auto it = std::find(__Notes[r][col + ref].begin(), __Notes[r][col + ref].end(), cols_notes[ref].front());
						if (it != __Notes[r][col + ref].end()) {
							__Notes[r][col + ref].erase(it);
							solved_one = true;
						}
					}
				}
				cols_notes[ref].erase(cols_notes[ref].begin());
			}
		}
	}

	
	return solved_one;
}

bool Sodoku_Class::obvious_singles() {
	bool Solved_one{ false };
	for (unsigned i{}; i < 9; i++) {
		for (unsigned j{}; j < 9; j++) {

			if (__Notes[i][j].size() == 1) {

				_Solved_Grid[i][j] = __Notes[i][j].front();
				Update_Notes(i, j, __Notes[i][j].front());

				__Notes[i][j].clear();
				__Notes[i][j].shrink_to_fit();

				Solved_one = true;
			}
		}
	}

	return Solved_one;
}

bool Sodoku_Class::hidden_pairs() {
	bool Solved_one{ false };
	for (unsigned i{}; i < 9; i += 3) {
		for (unsigned j{}; j < 9; j += 3) {
			if (hidden_pairs_solve(i, j))
				Solved_one = true;
		}
	}

	return Solved_one;
}

bool Sodoku_Class::hidden_pairs_solve(const unsigned row, const unsigned col) {
	bool solved_one{ false };
	std::vector<unsigned> not_a_pair{};		//numbers that are repeated in more than two cells

	for (uint8_t i{}; i < 9; i++) {	//Going through all cells of a block

		//if cell had a number already
		if (_Solved_Grid[row + i / 3][col + i % 3] != 0)
			continue;

		std::vector<unsigned> can_be_pair{};	//numbers that only apear in two cells. storing order: num1, pos1, num2, pos1

		//Picking a cell and going through all of its notes
		for (const auto& note : __Notes[row + i / 3][col + i % 3]) {
			//if note already examined
			if (std::find(not_a_pair.begin(), not_a_pair.end(), note) != not_a_pair.end())
				continue;
			uint8_t repeat_count{};
			uint8_t pair_pos{10};
			//Picking a number in cell an looking for it in other cells after the first picked cell to avoid lookign in a cell twice 
			for (uint8_t j{ i + 1u }; j < 9u; j++) {

				std::vector<num_t>& _N = __Notes[row + j / 3][col + j % 3];
				//if cell had a number already
				if (_Solved_Grid[row + j / 3][col + j % 3] != 0)
					continue;

				auto it = std::find(_N.begin(), _N.end(), note);
				if (it != _N.end()) {
					++repeat_count;
					pair_pos = j;
				}

				if (repeat_count > 1) {
					not_a_pair.push_back(note);
					pair_pos = 10;
					break;
				}

			}
			//We are here bec of break
			if (repeat_count > 1) {
				continue;
			}
			if (repeat_count == 1) {
				can_be_pair.push_back(note);
				can_be_pair.push_back(pair_pos);
			}
		}

		if (can_be_pair.size() == 2) {	//A single number only apeared in two cells has no use
			not_a_pair.push_back(can_be_pair[0]);
		}
		else if (can_be_pair.size() == 4 && can_be_pair[1] == can_be_pair[3]) {	//We have a pair

			std::vector<num_t>& _N = __Notes[row + i / 3][col + i % 3];
			//First cell containing the pair
			for (auto it = _N.begin(); it != _N.end(); ) {
				if (*it != can_be_pair[0] && *it != can_be_pair[2]) {	//Should be non of the two to be deleted
					it = _N.erase(it);
					solved_one = true;
				}
				else {
					++it;
				}
			}

			std::vector<num_t>& __N = __Notes[row + can_be_pair[1] / 3][col + can_be_pair[1] % 3];
			//Second cell containing the pair
			for (auto it = __N.begin(); it != __N.end(); ) {
				if (*it != can_be_pair[0] && *it != can_be_pair[2]) {	//Should be non of the two to be deleted
					it = __N.erase(it);
					solved_one = true;
				}
				else {
					++it;
				}
			}
		}

	}
	return solved_one;
}


void functions::Rotate(Suduko_table &table) {
	//OR 
	// row{0} = col{8}
	// row{1} = col{7}
	// ...
	// row{8} = col{0}
	for (uint8_t i{}; i < 9/2; i++) {
		for (uint8_t j{ i }; j < 9 - i - 1; j++) {
			Rotate_one_pos(table, Sudoku::Pos{ i,j }, table[i][j]);
		}
	}
}

void functions::Rotate_one_pos(Suduko_table& table, Sudoku::Pos pos , num_t pos_val) {
	//Rotating CW
	//	(i, j)	->	(j, 8-i)
	// 
	//Rotating CCW
	// 	(i, j)	->	(8-j, i)
	// 
	//Start from the number that is gonna replace pos so going CCW

	//To end recursion
	static Pos starting_pos{ 10,10 };
	if (starting_pos.i > 9 || starting_pos.j > 9) {	//Starting recursion, first call
		starting_pos = pos;
	}
	else if (starting_pos == pos) {	//Comming back where we started
		starting_pos.i = 10; starting_pos.j = 10;	//Ready for next call
		return;
	}

	Pos Next_pos = { pos.j, 8 - pos.i };
	num_t temp_num = table[Next_pos.i][Next_pos.j];

	table[Next_pos.i][Next_pos.j] = pos_val;
	Rotate_one_pos(table, Next_pos, temp_num);
}

void functions::Swap_rows(Suduko_table& table, num_t row1, num_t row2) {
	std::array<num_t, 9> temp_row{ table.at(row1) };

	table.at(row1) = table.at(row2);
	table.at(row2) = temp_row;
}

void functions::Swap_cols(Suduko_table& table, num_t col1, num_t col2) {
	std::array<num_t, 9> temp_col{};
	for (uint8_t i{}; i < 9; i++) {
		temp_col[i] = table.at(i).at(col1);
	}

	for (uint8_t i{}; i < 9; i++) {
		table.at(i).at(col1) = table.at(i).at(col2);
		table.at(i).at(col2) = temp_col[i];
	}
}


void Sodoku_Class::Clear()
{
	for (auto& row : Grid) {
		for (auto& num : row) {
			num = 0;
		}
	}
	for (auto& row : __Notes) {
		for (auto& col : row) {
			col.clear();
			col.shrink_to_fit();
		}
	}
	for (auto& row : _Solved_Grid) {
		for (auto& num : row) {
			num = 0;
		}
	}
	for (auto& row : __Seed) {
		for (auto& num : row) {
			num = 0;
		}
	}
}

void Sodoku_Class::Clear_Notes()
{
	for (auto& row : __Notes) {
		for (auto& col : row) {
			col.clear();
		}
	}
}

bool Sodoku_Class::New_Game(game_difficulty __diff)
{
	this->_difficulty = __diff;
	Clear();

	if (Find_Solvable(__diff)){
		return true;
	}
	else
		return false;
}

bool Sodoku_Class::New_Game(Suduko_table& seed, Suduko_table& solved, Sudoku::game_difficulty __diff) {
	this->_difficulty = __diff;
	Clear();

	__Seed = seed;
	_Solved_Grid = solved;

	uint8_t randomizer = rand() % 4;	//rotate up to 3 time
	for (uint8_t i{}; i < randomizer; i++) {
		functions::Rotate(__Seed);
		functions::Rotate(_Solved_Grid);
	}

	randomizer = rand() % 7;	//Swaping up to 6 rows and columns
	for (uint8_t i{}; i < randomizer; i++) {
		uint8_t block_start = rand() % 3;
		uint8_t x = rand() % 3;
		uint8_t y = rand() % 3;
		if (x != y) {
			functions::Swap_rows(__Seed, 3 * block_start + x, 3 * block_start + y);
			functions::Swap_rows(_Solved_Grid, 3 * block_start + x, 3 * block_start + y);
		}

		block_start = rand() % 3;
		x = rand() % 3;
		y = rand() % 3;
		if (x != y) {
			functions::Swap_cols(__Seed, 3 * block_start + x, 3 * block_start + y);
			functions::Swap_cols(_Solved_Grid, 3 * block_start + x, 3 * block_start + y);
		}
	}

	Grid = __Seed;
	return true;
}

bool Sodoku_Class::Find_Solvable(Sudoku::game_difficulty diff) {
	//const unsigned max_iterate{ 10000 };
	//unsigned iter{};
	bool found{ false };

	if (diff == Easy || diff == Simple) {
		_difficulty = Normal;
	}

	//while (iter < max_iterate) {
	while (1) {
		//Filling random numbers
		for (unsigned num{}; num < starting_numbers[_difficulty]; num++) {
			random_number_random_pos();
		}
		__Seed = _Solved_Grid;

		if (Solve()) {
			Grid = __Seed;
			Clear_Notes();
			if (diff != Easy && diff != Simple)
				return true;

			//For easy and simple
			found = true;
			break;
		}

		Clear();
		//++iter;
	}

	//For easy and simple
	if (found) {
		int randomizer = rand() % 3;			// 0, 1, 2
		randomizer *= (rand() % 2) ? -1 : 1;	// -2, -1, 0, 1, 2

		uint8_t adding_count = starting_numbers[diff] - starting_numbers[Normal] + randomizer;

		for (uint8_t i{}; i < adding_count; ) {
			uint8_t r_i = rand() % 9;
			uint8_t r_j = rand() % 9;
			if (Grid[r_i][r_j] == 0) {
				Grid[r_i][r_j] = _Solved_Grid[r_i][r_j];
				__Seed[r_i][r_j] = _Solved_Grid[r_i][r_j];
				i++;
			}
		}

		_difficulty = diff;
		return true;
	}
	else
		return false;
}

bool Sodoku_Class::is_solvable() {	//if a note is empty means that no number can be palced there
	for (uint8_t i{}; i < 9; i++) {
		for (uint8_t j{}; j < 9; j++) {
			if (_Solved_Grid[i][j] == 0 && __Notes[i][j].empty())
				return false;
		}
	}
	return true;
}

void Sodoku_Class::random_number_random_pos() {
	unsigned i = rand() % 9;
	unsigned j = rand() % 9;
	if (_Solved_Grid[i][j] == 0) {
		unsigned number = rand() % 9 + 1;	//1 - 9
		if (!check_row(i, number) && !check_col(j, number) && !check_block(i, j, number)) {
			_Solved_Grid[i][j] = number;
			return;
		}
		else {
			if(_difficulty >= Normal)
				random_number_random_pos();
		}
	}
	else {
		random_number_random_pos();
	}
}