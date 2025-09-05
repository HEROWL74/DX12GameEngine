// src/main.cpp
#include <Windows.h>
#include <iostream>
#include "Core/App.hpp"
#include "Utils/Common.hpp"

int WINAPI WinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE,_In_ LPSTR, _In_ int nCmdShow)
{
    using namespace Engine;

    // 繧ｳ繝ｳ繧ｽ繝ｼ繝ｫ繧ｦ繧｣繝ｳ繝峨え繧剃ｽ懈・・医ョ繝舌ャ繧ｰ繝ｭ繧ｰ陦ｨ遉ｺ逕ｨ・・
#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
    std::cout << "=== DX12 Game Engine Debug Console ===" << std::endl;
#endif

    // 繧｢繝励Μ繧ｱ繝ｼ繧ｷ繝ｧ繝ｳ縺ｮ菴懈・
    Core::App app;

    // 繧｢繝励Μ繧ｱ繝ｼ繧ｷ繝ｧ繝ｳ縺ｮ蛻晄悄蛹・
    auto initResult = app.initialize(hInstance, nCmdShow);
    if (!initResult)
    {
        Utils::log_error(initResult.error());

#ifdef _DEBUG
        std::cerr << "Initialization failed: " << initResult.error().what() << std::endl;
        std::cerr << "Press any key to exit..." << std::endl;
        std::cin.get();
#endif

        return -1;
    }

    // 繝｡繧､繝ｳ繝ｫ繝ｼ繝励・螳溯｡・
    const int exitCode = app.run();

#ifdef _DEBUG
    std::cout << "Application exited with code: " << exitCode << std::endl;
    FreeConsole();
#endif

    return exitCode;
}
