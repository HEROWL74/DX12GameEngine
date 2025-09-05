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

        // 初期状態をリセット
        m_keyStates.fill(false);
        m_prevKeyStates.fill(false);
        m_mouseState.reset();

        // ウィンドウの中心座標を計算
        calculateWindowCenter();

        // Raw Input を有効化（高精度マウス入力用）
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

        // 必ず相対モードを解除
        setRelativeMouseMode(false);

        // Raw Inputを無効化
        setRawMouseInput(false);

        // マウスキャプチャを解放
        if (m_mouseCaptured)
        {
            ReleaseCapture();
            m_mouseCaptured = false;
        }

        // カーソルを表示
        showCursor(true);

        // システムのカーソル位置を復元（念のため）
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

        // 前フレームの状態を保存
        m_prevKeyStates = m_keyStates;
        m_mouseState.savePreviousState();

        // キーボード状態を更新
        updateKeyboardState();

        // マウス状態を更新
        updateMouseState();

        // フレーム固有のデータをリセット
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

        // ShowCursor()は内部カウンターを使用するため、
        // 確実に表示/非表示にするために適切に制御
        if (show)
        {
            // カーソルを表示するまでShowCursor(TRUE)を繰り返す
            int count = ShowCursor(TRUE);
            while (count < 0)
            {
                count = ShowCursor(TRUE);
            }
        }
        else
        {
            // カーソルを非表示にするまでShowCursor(FALSE)を繰り返す
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

            // 1. ウィンドウ中央位置を計算
            calculateWindowCenter();

            // 2. マウスキャプチャを先に設定
            captureMouse(true);

            // 3. Raw Inputを有効化
            setRawMouseInput(true);

            // 4. マウスを中央に移動
            setMousePosition(m_windowCenter.x, m_windowCenter.y);

            // 5. 最後にカーソルを非表示
            showCursor(false);
        }
        else
        {
            Utils::log_info("Disabling relative mouse mode");

            // 1. 最初にカーソルを表示
            showCursor(true);

            // 2. Raw Inputを無効化
            setRawMouseInput(false);

            // 3. マウスキャプチャを解除
            captureMouse(false);

            // 4. デルタをリセット
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

        // 押されているキーの情報
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
        // Windows APIでキーボード状態を直接取得
        BYTE keyboardState[256];
        if (GetKeyboardState(keyboardState))
        {
            for (size_t i = 0; i < MAX_KEYS; ++i)
            {
                m_keyStates[i] = (keyboardState[i] & 0x80) != 0;
            }
        }
    }

    // InputManager.cpp の updateMouseState を修正
    void InputManager::updateMouseState()
    {
        if (!m_initialized)
        {
            return;
        }

        // 相対モードでない場合のみ通常の位置取得
        if (!m_relativeMode)
        {
            POINT cursorPos;
            if (GetCursorPos(&cursorPos))
            {
                ScreenToClient(m_windowHandle, &cursorPos);
                m_mouseState.setPosition(cursorPos.x, cursorPos.y);
            }
        }
        // 相対モードの場合、Raw Inputからのデルタのみを使用

        // ウィンドウ内判定
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
        // ホイールデータをリセット
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
            rid.dwFlags = RIDEV_INPUTSINK;  // アクティブでない時も受信
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

        // 状態を更新
        size_t index = keyCodeToIndex(keyCode);
        m_keyStates[index] = isPressed;

        // コールバック呼び出し
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
            // マウスボタンメッセージの処理
            MouseButton button = win32ToMouseButton(message, wParam);  // wParamも渡す
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
                // 生のマウス移動量を取得
                int deltaX = raw.data.mouse.lLastX;
                int deltaY = raw.data.mouse.lLastY;

                // 感度を適用（整数で計算してから設定）
                float adjustedDeltaX = deltaX * m_mouseSensitivity;
                float adjustedDeltaY = deltaY * m_mouseSensitivity;

                // デルタを直接設定
                m_mouseState.deltaX = static_cast<int>(adjustedDeltaX);
                m_mouseState.deltaY = static_cast<int>(adjustedDeltaY);

                // 相対モードでは画面中央を維持
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