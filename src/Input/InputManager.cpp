// src/Input/InputManager.cpp
#include "InputManager.hpp"
#include "../Utils/Common.hpp"
#include <format>
#include <algorithm>

namespace Engine::Input
{
    InputManager::InputManager() = default;

    InputManager::~InputManager()
    {
        shutdown();
    }

    void InputManager::initialize(HWND windowHandle)
    {
        if (m_initialized)
        {
            Utils::log_warning("InputManager already initialized");
            return;
        }

        m_windowHandle = windowHandle;
        if (!m_windowHandle)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Invalid window handle"));
            return;
        }

        // ������Ԃ����Z�b�g
        m_keyStates.fill(false);
        m_prevKeyStates.fill(false);
        m_mouseState.reset();

        // �E�B���h�E�̒��S���W���v�Z
        calculateWindowCenter();

        // Raw Input ��L�����i�����x�}�E�X���͗p�j
        setRawMouseInput(true);

        m_initialized = true;
        Utils::log_info("InputManager initialized successfully");
    }

    void InputManager::shutdown()
    {
        if (!m_initialized)
        {
            return;
        }

        // �K�����΃��[�h������
        setRelativeMouseMode(false);

        // Raw Input�𖳌���
        setRawMouseInput(false);

        // �}�E�X�L���v�`�������
        if (m_mouseCaptured)
        {
            ReleaseCapture();
            m_mouseCaptured = false;
        }

        // �J�[�\����\��
        showCursor(true);

        // �V�X�e���̃J�[�\���ʒu�𕜌��i�O�̂��߁j
        POINT centerPoint;
        centerPoint.x = GetSystemMetrics(SM_CXSCREEN) / 2;
        centerPoint.y = GetSystemMetrics(SM_CYSCREEN) / 2;
        SetCursorPos(centerPoint.x, centerPoint.y);

        m_initialized = false;
        m_windowHandle = nullptr;

        Utils::log_info("InputManager shutdown complete");
    }

    void InputManager::update()
    {
        if (!m_initialized)
        {
            return;
        }

        // �O�t���[���̏�Ԃ�ۑ�
        m_prevKeyStates = m_keyStates;
        m_mouseState.savePreviousState();

        // �L�[�{�[�h��Ԃ��X�V
        updateKeyboardState();

        // �}�E�X��Ԃ��X�V
        updateMouseState();

        // �t���[���ŗL�̃f�[�^�����Z�b�g
        resetFrameData();
    }

    bool InputManager::isKeyDown(KeyCode keyCode) const
    {
        if (!isValidKeyCode(keyCode))
        {
            return false;
        }

        size_t index = keyCodeToIndex(keyCode);
        return m_keyStates[index];
    }

    bool InputManager::isKeyPressed(KeyCode keyCode) const
    {
        if (!isValidKeyCode(keyCode))
        {
            return false;
        }

        size_t index = keyCodeToIndex(keyCode);
        return m_keyStates[index] && !m_prevKeyStates[index];
    }

    bool InputManager::isKeyReleased(KeyCode keyCode) const
    {
        if (!isValidKeyCode(keyCode))
        {
            return false;
        }

        size_t index = keyCodeToIndex(keyCode);
        return !m_keyStates[index] && m_prevKeyStates[index];
    }

    bool InputManager::isShiftDown() const
    {
        return isKeyDown(KeyCode::LeftShift) || isKeyDown(KeyCode::RightShift);
    }

    bool InputManager::isCtrlDown() const
    {
        return isKeyDown(KeyCode::LeftCtrl) || isKeyDown(KeyCode::RightCtrl);
    }

    bool InputManager::isAltDown() const
    {
        return isKeyDown(KeyCode::LeftAlt) || isKeyDown(KeyCode::RightAlt);
    }

    bool InputManager::isMouseButtonDown(MouseButton button) const
    {
        return m_mouseState.isButtonDown(button);
    }

    bool InputManager::isMouseButtonPressed(MouseButton button) const
    {
        return m_mouseState.isButtonPressed(button);
    }

    bool InputManager::isMouseButtonReleased(MouseButton button) const
    {
        return m_mouseState.isButtonReleased(button);
    }

    void InputManager::setMousePosition(int x, int y)
    {
        if (!m_initialized)
        {
            return;
        }

        POINT screenPoint = { x, y };
        ClientToScreen(m_windowHandle, &screenPoint);
        SetCursorPos(screenPoint.x, screenPoint.y);
    }
    void InputManager::showCursor(bool show)
    {
        if (m_cursorVisible == show)
        {
            return;
        }

        m_cursorVisible = show;

        // ShowCursor()�͓����J�E���^�[���g�p���邽�߁A
        // �m���ɕ\��/��\���ɂ��邽�߂ɓK�؂ɐ���
        if (show)
        {
            // �J�[�\����\������܂�ShowCursor(TRUE)���J��Ԃ�
            int count = ShowCursor(TRUE);
            while (count < 0)
            {
                count = ShowCursor(TRUE);
            }
        }
        else
        {
            // �J�[�\�����\���ɂ���܂�ShowCursor(FALSE)���J��Ԃ�
            int count = ShowCursor(FALSE);
            while (count >= 0)
            {
                count = ShowCursor(FALSE);
            }
        }

        Utils::log_info(std::format("Cursor visibility set to: {}", show));
    }

    void InputManager::captureMouse(bool capture)
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_mouseCaptured == capture)
        {
            return;
        }

        if (capture)
        {
            SetCapture(m_windowHandle);
            m_mouseCaptured = true;
        }
        else
        {
            ReleaseCapture();
            m_mouseCaptured = false;
        }
    }

    
    void InputManager::setRelativeMouseMode(bool relative)
    {
        if (!m_initialized)
        {
            return;
        }

        if (m_relativeMode == relative)
        {
            return;
        }

        m_relativeMode = relative;
        m_mouseState.isRelativeMode = relative;

        if (relative)
        {
            Utils::log_info("Enabling relative mouse mode");

            // 1. �E�B���h�E�����ʒu���v�Z
            calculateWindowCenter();

            // 2. �}�E�X�L���v�`�����ɐݒ�
            captureMouse(true);

            // 3. Raw Input��L����
            setRawMouseInput(true);

            // 4. �}�E�X�𒆉��Ɉړ�
            setMousePosition(m_windowCenter.x, m_windowCenter.y);

            // 5. �Ō�ɃJ�[�\�����\��
            showCursor(false);
        }
        else
        {
            Utils::log_info("Disabling relative mouse mode");

            // 1. �ŏ��ɃJ�[�\����\��
            showCursor(true);

            // 2. Raw Input�𖳌���
            setRawMouseInput(false);

            // 3. �}�E�X�L���v�`��������
            captureMouse(false);

            // 4. �f���^�����Z�b�g
            m_mouseState.deltaX = 0;
            m_mouseState.deltaY = 0;
        }
    }

    bool InputManager::handleWindowMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (!m_initialized || hWnd != m_windowHandle)
        {
            return false;
        }

        switch (message)
        {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            return handleKeyboardMessage(message, wParam, lParam);

        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL:
            return handleMouseMessage(message, wParam, lParam);

        case WM_INPUT:
            return handleRawInput(lParam);

        default:
            return false;
        }
    }

    std::string InputManager::getDebugInfo() const
    {
        std::string info = "InputManager Debug Info:\n";
        info += std::format("Initialized: {}\n", m_initialized);
        info += std::format("Mouse Position: ({}, {})\n", m_mouseState.x, m_mouseState.y);
        info += std::format("Mouse Delta: ({}, {})\n", m_mouseState.deltaX, m_mouseState.deltaY);
        info += std::format("Mouse Captured: {}\n", m_mouseCaptured);
        info += std::format("Relative Mode: {}\n", m_relativeMode);
        info += std::format("Cursor Visible: {}\n", m_cursorVisible);

        // ������Ă���L�[�̏��
        info += "Pressed Keys: ";
        for (size_t i = 0; i < MAX_KEYS; ++i)
        {
            if (m_keyStates[i])
            {
                info += std::format("{} ", static_cast<int>(i));
            }
        }
        info += "\n";

        return info;
    }

    void InputManager::updateKeyboardState()
    {
        // Windows API�ŃL�[�{�[�h��Ԃ𒼐ڎ擾
        BYTE keyboardState[256];
        if (GetKeyboardState(keyboardState))
        {
            for (size_t i = 0; i < MAX_KEYS; ++i)
            {
                m_keyStates[i] = (keyboardState[i] & 0x80) != 0;
            }
        }
    }

    // InputManager.cpp �� updateMouseState ���C��
    void InputManager::updateMouseState()
    {
        if (!m_initialized)
        {
            return;
        }

        // ���΃��[�h�łȂ��ꍇ�̂ݒʏ�̈ʒu�擾
        if (!m_relativeMode)
        {
            POINT cursorPos;
            if (GetCursorPos(&cursorPos))
            {
                ScreenToClient(m_windowHandle, &cursorPos);
                m_mouseState.setPosition(cursorPos.x, cursorPos.y);
            }
        }
        // ���΃��[�h�̏ꍇ�ARaw Input����̃f���^�݂̂��g�p

        // �E�B���h�E������
        RECT clientRect;
        if (GetClientRect(m_windowHandle, &clientRect))
        {
            m_mouseState.isInWindow = (m_mouseState.x >= clientRect.left &&
                m_mouseState.x < clientRect.right &&
                m_mouseState.y >= clientRect.top &&
                m_mouseState.y < clientRect.bottom);
        }
    }

    void InputManager::resetFrameData()
    {
        // �z�C�[���f�[�^�����Z�b�g
        m_mouseState.wheelDelta = 0.0f;
        m_mouseState.horizontalWheelDelta = 0.0f;
    }

    void InputManager::calculateWindowCenter()
    {
        if (!m_initialized)
        {
            return;
        }

        RECT clientRect;
        if (GetClientRect(m_windowHandle, &clientRect))
        {
            m_windowCenter.x = (clientRect.right - clientRect.left) / 2;
            m_windowCenter.y = (clientRect.bottom - clientRect.top) / 2;
        }
    }

    void InputManager::setRawMouseInput(bool enable)
    {
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01;  // Generic Desktop Controls
        rid.usUsage = 0x02;      // Mouse

        if (enable)
        {
            rid.dwFlags = RIDEV_INPUTSINK;  // �A�N�e�B�u�łȂ�������M
            rid.hwndTarget = m_windowHandle;
        }
        else
        {
            rid.dwFlags = RIDEV_REMOVE;
            rid.hwndTarget = nullptr;
        }

        if (!RegisterRawInputDevices(&rid, 1, sizeof(rid)))
        {
            DWORD error = GetLastError();
            Utils::log_warning(std::format("Failed to register raw input device: {}", error));
        }
    }

    bool InputManager::handleKeyboardMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        KeyCode keyCode = virtualKeyToKeyCode(wParam);
        if (keyCode == KeyCode::None)
        {
            return false;
        }

        bool isPressed = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN);
        bool wasPressed = isKeyDown(keyCode);

        // ��Ԃ��X�V
        size_t index = keyCodeToIndex(keyCode);
        m_keyStates[index] = isPressed;

        // �R�[���o�b�N�Ăяo��
        if (isPressed && !wasPressed && m_keyPressedCallback)
        {
            m_keyPressedCallback(keyCode);
        }
        else if (!isPressed && wasPressed && m_keyReleasedCallback)
        {
            m_keyReleasedCallback(keyCode);
        }

        return true;
    }

    bool InputManager::handleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);

        switch (message)
        {
        case WM_MOUSEMOVE:
            if (m_mouseMoveCallback)
            {
                m_mouseMoveCallback(x, y, m_mouseState.deltaX, m_mouseState.deltaY);
            }
            return true;

        case WM_MOUSEWHEEL:
        {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            m_mouseState.setWheelDelta(delta);
            if (m_mouseWheelCallback)
            {
                m_mouseWheelCallback(delta, x, y);
            }
        }
        return true;

        case WM_MOUSEHWHEEL:
        {
            float delta = GET_WHEEL_DELTA_WPARAM(wParam) / 120.0f;
            m_mouseState.setWheelDelta(0.0f, delta);
        }
        return true;

        default:
            // �}�E�X�{�^�����b�Z�[�W�̏���
            MouseButton button = win32ToMouseButton(message, wParam);  // wParam���n��
            if (button != static_cast<MouseButton>(255))
            {
                bool isPressed = (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
                    message == WM_MBUTTONDOWN || message == WM_XBUTTONDOWN);

                bool wasPressed = m_mouseState.isButtonDown(button);
                m_mouseState.setButtonState(button, isPressed);

                if (isPressed && !wasPressed && m_mouseButtonPressedCallback)
                {
                    m_mouseButtonPressedCallback(button, x, y);
                }
                else if (!isPressed && wasPressed && m_mouseButtonReleasedCallback)
                {
                    m_mouseButtonReleasedCallback(button, x, y);
                }
                return true;
            }
            break;
        }

        return false;
    }

    bool InputManager::handleRawInput(LPARAM lParam)
    {
        UINT dwSize = sizeof(RAWINPUT);
        static RAWINPUT raw;

        if (GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &raw, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize)
        {
            if (raw.header.dwType == RIM_TYPEMOUSE && m_relativeMode)
            {
                // ���̃}�E�X�ړ��ʂ��擾
                int deltaX = raw.data.mouse.lLastX;
                int deltaY = raw.data.mouse.lLastY;

                // ���x��K�p�i�����Ōv�Z���Ă���ݒ�j
                float adjustedDeltaX = deltaX * m_mouseSensitivity;
                float adjustedDeltaY = deltaY * m_mouseSensitivity;

                // �f���^�𒼐ڐݒ�
                m_mouseState.deltaX = static_cast<int>(adjustedDeltaX);
                m_mouseState.deltaY = static_cast<int>(adjustedDeltaY);

                // ���΃��[�h�ł͉�ʒ������ێ�
                setMousePosition(m_windowCenter.x, m_windowCenter.y);

                return true;
            }
        }

        return false;
    }

    bool InputManager::isValidKeyCode(KeyCode keyCode) const
    {
        uint32_t code = static_cast<uint32_t>(keyCode);
        return code > 0 && code < MAX_KEYS;
    }

    size_t InputManager::keyCodeToIndex(KeyCode keyCode) const
    {
        return static_cast<size_t>(keyCode);
    }

    KeyCode InputManager::virtualKeyToKeyCode(WPARAM vkCode) const
    {
        if (vkCode >= 0 && vkCode < MAX_KEYS)
        {
            return static_cast<KeyCode>(vkCode);
        }
        return KeyCode::None;
    }
}