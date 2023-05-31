#include "Suduko.h"
#include "ImGui_Styles.h"
#include <fstream>
#include "DroidSans.hpp"//Embedded Font
#include <Windows.h>	//For save
#include <filesystem>	//For save

#define Cursor_in_cell(upper_left, bottom_right)		(variables::mouse_pos.x >= upper_left.x && variables::mouse_pos.x <= bottom_right.x && variables::mouse_pos.y >= upper_left.y && variables::mouse_pos.y <= bottom_right.y)

#define	push_text_small_font	ImGui::PushFont(fonts::text_small)
#define	push_text_larg_font		ImGui::PushFont(fonts::text_large)
#define	push_num_font			ImGui::PushFont(fonts::number)
#define	push_note_font			ImGui::PushFont(fonts::note)
#define pop_font				ImGui::PopFont()

namespace App {

	namespace metrics
	{
		ImVec2 Menu_Size{};
	}

	namespace fonts
	{
		float number_size;
		float note_size;
		float text_small_size;
		float text_large_size;

		ImFont* number;
		ImFont* note;
		ImFont* text_small;
		ImFont* text_large;
	}

	namespace colors
	{
		//BackGround_color = ImVec4(0.30f, 0.385f, 0.395f, 1.00f);
		//colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.372f, 0.372f, 0.372f, 0.60f);
		enum App_colors
		{
			Cell_boarder,
			Cell_hoverd,
			Cell_highlite,
			Nums_seed,
			Nums_correct,
			Nums_hoverd,
			Nums_wrong,
			BackGround,
			Colors_count
		};
		static constexpr uint32_t Colors_array[Colors_count]
		{
			4280163870U, 4292142080U, 4293512599U, 4278190080U, 4294914816U, 4282071988U, 4278190335U, 4284834381U
		};
	}

	namespace path {
		static constexpr wchar_t save_prefix[7]{ L"Suduko" };
		static constexpr wchar_t save_format[5]{ L".dat" };
		static constexpr wchar_t seed_format[6]{ L".seed" };
		static constexpr wchar_t last_save_filename[23]{ L"SudukoLastGameSave.dat" };
		static constexpr wchar_t user_save_filename[19]{ L"SudukoUserSave.dat" };
		static constexpr wchar_t game_setting_filename[23]{ L"SudukoGameSetting.dat" };

		static std::filesystem::path game_save_path;
	}

	namespace variables
	{
		static constexpr float table_boarder_thickness{ 6.0f };
		static constexpr float table_cell_thickness{ 2.0f };

		static constexpr const char* game_diff_str[4]{ "Simple", "Easy", "Normal", "Hard" };

		static bool game_complete{ false };
		static bool time_paused{ false };

		static bool game_can_resume_dont_ask = false;
		static bool new_game_dont_ask = false;

		static bool game_is_a_loadded_save{ false };
		static _Save_struct loadded_game_save{};	//Potential memory leak if copied

		static std::array<bool, 4> generate_or_load{true, true, true, true};	//Default is generate all
	}

	namespace functions
	{
		static void make_document_path();

		void static show_overlay(bool save_update);

		static bool load_game_settings();
		static bool save_game_settings();
		static bool load_saves(std::vector<_Save_struct>& _saves);
		static bool save_game(_Save_struct& _save);
		static void clear_user_saves();
		static bool update_user_save();	//When a save is loaded and wanna save on top of that
	}
}

using namespace App;

Suduko_Game::Suduko_Game(const wchar_t* win_name)
	: ImGui_DX9(win_name, colors::Colors_array[colors::BackGround]), Suduko_table{}, clicked_cell{10,10}, take_notes{false},
	game_started{ false }, game_over{ false }, _second_chance{ false }, __Mistakes{}, __Remaining{}, __Time{},
	__starting_time{}, __Hints{}, __Undo_list{}
{
	//load_colors();
	functions::make_document_path();
	functions::load_game_settings();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = NULL;	//Disable imgui.ini

	cell_size = (Win_size.y - 2 * edge_offset) / 9;

	metrics::Menu_Size.x = Win_size.x - (9 * cell_size + 2 * edge_offset);
	metrics::Menu_Size.y = Win_size.y;

	fonts::number_size		= cell_size * 0.7f;
	fonts::note_size		= fonts::number_size / 3;
	fonts::text_small_size	= fonts::number_size * 0.5f;
	fonts::text_large_size	= fonts::number_size * 0.6f;
}

void Suduko_Game::Update() {
	
	show_window();
	//show_color_window();

	show_table();

	//static bool show_demo{ true };
	//if (show_demo) ImGui::ShowDemoWindow(&show_demo);
}

void Suduko_Game::Add_fonts_style()
{
	ImGuiIO& io = ImGui::GetIO();

	fonts::number = io.Fonts->AddFontFromMemoryCompressedBase85TTF(DroidSans_compressed_data_base85, fonts::number_size);
	fonts::note = io.Fonts->AddFontFromMemoryCompressedBase85TTF(DroidSans_compressed_data_base85, fonts::note_size);
	fonts::text_small = io.Fonts->AddFontFromMemoryCompressedBase85TTF(DroidSans_compressed_data_base85, fonts::text_small_size);
	fonts::text_large = io.Fonts->AddFontFromMemoryCompressedBase85TTF(DroidSans_compressed_data_base85, fonts::text_large_size);

	imgui_style_suduko_dark();
}

