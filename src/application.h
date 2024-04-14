#pragma once

namespace Application{

	extern bool application_active;
	extern bool show_app_dockspace;
	extern bool window_settings;
	extern bool window_3dviewport;
	extern bool window_inspector;
	extern bool window_debug;
	extern bool init;	

	void show_dock_space(bool* p_open);
	void show_window_settings(bool* window_settings);
	void show_window_3dviewport(bool* window_3dviewport);
	void show_window_inspector(bool *window_inspector);
	void show_window_debug(bool *window_debug);
	void render();
}