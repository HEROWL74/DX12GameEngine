// src/Core/Window.hpp
#pragma once

#include <Windows.h>
#include <string>
#include <functional>
#include <memory>
#include "../Utils/Common.hpp"
#include "../Input/InputManager.hpp"
namespace Engine::UI {
    class ImGuiManager;
}

namespace Engine::Core {
    // =============================================================================
   // �E�B���h�E�ݒ�\����
   // =============================================================================

    // �E�B���h�E�̍쐬�ݒ�
    struct WindowSettings
    {
        std::wstring title = L"Game Engine"; // �E�B���h�E�^�C�g��
        int width = 1280;// ��
        int height = 720;// ����
        int x = CW_USEDEFAULT; // x���W
        int y = CW_USEDEFAULT;// y���W
        bool resizable = true; // ���T�C�Y�ł��邩�ł��Ȃ���
        bool fullScreen = false; // �t���X�N���[���������ł͂Ȃ���
    };

    // =============================================================================
 // �E�B���h�E�C�x���g�̃R�[���o�b�N�^
 // =============================================================================

    // �E�B���h�E�����T�C�Y���ꂽ���̃R�[���o�b�N
    using ResizeCallback = std::function<void(int width, int height)>;

    // �E�B���h�E�������鎞�̃R�[���o�b�N
    using CloseCallback = std::function<void()>;

    // =============================================================================
   // Window�N���X
   // =============================================================================

    // �E�B���h�E�̍쐬�ƊǗ�����������N���X
    class Window
    {
    public:
        // �R���X�g���N�^
        Window() = default;

        // �f�X�g���N�^
        ~Window();

        // �R�s�[�֎~
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;

        //  ���[�u�͂ł���悤�ɂ���
        Window(Window&&) noexcept;
        Window& operator=(Window&&) noexcept;

        // �E�B���h�E���쐬
        // �A�v���P�[�V�����C���X�^���X
        // �E�B���h�E�ݒ�
        // ��������void�A���s����Error
        [[nodiscard]] Utils::VoidResult create(HINSTANCE hInstance, const WindowSettings& settings);

        // �E�B���h�E��\��
        // nCmdShow�\��
        void show(int nCmdShow) const noexcept;

        // �E�B���h�E���b�Z�[�W������
        // �E�B���h�E�n���h��
        [[nodiscard]] bool processMessages() const noexcept;

        // �E�B���h�E�n���h�����擾
        // �E�B���h�E�n���h��
        [[nodiscard]] HWND getHandle() const noexcept { return m_handle; }

        [[nodiscard]] std::pair<int, int> getClientSize() const noexcept;

        [[nodiscard]] bool isValid() const noexcept { return m_handle != nullptr; }

        void setTitle(std::wstring_view title)const noexcept;

        void setResizeCallback(ResizeCallback callback) noexcept { m_resizeCallback = std::move(callback); }

        void setCloseCallback(CloseCallback callback) noexcept { m_closeCallback = std::move(callback); }

        void setImGuiManager(UI::ImGuiManager* manager) { m_imguiManager = manager; }

        // ���̓V�X�e���ւ̃A�N�Z�X
        [[nodiscard]] Input::InputManager* getInputManager() const noexcept;

    private:
        HWND m_handle = nullptr;
        std::wstring m_className;
        HINSTANCE m_hInstance = nullptr;

        // ���̓V�X�e��
        std::unique_ptr<Input::InputManager> m_inputManager;
        UI::ImGuiManager* m_imguiManager = nullptr;

        // �C�x���g�R�[���o�b�N
        ResizeCallback m_resizeCallback;
        CloseCallback m_closeCallback;

        [[nodiscard]] Utils::VoidResult registerWindowClass(HINSTANCE hInstance);

        static LRESULT CALLBACK staticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        LRESULT windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        void destroy() noexcept;

    };
}