void Suduko_Game::show_window() {
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize;

	ImGui::SetNextWindowSize(metrics::Menu_Size);
	ImGui::SetNextWindowPos(ImVec2{ 2 * edge_offset + 9 * cell_size, 0 });
	ImGui::Begin("Game Options", nullptr, flags);

	static bool load_save_menu = false;
	static bool start_menu = false;
	static bool option_menu = false;
	static bool about_menu = false;

	static constexpr char choose_dif[] { "Choose game difficulty:" };
	

	static const ImVec2 button_size{ metrics::Menu_Size.x - 2 * edge_offset, fonts::text_large_size * 1.6f };
	static const float button_gap{ button_size.y / 4 };

	static const ImVec2 start_pos{ edge_offset, metrics::Menu_Size.y / 7 };
	static const ImVec2 back_quite_pos{ edge_offset, metrics::Menu_Size.y - button_size.y - 40.0f };

	static std::vector<_Save_struct> loaded_saves{};	//Store

	if (option_menu) {
		push_text_small_font;

		static bool generate_checkbox{ false };
		static bool first_time{ true };
		

		ImGui::BulletText("Generate on computer");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30.0f);
		if (ImGui::Checkbox("##All", &generate_checkbox)) {
			if (!generate_checkbox) {
				variables::generate_or_load.fill(false);

			}
			else {
				variables::generate_or_load.fill(true);

			}
		}

		ImGui::Indent(30.0f);

		ImGui::AlignTextToFramePadding();
		ImGui::BulletText("Generate Simple");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);
		ImGui::Checkbox("##Simple", &variables::generate_or_load[Sudoku::Simple]);

		ImGui::AlignTextToFramePadding();
		ImGui::BulletText("Generate Easy");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);
		ImGui::Checkbox("##Easy", &variables::generate_or_load[Sudoku::Easy]);

		ImGui::AlignTextToFramePadding();
		ImGui::BulletText("Generate Normal");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);
		ImGui::Checkbox("##Normal", &variables::generate_or_load[Sudoku::Normal]);

		ImGui::AlignTextToFramePadding();
		ImGui::BulletText("Generate Hard");
		ImGui::SameLine(ImGui::GetContentRegionAvail().x);
		ImGui::Checkbox("##Hard", &variables::generate_or_load[Sudoku::Hard]);

		ImGui::Unindent();

		//At least one is selected
		if (variables::generate_or_load[0] || variables::generate_or_load[1] || variables::generate_or_load[2] || variables::generate_or_load[3])
			generate_checkbox = true;

		if (generate_checkbox && !variables::generate_or_load[0] && !variables::generate_or_load[1] && !variables::generate_or_load[2] && !variables::generate_or_load[3])
			generate_checkbox = false;



		pop_font;
		push_text_larg_font;
		ImGui::SetCursorPos(back_quite_pos);
		if (ImGui::Button("Back", button_size)) {
			option_menu = false;
		}

		pop_font;
	}

	else if (about_menu) {
		push_text_small_font;
		ImGui::Text("This game is made by Aminia\nUsing ImGui API v1.89.6\n\nGame version: 0.1.0");
		ImGui::Separator();

		pop_font;
		push_text_larg_font;
		ImGui::SetCursorPos(ImVec2{ back_quite_pos.x, back_quite_pos.y - button_size.y - button_gap });
		if (ImGui::Button("Open GitHub page", button_size)) {
			ShellExecute(NULL, L"open", L"https://github.com/AAminiAA", 0, 0, SW_SHOWNORMAL);
		}

		ImGui::SetCursorPos(back_quite_pos);
		if (ImGui::Button("Back", button_size)) {
			about_menu = false;
		}
		pop_font;
	}

	else if (load_save_menu) {
		push_text_larg_font;

		ImGui::SeparatorText("Your saves");
		static int selected{-1};

		ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY;

		pop_font;
		push_text_small_font;
		if (ImGui::BeginTable("Saves", 3, flags, ImVec2{0, 320})) {

			ImGui::TableSetupColumn("#");
			ImGui::TableSetupColumn("Save name");
			ImGui::TableSetupColumn("Difficulty");
			ImGui::TableHeadersRow();

			for (int row = 0; row < loaded_saves.size(); row++)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				
				ImGui::Text("%u", row+1);
				ImGui::TableNextColumn();
				if (ImGui::Selectable(loaded_saves[row].Save_name, selected == row, ImGuiSelectableFlags_SpanAllColumns))	//Single selectibale
					selected = row;
				ImGui::TableNextColumn();
				ImGui::Text(variables::game_diff_str[loaded_saves[row].Difficulty]);
			}

			ImGui::EndTable();
		}

		pop_font;
		push_text_larg_font;

		if (selected >= 0 && loaded_saves.size() > 0) {	//Only show these button when a save is selected
			ImGui::Separator();
			if (ImGui::Button("Load save")) {

				reset_game_state();
				variables::game_is_a_loadded_save = true;
				variables::loadded_game_save = loaded_saves.at(selected);	//Potential memory leak in copping

				save_struct_to_game(loaded_saves.at(selected));
				//Reset timer
				__starting_time = std::chrono::steady_clock::now();
				__Remaining = Suduko_table.Remaining();

				if (Suduko_table.Is_Solved()) {
					game_over = true;
				}
				else {
					game_over = false;
				}

				game_started = true;
				load_save_menu = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Delete save")) {
				loaded_saves.erase(loaded_saves.begin() + selected);
			}
		}
		ImGui::SetCursorPos(ImVec2{ back_quite_pos.x, back_quite_pos.y - button_size.y - 7 });
		if (ImGui::Button("Clear all saves", button_size)) {
			functions::clear_user_saves();
			functions::load_saves(loaded_saves);
		}
		ImGui::SetCursorPos(back_quite_pos);
		if (ImGui::Button("Back", button_size)) {
			load_save_menu = false;
		}

		if (!load_save_menu) {	//after the user choose to close this menu
			functions::clear_user_saves();
			//Update saves
			for (auto& s : loaded_saves) {
				functions::save_game(s);
			}

			loaded_saves.clear();	//To save shit ton of memory bec its static
			loaded_saves.shrink_to_fit();

			selected = -1;	//Next time nothing is selected by default
		}

		pop_font;
	}

	else if (game_started || variables::time_paused) {	//Game menu
		push_text_small_font;
		static bool show_game_paused_overlay{ false };

		if (!variables::time_paused)
			update_time();

		ImGui::Text("Remaining: %u", __Remaining);
		ImGui::SameLine();
		ImGui::Text("\t| %s", variables::game_diff_str[Suduko_table.Get_difficulty()]);

		ImGui::Text("Time:\t %02u:%02u", __Time / 60, __Time % 60); ImGui::SameLine();
		char pause_lable[6]{};
		strcpy_s(pause_lable, 6, (show_game_paused_overlay) ? "Start" : "Pause");	//Not using time_paused bec other places can manupilate it

		if (ImGui::Button(pause_lable) && !game_over) {	//After game over time is stoped and can not change
			if (variables::time_paused) {

				show_game_paused_overlay = false;

				variables::time_paused = false;	//if time was paused, it is now started	and timer will reset
				game_started = true;
			
				__starting_time = std::chrono::steady_clock::now();
			}
			else {	//if time was started, it is now paused
				show_game_paused_overlay = true;
				variables::time_paused = true;
				game_started = false;
			}

		}

		ImGui::Text("Mistakes: %u/%u", __Mistakes, _Max_Mistakes);
		ImGui::SameLine();
		ImGui::Text("\tHints: %u/%u", __Hints, __Max_Hints);
		ImGui::Separator();

		pop_font;
		push_text_larg_font;

		static constexpr char _note_is_on[]	{ "Note ON " };
		static constexpr char _note_is_off[]{ "Note OFF" };

		static float triple_button_size{ button_size.x - 2 * button_gap };
		ImGui::SetCursorPosX(edge_offset);
		std::string _button_lable = (take_notes) ? _note_is_on : _note_is_off;
		if (ImGui::Button(_button_lable.c_str(), ImVec2{ triple_button_size * 0.45f, button_size.y })) {
			take_notes = !take_notes;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) // With a delay
			ImGui::SetTooltip("You can also press N\nto toggle note");

		ImGui::SameLine(0, button_gap);
		if (ImGui::Button("Hint", ImVec2{ triple_button_size * 0.275f, button_size.y }) && !game_over) {	//Can only hint if game is running
			if (__Hints < __Max_Hints) {
				give_hint();
				__Hints++;
			}
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) // With a delay
			ImGui::SetTooltip("You can also press H\nto get a hint");

		ImGui::SameLine(0, button_gap);
		if (ImGui::Button("Undo", ImVec2{ triple_button_size * 0.275f, button_size.y }) && !game_over) {	//Can only undo if game is running
			if (__Undo_list.size() > 0)
				undo();
		}

		static bool show_save_update_overlay{ false };
		ImGui::SetCursorPos(ImVec2{ back_quite_pos.x, back_quite_pos.y - button_size.y - button_gap });
		ImVec2 save_button_size{ button_size.x/3, button_size.y };
		if (ImGui::Button("Save", save_button_size)) {
			if (variables::game_is_a_loadded_save) {
				game_to_save_struct(variables::loadded_game_save);

				if (functions::update_user_save())
					show_save_update_overlay = true;
			}
			else {
				variables::time_paused = true;
				ImGui::OpenPopup("Save");
			}
		}
		ImGui::SameLine(0, button_gap);
		static bool _New_Game = false;
		if (ImGui::Button("New game", ImVec2{ button_size.x - save_button_size.x - button_gap, button_size.y })) {
			//If player checked dont ask me again or the game was over, no need for modal
			if(variables::new_game_dont_ask || game_over){
				_New_Game = false;
				start_menu = true;	//going to select difficulty menu

				show_game_paused_overlay = false;	//When exiting game during pause, dont come back with pause

				reset_game_state();
				pop_font;
				ImGui::End();
				return;
			}

			variables::time_paused = true;
			ImGui::OpenPopup("Close this game?##NewGame");
			_New_Game = true;
		}
		//if (ImGui::Button("Export seed")) {

		//}
		ImGui::SetCursorPos(back_quite_pos);
		if (ImGui::Button("Main menu", button_size)) {
			//If player checked dont ask me again or the game was over, no need for modal
			if (variables::game_can_resume_dont_ask || game_over) {	//Dont ask me next time
				save_last_game();	//going back to main menu WILL save your current game
				current_game_is_saved = true;

				show_game_paused_overlay = false;

				reset_game_state();
				pop_font;
				ImGui::End();
				return;		//Nothing else needs executaion when we are gonna close it anyway
			}
			else {
				variables::time_paused = true;
				ImGui::OpenPopup("Close this game?##Exit");
			}
		}
		
		//Handling Modal window if main menu pressed
		if (quit_current_game_modal()) {
			//This could be from new game or main menu

			if (_New_Game) {	//Called by new game
				//new game WILL NOT save your current game
				_New_Game = false;
				start_menu = true;	//going to "select difficulty" menu
			}

			show_game_paused_overlay = false;	//When exiting game during pause, dont come back with pause

			reset_game_state();
			pop_font;
			ImGui::End();
			return;		//Nothing else needs executaion when we are gonna close it anyway
		}

		//Handling game over by completion
		if (variables::game_complete) {
			variables::time_paused = true;
			variables::game_complete = false;	//bec this modal will be called after completion every frame
			ImGui::OpenPopup("Congratulations");
		}

		//Handling game over by over mistake
		if (__Mistakes > _Max_Mistakes && _second_chance && !game_over) {
			variables::time_paused = true;
			ImGui::OpenPopup("Game over##1");
		}
		
		else if (__Mistakes > _Max_Mistakes && !game_over) {	//if game is over and we are here, means player spectating or second chance
			variables::time_paused = true;
			ImGui::OpenPopup("Game over");
		}
		
		if (game_over_modal()) {	//Returns true when player selects exit, for game over and game realy over
			save_last_game();	//because game_over is true, this wont save game, just trunc the last save
			reset_game_state();	//we do not save the game here

			show_game_paused_overlay = false;
			start_menu = false;
		}

		save_game_modal();

		if (show_save_update_overlay) {
			static auto overlay_timer_begin =  std::chrono::steady_clock::now();
			static bool timer_started{ true };
			if (timer_started) {
				auto now = std::chrono::steady_clock::now();
				std::chrono::duration<unsigned> time = std::chrono::duration_cast<std::chrono::seconds>(now - overlay_timer_begin);
				if (time.count() < 2) {
					functions::show_overlay(true);
				}
				else {
					timer_started = false;
					show_save_update_overlay = false;
				}
			}
			else {
				timer_started = true;
				overlay_timer_begin = std::chrono::steady_clock::now();
			}
		}
		else if (show_game_paused_overlay) {
			functions::show_overlay(false);
		}

		pop_font;
	}

	else if (start_menu) {
		push_text_larg_font;

		ImGui::Text(choose_dif);
		ImGui::SetCursorPos(start_pos);
		if (ImGui::Button(variables::game_diff_str[Sudoku::Simple], button_size)) {

			if (variables::generate_or_load[Sudoku::Simple]) {	//true means want to generate
				if (Suduko_table.New_Game(Sudoku::Simple)) {
					game_started = true;
					save_seed(Sudoku::Simple);
				}
			}
			else {
				if (load_seed(Sudoku::Simple)) {
					game_started = true;
				}
			}

				
		}
		float new_y_pos = start_pos.y + button_size.y + button_gap;
		float x_pos = start_pos.x;
		ImGui::SetCursorPos(ImVec2{ x_pos, new_y_pos });
		if (ImGui::Button(variables::game_diff_str[Sudoku::Easy], button_size)) {

			if (variables::generate_or_load[Sudoku::Easy]) {	//true means want to generate
				if (Suduko_table.New_Game(Sudoku::Easy)) {
					game_started = true;
					save_seed(Sudoku::Easy);
				}
			}
			else {
				if (load_seed(Sudoku::Easy)) {
					game_started = true;
				}
			}

		}
		new_y_pos += button_size.y + button_gap;
		
		ImGui::SetCursorPos(ImVec2{ x_pos, new_y_pos });
		if (ImGui::Button(variables::game_diff_str[Sudoku::Normal], button_size)) {

			if (variables::generate_or_load[Sudoku::Normal]) {	//true means want to generate
				if (Suduko_table.New_Game(Sudoku::Normal)) {
					game_started = true;
					save_seed(Sudoku::Normal);
				}
			}
			else {
				if (load_seed(Sudoku::Normal)) {
					game_started = true;
				}
			}
		}
		new_y_pos += button_size.y + button_gap;
		ImGui::SetCursorPos(ImVec2{ x_pos, new_y_pos });
		if (ImGui::Button(variables::game_diff_str[Sudoku::Hard], button_size)) {

			if (variables::generate_or_load[Sudoku::Hard]) {	//true means want to generate
				if (Suduko_table.New_Game(Sudoku::Hard)) {
					game_started = true;
					save_seed(Sudoku::Hard);
				}
			}
			else {
				if (load_seed(Sudoku::Hard)) {
					game_started = true;
				}
			}
		}
		ImGui::SetCursorPos(back_quite_pos);
		if (ImGui::Button("Back", button_size)) {
			start_menu = false;
		}

		if (game_started) {	//Player selected a game
			//Timer start
			__Time = 0;
			__starting_time = std::chrono::steady_clock::now();
			__Remaining = Suduko_table.Remaining();
			game_over = false;	//if the last game was a game over, only way to reset that is  to start another game
			variables::game_is_a_loadded_save = false;
			start_menu = false;
		}

		pop_font;
	}

	else {	//Main menu
		push_text_larg_font;

		ImGui::SetCursorPos(start_pos);
		if (ImGui::Button("Start", button_size)) {
			start_menu = true;
		}
		float new_y_pos = start_pos.y + button_size.y + button_gap;	//Creating Gap
		float x_pos = start_pos.x;
		ImGui::SetCursorPos(ImVec2{ x_pos, new_y_pos });
		if (ImGui::Button("Resume last game", button_size)) {
			if (load_last_game()) {	//returns false when no save, no file, no directory
				game_started = true;
			}
		}
		new_y_pos += button_size.y + button_gap;
		ImGui::SetCursorPos(ImVec2{ x_pos, new_y_pos });
		if (ImGui::Button("Load save", button_size)) {
			functions::load_saves(loaded_saves);
			load_save_menu = true;
		}
		new_y_pos += button_size.y + button_gap;
		ImGui::SetCursorPos(ImVec2{ x_pos, new_y_pos });
		if (ImGui::Button("Setting", button_size)) {
			option_menu = true;
		}
		new_y_pos += button_size.y + button_gap;
		ImGui::SetCursorPos(ImVec2{ x_pos, new_y_pos });
		if (ImGui::Button("About", button_size)) {
			about_menu = true;
		}

		ImGui::SetCursorPos(back_quite_pos);
		if (ImGui::Button("Quit", button_size)) {
			PostMessage(this->hwnd, WM_CLOSE, 0, 0);
		}

		pop_font;
	}

	ImGui::End();
}

