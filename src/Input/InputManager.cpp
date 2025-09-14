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

        // 蛻晄悄迥ｶ諷九ｒ繝ｪ繧ｻ繝・ヨ
        m_keyStates.fill(false);
        m_prevKeyStates.fill(false);
        m_mouseState.reset();

        // 繧ｦ繧｣繝ｳ繝峨え縺ｮ荳ｭ蠢・ｺｧ讓吶ｒ險育ｮ・
        calculateWindowCenter();

        // Raw Input 繧呈怏蜉ｹ蛹厄ｼ磯ｫ倡ｲｾ蠎ｦ繝槭え繧ｹ蜈･蜉帷畑・・
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

        // 蠢・★逶ｸ蟇ｾ繝｢繝ｼ繝峨ｒ隗｣髯､
        setRelativeMouseMode(false);

        // Raw Input繧堤┌蜉ｹ蛹・
        setRawMouseInput(false);

        // 繝槭え繧ｹ繧ｭ繝｣繝励メ繝｣繧定ｧ｣謾ｾ
        if (m_mouseCaptured)
        {
            ReleaseCapture();
            m_mouseCaptured = false;
        }

        // 繧ｫ繝ｼ繧ｽ繝ｫ繧定｡ｨ遉ｺ
        showCursor(true);

        // 繧ｷ繧ｹ繝・Β縺ｮ繧ｫ繝ｼ繧ｽ繝ｫ菴咲ｽｮ繧貞ｾｩ蜈・ｼ亥ｿｵ縺ｮ縺溘ａ・・
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

        // 蜑阪ヵ繝ｬ繝ｼ繝縺ｮ迥ｶ諷九ｒ菫晏ｭ・
        m_prevKeyStates = m_keyStates;
        m_mouseState.savePreviousState();

        // 繧ｭ繝ｼ繝懊・繝臥憾諷九ｒ譖ｴ譁ｰ
        updateKeyboardState();

        // 繝槭え繧ｹ迥ｶ諷九ｒ譖ｴ譁ｰ
        updateMouseState();

        // 繝輔Ξ繝ｼ繝蝗ｺ譛峨・繝・・繧ｿ繧偵Μ繧ｻ繝・ヨ
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

        // ShowCursor()縺ｯ蜀・Κ繧ｫ繧ｦ繝ｳ繧ｿ繝ｼ繧剃ｽｿ逕ｨ縺吶ｋ縺溘ａ縲・
        // 遒ｺ螳溘↓陦ｨ遉ｺ/髱櫁｡ｨ遉ｺ縺ｫ縺吶ｋ縺溘ａ縺ｫ驕ｩ蛻・↓蛻ｶ蠕｡
        if (show)
        {
            // 繧ｫ繝ｼ繧ｽ繝ｫ繧定｡ｨ遉ｺ縺吶ｋ縺ｾ縺ｧShowCursor(TRUE)繧堤ｹｰ繧願ｿ斐☆
            int count = ShowCursor(TRUE);
            while (count < 0)
            {
                count = ShowCursor(TRUE);
            }
        }
        else
        {
            // 繧ｫ繝ｼ繧ｽ繝ｫ繧帝撼陦ｨ遉ｺ縺ｫ縺吶ｋ縺ｾ縺ｧShowCursor(FALSE)繧堤ｹｰ繧願ｿ斐☆
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

            // 1. 繧ｦ繧｣繝ｳ繝峨え荳ｭ螟ｮ菴咲ｽｮ繧定ｨ育ｮ・
            calculateWindowCenter();

            // 2. 繝槭え繧ｹ繧ｭ繝｣繝励メ繝｣繧貞・縺ｫ險ｭ螳・
            captureMouse(true);

            // 3. Raw Input繧呈怏蜉ｹ蛹・
            setRawMouseInput(true);

            // 4. 繝槭え繧ｹ繧剃ｸｭ螟ｮ縺ｫ遘ｻ蜍・
            setMousePosition(m_windowCenter.x, m_windowCenter.y);

            // 5. 譛蠕後↓繧ｫ繝ｼ繧ｽ繝ｫ繧帝撼陦ｨ遉ｺ
            showCursor(false);
        }
        else
        {
            Utils::log_info("Disabling relative mouse mode");

            // 1. 譛蛻昴↓繧ｫ繝ｼ繧ｽ繝ｫ繧定｡ｨ遉ｺ
            showCursor(true);

            // 2. Raw Input繧堤┌蜉ｹ蛹・
            setRawMouseInput(false);

            // 3. 繝槭え繧ｹ繧ｭ繝｣繝励メ繝｣繧定ｧ｣髯､
            captureMouse(false);

            // 4. 繝・Ν繧ｿ繧偵Μ繧ｻ繝・ヨ
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

        // 謚ｼ縺輔ｌ縺ｦ縺・ｋ繧ｭ繝ｼ縺ｮ諠・ｱ
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
        // Windows API縺ｧ繧ｭ繝ｼ繝懊・繝臥憾諷九ｒ逶ｴ謗･蜿門ｾ・
        BYTE keyboardState[256];
        if (GetKeyboardState(keyboardState))
        {
            for (size_t i = 0; i < MAX_KEYS; ++i)
            {
                m_keyStates[i] = (keyboardState[i] & 0x80) != 0;
            }
        }
    }

    // InputManager.cpp 縺ｮ updateMouseState 繧剃ｿｮ豁｣
    void InputManager::updateMouseState()
    {
        if (!m_initialized)
        {
            return;
        }

        // 逶ｸ蟇ｾ繝｢繝ｼ繝峨〒縺ｪ縺・ｴ蜷医・縺ｿ騾壼ｸｸ縺ｮ菴咲ｽｮ蜿門ｾ・
        if (!m_relativeMode)
        {
            POINT cursorPos;
            if (GetCursorPos(&cursorPos))
            {
                ScreenToClient(m_windowHandle, &cursorPos);
                m_mouseState.setPosition(cursorPos.x, cursorPos.y);
            }
        }
        // 逶ｸ蟇ｾ繝｢繝ｼ繝峨・蝣ｴ蜷医ヽaw Input縺九ｉ縺ｮ繝・Ν繧ｿ縺ｮ縺ｿ繧剃ｽｿ逕ｨ

        // 繧ｦ繧｣繝ｳ繝峨え蜀・愛螳・
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
        // 繝帙う繝ｼ繝ｫ繝・・繧ｿ繧偵Μ繧ｻ繝・ヨ
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
            rid.dwFlags = RIDEV_INPUTSINK;  // 繧｢繧ｯ繝・ぅ繝悶〒縺ｪ縺・凾繧ょ女菫｡
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

        // 迥ｶ諷九ｒ譖ｴ譁ｰ
        size_t index = keyCodeToIndex(keyCode);
        m_keyStates[index] = isPressed;

        // 繧ｳ繝ｼ繝ｫ繝舌ャ繧ｯ蜻ｼ縺ｳ蜃ｺ縺・
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
            // 繝槭え繧ｹ繝懊ち繝ｳ繝｡繝・そ繝ｼ繧ｸ縺ｮ蜃ｦ逅・
            MouseButton button = win32ToMouseButton(message, wParam);  // wParam繧よｸ｡縺・
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
                // 逕溘・繝槭え繧ｹ遘ｻ蜍暮㍼繧貞叙蠕・
                int deltaX = raw.data.mouse.lLastX;
                int deltaY = raw.data.mouse.lLastY;

                // 諢溷ｺｦ繧帝←逕ｨ・域紛謨ｰ縺ｧ險育ｮ励＠縺ｦ縺九ｉ險ｭ螳夲ｼ・
                float adjustedDeltaX = deltaX * m_mouseSensitivity;
                float adjustedDeltaY = deltaY * m_mouseSensitivity;

                // 繝・Ν繧ｿ繧堤峩謗･險ｭ螳・
                m_mouseState.deltaX = static_cast<int>(adjustedDeltaX);
                m_mouseState.deltaY = static_cast<int>(adjustedDeltaY);

                // 逶ｸ蟇ｾ繝｢繝ｼ繝峨〒縺ｯ逕ｻ髱｢荳ｭ螟ｮ繧堤ｶｭ謖・
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
