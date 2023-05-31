#include "Sudoku/Suduko.h"

int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
)
{
	Suduko_Game game{ L"Sudoku" };
	game.Add_fonts_style();
	game.Run();
	return 0;
}