void Suduko_Game::update_time() {
	auto now = std::chrono::steady_clock::now();
	if (std::chrono::duration_cast<std::chrono::seconds>(now - __starting_time).count() == 1) {
		__Time++;
		__starting_time = now;
	}
}

bool Suduko_Game::give_hint() {
	if (variables::game_complete)
		return false;
	while (1) {
		unsigned i = rand() % 9;
		unsigned j = rand() % 9;
		//if cell is empty or it has a wrong number in it can store hint
		if (Suduko_table.at(i).at(j) == 0 || (Suduko_table.at(i).at(j) != 0 && Suduko_table.at(i).at(j) != Suduko_table.at_solved(i).at(j))) {
			Suduko_table.at(i).at(j) = Suduko_table.at_solved(i).at(j);

			Suduko_table.Update_Notes(i, j, Suduko_table.at(i).at(j));
			//Correct number will delete the notes in that cell
			Suduko_table.at_notes(i).at(j).clear();
			Suduko_table.at_notes(i).at(j).shrink_to_fit();

			clicked_cell.x = j;
			clicked_cell.y = i;
			break;
		}
	}
	__Remaining--;

	if (Suduko_table.Is_Solved()) {	//bec if game is complete and you ask for hint game will stuck at while loop
		variables::game_complete = true;
	}
	return true;
}

