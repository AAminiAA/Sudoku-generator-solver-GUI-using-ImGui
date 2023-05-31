#pragma once
#include "../App/ImGui_DX9.h"
#include "Sodoku_Class.h"
#include "imgui_internal.h"
#include <chrono>	//For timing
#include <list>		//For undo list

namespace App {
	namespace variables
	{
		inline ImVec2 mouse_pos{};
		inline bool mouse_clicked{};
		inline bool key_pressed{};
		inline ImGuiKey keyboard_key{};
	}
}

#pragma pack(push, 1)
struct _Save_struct
{
	unsigned Time;

	char Save_name[50];
	uint8_t Difficulty;
	uint8_t Mistakes;
	uint8_t Hints;

	uint8_t Seed[9][9];
	uint8_t Grid[9][9];
	uint8_t Notes[9][9][9];
};

struct Setting_Save_struct
{
	bool game_can_resume_ask;
	bool new_game_ask;

	std::array<bool, 4> generate_or_load;
};

struct Seed_save_struct
{
	Suduko_table Seed;
	Suduko_table Solve;
};
#pragma pack(pop)

struct _Undo_struct
{
	bool is_note;
	uint8_t changed_cell[2];	//Row, Col
	//for Numbers: Number that was here and now its changed
	//for Notes: the note that user added
	uint8_t num;
	//for Numbers: Nothing
	//for Notes: the note that user deleted
	uint8_t removed_note;
};

class Suduko_Game : public ImGui_DX9
{

public:
	Suduko_Game(const wchar_t *win_name);
	virtual void Update() override;
	void Add_fonts_style();

	virtual ~Suduko_Game();

private:
	void show_window();

	void reset_game_state();
	bool give_hint();
	void undo();
	void update_time();

	bool quit_current_game_modal();
	bool game_over_modal();
	bool save_game_modal();

	bool save_last_game();
	bool load_last_game();
	void game_to_save_struct(_Save_struct& _save);
	void save_struct_to_game(_Save_struct& _save);

	void show_table();
	void draw_cell(const size_t row, const size_t col);
	void draw_notes(ImVec2 upper_left, ImVec2 lower_right);
	void draw_block_borders();
	void draw_outter_rect();
	bool cell_hoverd(ImVec2 cell_upper_left, ImVec2 cell_lower_right);
	bool cell_clicked(ImVec2 cell_upper_left, ImVec2 cell_lower_right);
	void change_from_keyboard();

	bool save_seed(Sudoku::game_difficulty diff);
	bool load_seed(Sudoku::game_difficulty diff);

private:
	Sodoku_Class Suduko_table;

	float cell_size;
	const float edge_offset{ 20 };

	ImVec2 clicked_cell;
	bool take_notes{};

	bool game_started;
	bool game_over;
	bool _second_chance;
	bool current_game_is_saved{};
	unsigned __Mistakes;
	unsigned __Hints;
	unsigned _Max_Mistakes{ 3 };
	unsigned __Remaining;
	unsigned __Time;
	std::chrono::steady_clock::time_point __starting_time;

	std::list<_Undo_struct> __Undo_list;

private:

	ImDrawList* draw;

private:

	static constexpr unsigned __Max_Hints{ 3 };
	static constexpr unsigned __Max_Undo{ 15 };
};

