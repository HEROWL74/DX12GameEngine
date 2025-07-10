// src/Input/InputManager.hpp
#pragma once

#include <Windows.h>
#include <windowsx.h>
#include <array>
#include <memory>
#include <functional>
#include <unordered_map>
#include <string>
#include "KeyCode.hpp"
#include "MouseState.hpp"


namespace Engine::Input
{
    // ���̓C�x���g�̃R�[���o�b�N�^��`
    using KeyPressedCallback = std::function<void(KeyCode keyCode)>;
    using KeyReleasedCallback = std::function<void(KeyCode keyCode)>;
    using MouseButtonCallback = std::function<void(MouseButton button, int x, int y)>;
    using MouseMoveCallback = std::function<void(int x, int y, int deltaX, int deltaY)>;
    using MouseWheelCallback = std::function<void(float delta, int x, int y)>;

    // ���̓V�X�e���̒��j�N���X
    // �L�[�{�[�h�ƃ}�E�X�̓��͂𓝍��I�ɊǗ�
    class InputManager
    {
    public:
        // �ő哯�������\�L�[��
        static constexpr size_t MAX_KEYS = 256;

        InputManager();
        ~InputManager();

        // �R�s�[�E���[�u�֎~�i�V���O���g���I�Ȏg�p��z��j
        InputManager(const InputManager&) = delete;
        InputManager& operator=(const InputManager&) = delete;
        InputManager(InputManager&&) = delete;
        InputManager& operator=(InputManager&&) = delete;

        // �������ƏI������
        void initialize(HWND windowHandle);
        void shutdown();

        // �t���[���X�V����
        void update();

        // �L�[�{�[�h���͊֘A
        bool isKeyDown(KeyCode keyCode) const;
        bool isKeyPressed(KeyCode keyCode) const;
        bool isKeyReleased(KeyCode keyCode) const;

        // �C���L�[�̏�Ԏ擾
        bool isShiftDown() const;
        bool isCtrlDown() const;
        bool isAltDown() const;

        // �}�E�X���͊֘A
        const MouseState& getMouseState() const { return m_mouseState; }
        bool isMouseButtonDown(MouseButton button) const;
        bool isMouseButtonPressed(MouseButton button) const;
        bool isMouseButtonReleased(MouseButton button) const;

        // �}�E�X�ʒu�擾
        int getMouseX() const { return m_mouseState.x; }
        int getMouseY() const { return m_mouseState.y; }
        int getMouseDeltaX() const { return m_mouseState.deltaX; }
        int getMouseDeltaY() const { return m_mouseState.deltaY; }

        // �}�E�X�z�C�[���擾
        float getMouseWheelDelta() const { return m_mouseState.wheelDelta; }

        // �}�E�X����
        void setMousePosition(int x, int y);
        void showCursor(bool show);
        void captureMouse(bool capture);
        void setRelativeMouseMode(bool relative);

        // �C�x���g�R�[���o�b�N�ݒ�
        void setKeyPressedCallback(KeyPressedCallback callback) { m_keyPressedCallback = callback; }
        void setKeyReleasedCallback(KeyReleasedCallback callback) { m_keyReleasedCallback = callback; }
        void setMouseButtonPressedCallback(MouseButtonCallback callback) { m_mouseButtonPressedCallback = callback; }
        void setMouseButtonReleasedCallback(MouseButtonCallback callback) { m_mouseButtonReleasedCallback = callback; }
        void setMouseMoveCallback(MouseMoveCallback callback) { m_mouseMoveCallback = callback; }
        void setMouseWheelCallback(MouseWheelCallback callback) { m_mouseWheelCallback = callback; }

        // Win32���b�Z�[�W�n���h���iWindow.cpp����Ăяo�����j
        bool handleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

        // �f�o�b�O���擾
        std::string getDebugInfo() const;

        // �ݒ�
        void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
        float getMouseSensitivity() const { return m_mouseSensitivity; }

        // �L�����`�F�b�N
        bool isInitialized() const { return m_initialized; }

    private:
        // �������
        bool m_initialized = false;
        HWND m_windowHandle = nullptr;
        float m_mouseSensitivity = 1.0f;

        // �L�[�{�[�h���
        std::array<bool, MAX_KEYS> m_keyStates = { false };
        std::array<bool, MAX_KEYS> m_prevKeyStates = { false };

        // �}�E�X���
        MouseState m_mouseState;

        // �J�[�\������
        bool m_cursorVisible = true;
        bool m_mouseCaptured = false;

        // ���΃}�E�X���[�h�p
        POINT m_windowCenter = { 0, 0 };
        bool m_relativeMode = false;

        // �C�x���g�R�[���o�b�N
        KeyPressedCallback m_keyPressedCallback;
        KeyReleasedCallback m_keyReleasedCallback;
        MouseButtonCallback m_mouseButtonPressedCallback;
        MouseButtonCallback m_mouseButtonReleasedCallback;
        MouseMoveCallback m_mouseMoveCallback;
        MouseWheelCallback m_mouseWheelCallback;

        // �������\�b�h
        void updateKeyboardState();
        void updateMouseState();
        void resetFrameData();
        void calculateWindowCenter();
        void setRawMouseInput(bool enable);

        // Win32���b�Z�[�W�n���h���i�����j
        bool handleKeyboardMessage(UINT message, WPARAM wParam, LPARAM lParam);
        bool handleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);
        bool handleRawInput(LPARAM lParam);

        // ���[�e�B���e�B���\�b�h
        bool isValidKeyCode(KeyCode keyCode) const;
        size_t keyCodeToIndex(KeyCode keyCode) const;
        KeyCode virtualKeyToKeyCode(WPARAM vkCode) const;
    };
}