void Suduko_Game::undo() {
	_Undo_struct& _s = __Undo_list.back();
	if (_s.is_note) {
		std::vector<num_t>& _n = Suduko_table.at_notes(_s.changed_cell[0]).at(_s.changed_cell[1]);
		if (_s.removed_note == 0) {	//Player added a note before and it should be erased now
			auto it = std::find(_n.begin(), _n.end(), _s.num);
			_n.erase(it);
		}
		else {	//Player erased a note before and it should be added now
			_n.push_back(_s.removed_note);
			//Sort for number to be consistant after push back
			std::sort(_n.begin(), _n.end());
		}
	}
	else {
		Suduko_table.at(_s.changed_cell[0]).at(_s.changed_cell[1]) = _s.num;
	}

	__Undo_list.pop_back();
}

void Suduko_Game::reset_game_state() {
	_second_chance = false;
	game_started = false;
	//game_over = false;
	__Mistakes = 0;
	__Hints = 0;
	_Max_Mistakes = 3;
	take_notes = false;

	__Undo_list.clear();

	variables::game_complete = false;
	variables::game_is_a_loadded_save = false;

	variables::time_paused = false;
	__Time = 0;

	clicked_cell.x = 10;
	clicked_cell.y = 10;
}

bool Suduko_Game::game_over_modal() {
	//Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	bool should_exit = false;

	if (ImGui::BeginPopupModal("Game over", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("You made more than %u mistakes!\nDo you want to continue with one more chance?", _Max_Mistakes);
		ImGui::Separator();

		if (ImGui::Button("Exit", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			should_exit = true;
			game_over = true;
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Continue", ImVec2(120, 0))) {
			_second_chance = true;
			_Max_Mistakes++;

			variables::time_paused = false;	//if player chooses to continue
			__starting_time = std::chrono::steady_clock::now();

			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Game over##1", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("You uesed your second chance!\nGood luck next time.", _Max_Mistakes);
		ImGui::Separator();

		if (ImGui::Button("Exit", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			should_exit = true;
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Spectate", ImVec2(120, 0))) {
			clicked_cell.x = 10;	//Because player cant change anything its better that nothing is selected
			clicked_cell.y = 10;
			ImGui::CloseCurrentPopup();
		}

		game_over = true;

		ImGui::EndPopup();
	}
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Congratulations", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("You completed a game!\nTry somthing harder to chalenge yourself.", _Max_Mistakes);
		ImGui::Separator();

		if (ImGui::Button("Exit", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			should_exit = true;
		}
		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Spectate", ImVec2(120, 0))) {
			clicked_cell.x = 10;	//Because player cant change anything its better that nothing is selected
			clicked_cell.y = 10;
			ImGui::CloseCurrentPopup();
		}

		game_over = true;

		ImGui::EndPopup();
	}

	return should_exit;
}

bool Suduko_Game::quit_current_game_modal() {
	bool should_exit = false;
	//Always center this window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Close this game?##NewGame", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Are you sure you want to exit?\nThis game will be lost if not saved!");
		ImGui::Separator();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::Checkbox("Don't ask me next time", &variables::new_game_dont_ask);
		ImGui::PopStyleVar();

		
		if (ImGui::Button("Exit", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();
			should_exit = true;
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) { 
			ImGui::CloseCurrentPopup(); 

			variables::time_paused = false;
			__starting_time = std::chrono::steady_clock::now();
		}

		ImGui::EndPopup();
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Close this game?##Exit", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Your current game is saved automatically\nAnd you can resume playing in the main menu.");
		ImGui::Separator();

		
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::Checkbox("Don't show this next time", &variables::game_can_resume_dont_ask);
		ImGui::PopStyleVar();


		if (ImGui::Button("Exit", ImVec2(120, 0))) {
			save_last_game();	//going back to main menu WILL save your current game
			current_game_is_saved = true;

			ImGui::CloseCurrentPopup();
			should_exit = true;
		}

		ImGui::SetItemDefaultFocus();
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0))) {
			ImGui::CloseCurrentPopup();

			variables::time_paused = false;
			__starting_time = std::chrono::steady_clock::now();
		}

		ImGui::EndPopup();
	}

	return should_exit;
}

