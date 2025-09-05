// src/main.cpp
#include <Windows.h>
#include <iostream>
#include "Core/App.hpp"
#include "Utils/Common.hpp"

int WINAPI WinMain(_In_ HINSTANCE hInstance,_In_opt_ HINSTANCE,_In_ LPSTR, _In_ int nCmdShow)
{
    using namespace Engine;

    // コンソールウィンドウを作成（デバッグログ表示用）
#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
    std::cout << "=== DX12 Game Engine Debug Console ===" << std::endl;
#endif

    // アプリケーションの作成
    Core::App app;

    // アプリケーションの初期化
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

    // メインループの実行
    const int exitCode = app.run();

#ifdef _DEBUG
    std::cout << "Application exited with code: " << exitCode << std::endl;
    FreeConsole();
#endif

    return exitCode;
}