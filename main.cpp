//main.cpp
#include <Windows.h>
#include "Src/Core/App.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	using namespace Core;
	App app;

	if (!app.initialize(hInstance, nCmdShow)) {
		return -1;
	}

	return app.run();
}