bool Suduko_Game::save_game_modal() {

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	static bool save_modal{ true };
	if (ImGui::BeginPopupModal("Save", &save_modal, ImGuiWindowFlags_AlwaysAutoResize)) {
		static char text_buffer[50]{};
		ImGui::Text("Add this game to your saves collection.");
		ImGui::Separator();
		ImGui::InputText("Save name", text_buffer, 50);

		ImGui::NewLine();
		if (ImGui::Button("Save")) {

			_Save_struct save{};
			game_to_save_struct(save);
			strcpy_s(save.Save_name, 50, text_buffer);
			functions::save_game(save);

			variables::time_paused = false;
			__starting_time = std::chrono::steady_clock::now();

			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			variables::time_paused = false;
			__starting_time = std::chrono::steady_clock::now();

			ImGui::CloseCurrentPopup();
		}


		ImGui::EndPopup();
	}
	return true;
}

void Suduko_Game::show_table()
{
	draw = ImGui::GetBackgroundDrawList();	//Must be called each time

	draw_outter_rect();
	for (size_t i{}; i < 9; i++) {
		for (size_t j{}; j < 9; j++) {
			draw_cell(i, j);
		}
	}
	draw_block_borders();
}

void Suduko_Game::draw_outter_rect() {

	//Drawing inner background rect
	ImVec2 upper_left{ edge_offset, edge_offset };
	ImVec2 lower_right{ edge_offset + 9 * cell_size, edge_offset + 9 * cell_size };

	draw->AddRectFilled(upper_left, lower_right, ImGui::GetColorU32(ImGuiCol_Text), variables::table_boarder_thickness);

	//Drawing outer boarder rect
	upper_left.x = edge_offset - variables::table_boarder_thickness / 2;
	upper_left.y = edge_offset - variables::table_boarder_thickness / 2;

	lower_right.x = edge_offset + 9 * cell_size + variables::table_boarder_thickness / 2;
	lower_right.y = edge_offset + 9 * cell_size + variables::table_boarder_thickness / 2;

	//Give it 3 rounding
	draw->AddRect(upper_left, lower_right, colors::Colors_array[colors::Cell_boarder], 3.0f, 0, variables::table_boarder_thickness);
}

void Suduko_Game::draw_cell(const size_t row, const size_t col) {
	static bool cell_is_clicked = false;

	ImVec2 upper_left{ edge_offset + col * cell_size , edge_offset + row * cell_size };
	ImVec2 lower_right{ edge_offset + (col+1) * cell_size , edge_offset + (row+1) * cell_size };

	bool cell_is_hoverd = cell_hoverd(upper_left, lower_right);
	bool clicked_cell_is_number = (clicked_cell.x < 9 && clicked_cell.y < 9) && (Suduko_table.at(clicked_cell.y).at(clicked_cell.x) != 0);
	
	if (game_started) {	//No num, note, click when game is not started
		
		//Order of these if elses matter ***
		//Cell is clicked -> Highest priority
		if (clicked_cell.y == row && clicked_cell.x == col) {
			draw->AddRectFilled(upper_left, lower_right, colors::Colors_array[colors::Cell_hoverd]);

		}
		//Same number as the clicked cell -> Second priority
		else if (clicked_cell_is_number && Suduko_table.at(row).at(col) == Suduko_table.at(clicked_cell.y).at(clicked_cell.x)) {
			draw->AddRectFilled(upper_left, lower_right, colors::Colors_array[colors::Cell_hoverd]);
		}
		//Cell is hovered -> Third priority
		else if (cell_is_hoverd && game_started) {
			draw->AddRectFilled(upper_left, lower_right, colors::Colors_array[colors::Cell_hoverd]);
		}
		//Cell is highlited -> Last priority
		else if (clicked_cell.y == row || clicked_cell.x == col || ((uint8_t)clicked_cell.x / 3 == col / 3 && (uint8_t)clicked_cell.y / 3 == row / 3)) {
			draw->AddRectFilled(upper_left, lower_right, colors::Colors_array[colors::Cell_highlite]);
		}
		//Normal empty cell on top
		draw->AddRect(upper_left, lower_right, colors::Colors_array[colors::Cell_boarder], 0, 0, variables::table_cell_thickness);

		ImVec2 center{ cell_size / 2 + upper_left.x, cell_size / 2 + upper_left.y };

		if (Suduko_table.at(row)[col] != 0) {	//Show number
			char num[2]{};
			sprintf_s(num, 2, "%u", Suduko_table.at(row).at(col));

			ImVec2 num_pos{ center.x - fonts::number_size / 4, center.y - fonts::number_size / 2 };

			push_num_font;
			if (Suduko_table.at(row).at(col) != Suduko_table.at_solved(row).at(col))
				draw->AddText(num_pos, colors::Colors_array[colors::Nums_wrong], num);
			else if (Suduko_table.at_seed(row).at(col) != 0)
				draw->AddText(num_pos, colors::Colors_array[colors::Nums_seed], num);
			//else if (cell_is_hoverd)
			//	draw->AddText(num_pos, __Nums_hoverd_color, num);
			else
				draw->AddText(num_pos, colors::Colors_array[colors::Nums_correct], num);
			pop_font;
		}
		else {	//Show Notes
			if (!Suduko_table.at_notes(row).at(col).empty())
				draw_notes(upper_left, lower_right);

		}

		if (variables::key_pressed && !game_over) {	//can modify table when game is not over
			change_from_keyboard();
			//It will become true by the callback and wont become false for some reason so  need to make it false right after
			variables::key_pressed = false;
		}

		if (!game_over && cell_clicked(upper_left, lower_right)) {
			if (clicked_cell.x == col && clicked_cell.y == row) {	//clicking on an active cell will deactivate it
				//Showing that no cell is currently selected
				clicked_cell.x = 10; clicked_cell.y = 10;
			}
			else {
				clicked_cell.x = col;
				clicked_cell.y = row;
			}
		}
	}
	else {
		//Normal empty cell
		draw->AddRect(upper_left, lower_right, colors::Colors_array[colors::Cell_boarder], 0, 0, variables::table_cell_thickness);
	}
}

