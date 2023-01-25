
#include "../lib/log.h"

#include "font.dat"
#include "gl.h"
#include "platform.h"

#include <glad/glad.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_sdl.h>

#ifdef _WIN32
#include <ConsoleApi.h>
#include <ShellScalingApi.h>
extern "C" {
__declspec(dllexport) bool NvOptimusEnablement = true;
__declspec(dllexport) bool AmdPowerXpressRequestHighPerformance = true;
}
#else
#include <sys/ioctl.h>
#endif

void Platform::init_console() {
#ifdef _WIN32
	// Set terminal to output UTF-8:
	if (!SetConsoleOutputCP(CP_UTF8)) {
		warn("Could not set output codepage to UTF-8 (error %d).", GetLastError());
	}

	// Set terminal to support ANSI colors:
	HANDLE conout = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD mode;
	GetConsoleMode(conout, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (SetConsoleMode(conout, mode) == 0) {
		warn("Could not enable virtual terminal processing (error %d).", GetLastError());
	}
#endif
}

uint32_t Platform::console_width() {
	uint32_t cols = 0;
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
#else
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	cols = w.ws_col;
#endif
	return cols;
}

void Platform::remove_console() {
#ifdef _WIN32
	FreeConsole();
#endif
}

Platform::Platform() {
	platform_init();
}

Platform::~Platform() {
	platform_shutdown();
}

void Platform::platform_init() {

#ifdef _WIN32
	if (SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) != S_OK)
		warn("Could not set process DPI aware.");
#endif

	if (SDL_Init(SDL_INIT_VIDEO) != 0) {
		die("Could not initialize SDL: %s", SDL_GetError());
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	Vec2 wsize = Vec2(1280, 720);

	window = SDL_CreateWindow("Scotty3D", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
	                          static_cast<int>(wsize.x), static_cast<int>(wsize.y),
	                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
	if (!window) {
		die("Could not create window: %s", SDL_GetError());
	}

	auto context = [&](int major, int minor) {
		if (gl_context) return;
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);
		gl_context = SDL_GL_CreateContext(window);
	};

#ifndef __APPLE__ // >:|
	context(4, 5);
	if (!gl_context) info("Could not create OpenGL 4.5 context, trying 4.1 (%s).", SDL_GetError());
#endif
	context(4, 1);
	if (!gl_context) warn("Could not create OpenGL 4.1 context, trying 3.3 (%s).", SDL_GetError());
	context(3, 3);
	if (!gl_context)
		die("Could not create OpenGL 3.3 context, shutting down (%s).", SDL_GetError());

	SDL_GL_MakeCurrent(window, gl_context);
	if (SDL_GL_SetSwapInterval(-1)) {
		info("Could not enable vsync with late swap: using normal vsync.");
		SDL_GL_SetSwapInterval(1);
	}

	if (!gladLoadGL()) {
		die("Could not load OpenGL functions.");
	}

	keybuf = SDL_GetKeyboardState(nullptr);

	GL::setup();
	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init();
}

void Platform::set_dpi() {

	float dpi;
	if (force_dpi == force_dpi) {
		dpi = force_dpi;
	} else {
		int32_t index = SDL_GetWindowDisplayIndex(window);
		if (index < 0) {
			return;
		}
		if (SDL_GetDisplayDPI(index, nullptr, &dpi, nullptr)) {
			return;
		}
		#ifdef __APPLE__
		//on apple, assume window pixels are 96.0f dpi:
		dpi = 96.0f;
		#endif
	}
	float scale = window_draw().x / window_size().x;
	if (prev_dpi == dpi && prev_scale == scale) return;

	log("Current scale: %f, dpi: %f (adjust with --force-dpi)\n", scale, dpi);

	ImGuiStyle style;
	ImGui::StyleColorsDark(&style);
	style.WindowRounding = 0.0f;
	style.ScaleAllSizes(0.8f * dpi / 96.0f * scale);
	ImGui::GetStyle() = style;

	
	ImGuiIO& IO = ImGui::GetIO();
	ImFontConfig config;
	config.FontDataOwnedByAtlas = false;
	IO.IniFilename = nullptr;
	IO.Fonts->Clear();
	IO.Fonts->AddFontFromMemoryTTF(font_ttf, font_ttf_len, 14.0f * dpi / 96.0f * scale, &config);
	IO.FontGlobalScale = 1.0f / scale;
	IO.Fonts->Build();
	
	ImGui_ImplOpenGL3_DestroyFontsTexture();
	ImGui_ImplOpenGL3_CreateFontsTexture();

	prev_dpi = dpi;
	prev_scale = scale;
}

bool Platform::is_down(SDL_Scancode key) {
	return keybuf[key];
}

void Platform::platform_shutdown() {

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	GL::shutdown();
	SDL_GL_DeleteContext(gl_context);
	SDL_DestroyWindow(window);
	window = nullptr;
	gl_context = nullptr;
	SDL_Quit();
}

void Platform::complete_frame() {

	GL::Framebuffer::bind_screen();
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	SDL_GL_SwapWindow(window);
}

void Platform::begin_frame() {

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void Platform::strcpy(char* dst, const char* src, size_t limit) {
#ifdef _WIN32
	strncpy_s(dst, limit, src, limit - 1);
#else
	strncpy(dst, src, limit - 1);
	dst[limit - 1] = '\0';
#endif
}

void Platform::loop(App& app) {

	bool running = true;
	while (running) {

		set_dpi();
		SDL_Event e;
		while (SDL_PollEvent(&e)) {

			ImGui_ImplSDL2_ProcessEvent(&e);

			switch (e.type) {
			case SDL_QUIT: {
				if (app.quit()) running = false;
			} break;
			}

			app.event(e);
		}

		begin_frame();
		app.render();
		complete_frame();
	}
}

Vec2 Platform::scale(Vec2 pt) {
	return pt * window_draw() / window_size();
}

Vec2 Platform::window_size() {
	int w, h;
	SDL_GetWindowSize(window, &w, &h);
	return Vec2(w, h);
}

Vec2 Platform::window_draw() {
	int w, h;
	SDL_GL_GetDrawableSize(window, &w, &h);
	return Vec2(w, h);
}

void Platform::grab_mouse() {
	SDL_SetWindowGrab(window, SDL_TRUE);
}

void Platform::ungrab_mouse() {
	SDL_SetWindowGrab(window, SDL_FALSE);
}

Vec2 Platform::get_mouse() {
	int x, y;
	SDL_GetMouseState(&x, &y);
	return Vec2(x, y);
}

void Platform::capture_mouse() {
	SDL_CaptureMouse(SDL_TRUE);
	SDL_SetRelativeMouseMode(SDL_TRUE);
}

void Platform::release_mouse() {
	SDL_CaptureMouse(SDL_FALSE);
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void Platform::set_mouse(Vec2 pos) {
	SDL_WarpMouseInWindow(window, static_cast<int32_t>(pos.x), static_cast<int32_t>(pos.y));
}
