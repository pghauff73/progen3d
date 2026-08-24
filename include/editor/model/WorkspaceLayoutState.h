#pragma once

class WorkspaceLayoutState
{
public:
	float editor_width_ratio = 0.54f;
	float preview_height_ratio = 0.82f;
	float console_height_ratio = 0.18f;
	bool preview_fullscreen = false;
	bool authentication_panel_visible = false;
	float editor_font_scale = 1.0f;
};