void Suduko_Game::draw_notes(ImVec2 upper_left, ImVec2 lower_right) {
	char num[2]{};
	unsigned row = upper_left.y / cell_size;
	unsigned col = upper_left.x / cell_size;

	std::vector<num_t>& note = Suduko_table.at_notes(row).at(col);

	ImVec2 num_pos{};

	push_note_font;
	for (unsigned i{}; i < 3; i++) {
		for (unsigned j{}; j < 3 && (i*3+j)< note.size(); j++) {

			num_pos.x = upper_left.x + j * (cell_size / 3) + (cell_size / 6) - fonts::note_size / 4;
			num_pos.y = upper_left.y + i * (cell_size / 3) + (cell_size / 6) - fonts::note_size / 2;

			sprintf_s(num, 2, "%u", note.at(i * 3 + j));

			draw->AddText(num_pos, colors::Colors_array[colors::Nums_seed], num);
		}
	}
	pop_font;
}

void Suduko_Game::draw_block_borders() {

	for (size_t vertical{ 3 }; vertical < 9; vertical+=3) {
		ImVec2 v_start{ vertical * cell_size + edge_offset, edge_offset };
		ImVec2 v_end{ vertical * cell_size + edge_offset, edge_offset + 9*cell_size };
		draw->AddLine(v_start, v_end, colors::Colors_array[colors::Cell_boarder], variables::table_boarder_thickness);
	}

	for (size_t horizantal{ 3 }; horizantal < 9; horizantal += 3) {
		ImVec2 h_start{ edge_offset, horizantal * cell_size + edge_offset };
		ImVec2 h_end{ edge_offset + 9 * cell_size, horizantal * cell_size + edge_offset };
		draw->AddLine(h_start, h_end, colors::Colors_array[colors::Cell_boarder], variables::table_boarder_thickness);
	}
}

bool Suduko_Game::cell_hoverd(ImVec2 cell_upper_left, ImVec2 cell_lower_right) {
	if (Cursor_in_cell(cell_upper_left, cell_lower_right)) {
		return true;
	}
	return false;
}

bool Suduko_Game::cell_clicked(ImVec2 cell_upper_left, ImVec2 cell_lower_right) {
	static bool last_click_released{ true };

	if(!last_click_released)	//using this variable to prevent click jitter
		last_click_released = !variables::mouse_clicked;	//wait for last click to be released
	
	if (Cursor_in_cell(cell_upper_left, cell_lower_right) && variables::mouse_clicked && last_click_released) {
		last_click_released = false;
		return true;
	}
	
	return false;
}

void Suduko_Game::change_from_keyboard() {
	//static bool last_click_released{ true };
	
	//meaning that no cell is currently selected
	if (clicked_cell.x > 9 || clicked_cell.y > 9)
		return;

	const int zero_ascii{ ImGuiKey_0 };
	const int keypad_zero_ascii{ ImGuiKey_Keypad0 };

	std::vector<num_t>& ClickedNote = Suduko_table.at_notes(clicked_cell.y).at(clicked_cell.x);
	num_t& ClickedNum = Suduko_table.at(clicked_cell.y).at(clicked_cell.x);
	
	//Modifing table numbers
	int key_int{ variables::keyboard_key - keypad_zero_ascii };
	key_int = (key_int > 0 && key_int < 10) ? key_int : variables::keyboard_key - zero_ascii;	//Keypad numbers or keyboard numbers

	if (key_int > 0 && key_int < 10) {
		if (!take_notes) {	//Type number
			if (Suduko_table.at_seed(clicked_cell.y).at(clicked_cell.x) == 0) {	//Cannot modify seed numbers

				//Pushing current state of the game in undo list, before changing anything
				_Undo_struct current_state{ false, {clicked_cell.y, clicked_cell.x}, ClickedNum, 0 };
				if (__Undo_list.size() == __Max_Undo) {
					__Undo_list.pop_front();	//pop front to make place for new undos
				}
				__Undo_list.push_back(current_state);

				ClickedNum = key_int;

				current_game_is_saved = false;	//Any modification to table makes the last save obsolete

				//if the number was correct update notes
				if (ClickedNum == Suduko_table.at_solved(clicked_cell.y).at(clicked_cell.x)) {
					Suduko_table.Update_Notes(clicked_cell.y, clicked_cell.x, key_int);

					//Correct number will delete the notes in that cell
					ClickedNote.clear();
					ClickedNote.shrink_to_fit();

					if (Suduko_table.Is_Solved()) {
						variables::game_complete = true;
					}
					__Remaining--;
				}
				//If not add a mistake
				else {
					++__Mistakes;
				}
			}
		}
		else if (ClickedNum == 0) {	//Type Note in cell if a number isn't there
			auto it = std::find(ClickedNote.begin(), ClickedNote.end(), key_int);

			current_game_is_saved = false;	//Any modification to table makes the last save obsolete

			//Pushing current state of the game before changing anything
			_Undo_struct current_state{ true, {clicked_cell.y, clicked_cell.x}, 0, 0 };
			if (it != ClickedNote.end()) {
				current_state.removed_note = *it;
				ClickedNote.erase(it);
			}	
			else {	//Max 9 note in a single cell bec we have 9 number in total!
				current_state.num = key_int;
				ClickedNote.push_back(key_int);
			}

			if (__Undo_list.size() == __Max_Undo) {
				__Undo_list.pop_front();	//pop front to make place for new undos
			}
			__Undo_list.push_back(current_state);
				
		}

		//last_click_released = false;
	}

	//Deleting a number or deleting the last note added
	if (variables::keyboard_key == ImGuiKey_Backspace || variables::keyboard_key == ImGuiKey_Delete) {

		if (ClickedNum == 0) {
			if (!ClickedNote.empty()) {	//Pop a note if its not empty

				_Undo_struct current_state{ true, {clicked_cell.y, clicked_cell.x}, 0, ClickedNote.back() };
				if (__Undo_list.size() == __Max_Undo) {
					__Undo_list.pop_front();	//pop front to make place for new undos
				}
				__Undo_list.push_back(current_state);

				ClickedNote.pop_back();
			}
		}
		else {	//Deleting a number

			//Pushing current state of the game in undo list, before changing anything
			_Undo_struct current_state{ false,{clicked_cell.y, clicked_cell.x}, ClickedNum, 0 };
			if (__Undo_list.size() == __Max_Undo) {
				__Undo_list.pop_front();	//pop front to make place for new undos
			}
			__Undo_list.push_back(current_state);
			ClickedNum = 0;
		}
	}
	else if (variables::keyboard_key == ImGuiKey_N) {
		take_notes = !take_notes;
	}
	else if (variables::keyboard_key == ImGuiKey_H) {
		if (__Hints < __Max_Hints) {
			give_hint();
			__Hints++;
		}
	}
	else if (variables::keyboard_key == ImGuiKey_RightArrow) {
		if(clicked_cell.x < 8)
			++clicked_cell.x;
	}
	else if (variables::keyboard_key == ImGuiKey_LeftArrow) {
		if (clicked_cell.x > 0)
			--clicked_cell.x;
	}
	else if (variables::keyboard_key == ImGuiKey_UpArrow) {
		if (clicked_cell.y > 0)
			--clicked_cell.y;
	}
	else if (variables::keyboard_key == ImGuiKey_DownArrow) {
		if (clicked_cell.y < 8)
			++clicked_cell.y;
	}
}


