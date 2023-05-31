#pragma once

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"
#include <d3d9.h>
#include <tchar.h>

class ImGui_DX9
{
public:
	ImGui_DX9(const wchar_t *win_name, const ImU32 Bg_col);

	void Run();

	virtual void Update() {}

	virtual ~ImGui_DX9();
protected:
	HWND hwnd;
	WNDCLASSEXW wc;

	ImVec2 Win_size;
	static constexpr float Win_X_to_Y_ratio{ 1.45f };
	static constexpr float Win_Y_to_Screen_Y{ 0.6f };
	
	ImVec4 Bg_color;
};

