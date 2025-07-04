// src/main.cpp
#include <Windows.h>
#include <iostream>
#include "Core/App.hpp"
#include "Utils/Common.hpp"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    using namespace Engine;

    // �R���\�[���E�B���h�E���쐬�i�f�o�b�O���O�\���p�j
#ifdef _DEBUG
    AllocConsole();
    freopen_s(reinterpret_cast<FILE**>(stdout), "CONOUT$", "w", stdout);
    freopen_s(reinterpret_cast<FILE**>(stderr), "CONOUT$", "w", stderr);
    std::cout << "=== DX12 Game Engine Debug Console ===" << std::endl;
#endif

    // �A�v���P�[�V�����̍쐬
    Core::App app;

    // �A�v���P�[�V�����̏�����
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

    // ���C�����[�v�̎��s
    const int exitCode = app.run();

#ifdef _DEBUG
    std::cout << "Application exited with code: " << exitCode << std::endl;
    FreeConsole();
#endif

    return exitCode;
}