Suduko_Game::~Suduko_Game() {
	if (!current_game_is_saved) {	//if the game was still running and the save wasnt up to date make another save
		save_last_game();	//checking game over in here
	}
	functions::save_game_settings();
}


bool Suduko_Game::save_last_game() {
	std::filesystem::path _path{ path::game_save_path / path::last_save_filename };

	std::fstream out_file;
	bool __Flag = std::filesystem::exists(_path);
	if (!__Flag) {	//if save file doesn't exist creat it
		out_file.open(_path, std::ios::out | std::ios::trunc | std::ios::binary);
		out_file.close();
	}

	out_file.open(_path, std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);
	if (!out_file.is_open())
		return false;

	if (game_over) {	//if the game is over, no need to save
		return true;
	}
	_Save_struct save{};
	game_to_save_struct(save);

	out_file.write(reinterpret_cast<char*>(&save), sizeof(save));

	out_file.close();
	return true;
}

bool Suduko_Game::load_last_game() {
	std::filesystem::path _path{ path::game_save_path / path::last_save_filename };

	std::ifstream in_file;
	bool __Flag = std::filesystem::exists(_path);
	if (!__Flag) {	//save file doesn't exist
		return false;
	}

	in_file.open(_path, std::ios::binary);
	if (!in_file.is_open())
		return false;

	_Save_struct save{};
	in_file.read(reinterpret_cast<char*>(&save), sizeof(save));
	if (!in_file) {	//if there was nothing to read
		in_file.close();
		return false;
	}
		
	save_struct_to_game(save);
	
	__starting_time = std::chrono::steady_clock::now();
	__Remaining = Suduko_table.Remaining();

	in_file.close();
	return true;
}

void Suduko_Game::game_to_save_struct(_Save_struct& _save) {
	_save.Difficulty = static_cast<uint8_t>(Suduko_table.Get_difficulty());
	_save.Mistakes = __Mistakes;
	_save.Hints = __Hints;
	_save.Time = __Time;	//__Time has the time offset in it

	for (size_t i{}; i < 9; i++) {
		for (size_t j{}; j < 9; j++) {

			_save.Grid[i][j] = Suduko_table.at(i).at(j);
			_save.Seed[i][j] = Suduko_table.at_seed(i).at(j);

			for (size_t k{}; k < Suduko_table.at_notes(i).at(j).size(); k++) {
				_save.Notes[i][j][k] = Suduko_table.at_notes(i).at(j).at(k);
			}
		}
	}
}

void Suduko_Game::save_struct_to_game(_Save_struct& _save) {
	Suduko_table.Set_difficulty(static_cast<Sudoku::game_difficulty>(_save.Difficulty));

	 __Mistakes = _save.Mistakes;
	 if (__Mistakes > 3) {
		 _Max_Mistakes = 4;
		 _second_chance = true;
	 }
	 __Time = _save.Time;
	 __Hints = _save.Hints;

	for (size_t i{}; i < 9; i++) {
		for (size_t j{}; j < 9; j++) {
			 Suduko_table.at(i).at(j) = _save.Grid[i][j];
			 Suduko_table.at_seed(i).at(j) = _save.Seed[i][j];
			 Suduko_table.at_solved(i).at(j) = _save.Seed[i][j];
		}
	}

	Suduko_table.Solve();	//to generate solved_grid - clears Notes
	Suduko_table.Clear_Notes();
	
	for (size_t i{}; i < 9; i++) {
		for (size_t j{}; j < 9; j++) {
			std::vector<num_t>& _N = Suduko_table.at_notes(i).at(j);
			for (size_t k{}; k < 9; k++) {
				if(_save.Notes[i][j][k] != 0)
					_N.push_back(_save.Notes[i][j][k]);
			}
		}
	}
}

bool Suduko_Game::save_seed(Sudoku::game_difficulty diff) {
	uint8_t seed_count{};

	std::fstream fs;
	std::string Path{ "Seeds/" };
	if(!std::filesystem::exists("Seeds"))
		std::filesystem::create_directory("Seeds");

	switch (diff) {
	case Sudoku::Simple:
		Path += "SudokuSeedsSimple.seed";
		break;
	case Sudoku::Easy:
		Path += "SudokuSeedsEasy.seed";
		break;
	case Sudoku::Normal:
		Path += "SudokuSeedsNormal.seed";
		break;
	case Sudoku::Hard:
		Path += "SudokuSeedsHard.seed";
		break;
	case Sudoku::Expert:
		Path += "SudokuSeedsExpert.seed";
		break;
	}

	fs.open(Path, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	if (!fs.is_open()) {
		fs.open(Path, std::ios::trunc | std::ios::out);	//Creating new
		fs.close();
		fs.open(Path, std::ios::in | std::ios::out | std::ios::binary | std::ios::ate);
	}

	if (!fs.is_open()) {
		return false;
	}

	auto last_out_pos = fs.tellp();
	fs.seekg(0, std::ios::beg);
	uint16_t count{};
	fs >> count;
	if (!fs) {
		fs.clear();
		count = 0U;

		fs.seekp(0, std::ios::beg);

		fs.width(4);
		fs << count;

		last_out_pos = fs.tellp();
	}
	++count;

	fs.seekp(0, std::ios::beg);

	fs.width(4);
	fs << count;

	fs.seekp(last_out_pos);

	Seed_save_struct save{};
	for (uint8_t i{}; i < 9; i++) {
		for (uint8_t j{}; j < 9; j++) {

			save.Seed[i][j] = Suduko_table.at_seed(i).at(j);
			save.Solve[i][j] = Suduko_table.at_solved(i).at(j);
		}
	}

	fs.write(reinterpret_cast<char*>(&save), sizeof(save));

	fs.close();
	return true;
}

bool Suduko_Game::load_seed(Sudoku::game_difficulty diff) {
	uint16_t seed_count{};

	std::ifstream fs;
	std::string Path{ "Seeds/" };

	switch (diff) {
	case Sudoku::Simple:
		Path += "SudokuSeedsSimple.seed";
		break;
	case Sudoku::Easy:
		Path += "SudokuSeedsEasy.seed";
		break;
	case Sudoku::Normal:
		Path += "SudokuSeedsNormal.seed";
		break;
	case Sudoku::Hard:
		Path += "SudokuSeedsHard.seed";
		break;
	case Sudoku::Expert:
		Path += "SudokuSeedsExpert.seed";
		break;
	}

	fs.open(Path, std::ios::in | std::ios::binary);
	if (!fs.is_open()) {
		return false;
	}

	fs >> seed_count;
	if (!fs) {
		return false;
	}

	Seed_save_struct save{};
	uint8_t random_seed = rand() % seed_count + 1;	//1 read or higher
	for (uint8_t i{}; i < random_seed; i++) {
		fs.read(reinterpret_cast<char*>(&save), sizeof(save));
	}
	fs.close();

	Suduko_table.New_Game(save.Seed, save.Solve, diff);

	return true;
}


void functions::clear_user_saves() {
	std::filesystem::path _path{ path::game_save_path / path::user_save_filename };

	std::fstream out_file;
	bool __Flag = std::filesystem::exists(_path);
	if (__Flag) {
		out_file.open(_path, std::ios::out | std::ios::trunc);
		out_file.close();
	}
}

bool functions::save_game(_Save_struct& _save) {
	std::filesystem::path _path{ path::game_save_path / path::user_save_filename };

	std::fstream out_file;
	bool __Flag = std::filesystem::exists(_path);
	if (!__Flag) {	//if save file doesn't exist creat it
		out_file.open(_path, std::ios::out | std::ios::trunc | std::ios::binary);
		out_file.close();
	}

	out_file.open(_path, std::ios::out | std::ios::in | std::ios::binary | std::ios::app);
	if (!out_file.is_open())
		return false;

	out_file.write(reinterpret_cast<char*>(&_save), sizeof(_save));

	out_file.close();
	return true;
}

bool functions::load_saves(std::vector<_Save_struct>& _saves) {
	std::filesystem::path _path{ path::game_save_path / path::user_save_filename };

	std::ifstream in_file;
	bool __Flag = std::filesystem::exists(_path);
	if (!__Flag) {	//save file doesn't exist
		return false;
	}

	in_file.open(_path, std::ios::binary);
	if (!in_file.is_open())
		return false;

	_saves.clear();	//Clear so dont spam push backs
	_Save_struct save{};

	while (!in_file.eof()) {
		in_file.read(reinterpret_cast<char*>(&save), sizeof(save));

		if (!in_file) {	//if there was nothing to read
			break;
		}
		else {
			_saves.push_back(save);
		}
	}

	in_file.close();
	return true;
}

void functions::make_document_path() {
	wchar_t user_name[100]{};
	DWORD username_len = 100;
	bool __Flag = GetUserName(user_name, &username_len);

	wchar_t __game_save_path[50]{ L"C:\\Users\\" };
	wcscat_s(__game_save_path, 50, user_name);
	wcscat_s(__game_save_path, 50, L"\\Documents\\Suduko");

	std::filesystem::path _path{ __game_save_path };
	__Flag = std::filesystem::exists(_path);
	if (!__Flag) {	//if directory doesn't exist creat one
		__Flag = std::filesystem::create_directories(__game_save_path);
	}

	path::game_save_path = __game_save_path;
}

bool functions::update_user_save() {
	std::vector<_Save_struct> saves{};
	if (!load_saves(saves))
		return false;
	clear_user_saves();	//Clear only after successfuly copied

	for (size_t i{}; i < saves.size(); i++) {
		if (strcmp(saves.at(i).Save_name, variables::loadded_game_save.Save_name) == 0) {
			auto it = saves.erase(saves.begin() + i);
			saves.emplace(saves.begin(), variables::loadded_game_save);	//put it at first
			for (auto& s : saves)
				save_game(s);
			return true;
		}
	}

	return false;
}

bool functions::load_game_settings() {
	//Game setting file next to .exe file
	std::ifstream in_file{ path::game_setting_filename, std::ios::binary };

	if (!in_file.is_open()) {	//if save file doesn't exist
		return false;
	}

	Setting_Save_struct save{};
	in_file.read(reinterpret_cast<char*>(&save), sizeof(save));
	variables::game_can_resume_dont_ask = save.game_can_resume_ask;
	variables::new_game_dont_ask = save.new_game_ask;
	variables::generate_or_load = save.generate_or_load;

	in_file.close();
	return true;
}

bool functions::save_game_settings() {
	//Game setting file next to .exe file
	std::ofstream out_file{ path::game_setting_filename, std::ios::binary };

	if (!out_file.is_open()) {	//if save file doesn't exist creat it
		out_file.open(path::game_setting_filename, std::ios::trunc | std::ios::binary);

		if (!out_file.is_open())
			return false;
	}

	Setting_Save_struct save{ variables::game_can_resume_dont_ask, variables::new_game_dont_ask };
	save.generate_or_load = variables::generate_or_load;
	out_file.write(reinterpret_cast<char*>(&save), sizeof(save));

	out_file.close();
	return true;
}

void functions::show_overlay(bool save_update) {

	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

	const float PAD = 10.0f;
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImVec2 work_pos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
	ImVec2 work_size = viewport->WorkSize;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = work_pos.x + PAD;
	window_pos.y = work_pos.y + PAD;

	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always);
	window_flags |= ImGuiWindowFlags_NoMove;


	ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background

	if (ImGui::Begin("Example: Simple overlay", nullptr, window_flags))
	{
		push_num_font;
		if(save_update)
			ImGui::Text("Save Updated");
		else
			ImGui::Text("Game Puased");
		pop_font;
	}

	ImGui::End();
}