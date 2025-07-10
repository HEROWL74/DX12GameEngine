// src/Input/MouseState.hpp
#pragma once

#include <Windows.h>
#include <cstdint>

namespace Engine::Input
{
    // �}�E�X�{�^���̏�Ԃ�\���񋓌^
    enum class MouseButton : uint8_t
    {
        Left = 0,
        Right = 1,
        Middle = 2,
        X1 = 3,     // �T�C�h�{�^��1
        X2 = 4,     // �T�C�h�{�^��2
        Count = 5   // �{�^����
    };

    // �}�E�X�̌��ݏ�Ԃ�ێ�����\����
    struct MouseState
    {
        // ���݂̉�ʍ��W�ʒu
        int x = 0;
        int y = 0;

        // �O�t���[������̈ړ���
        int deltaX = 0;
        int deltaY = 0;

        // �z�C�[���̉�]�ʁi�����j
        float wheelDelta = 0.0f;

        // �����z�C�[���̉�]�ʁi�Ή��}�E�X�̂݁j
        float horizontalWheelDelta = 0.0f;

        // �e�{�^���̉������
        bool buttonStates[static_cast<size_t>(MouseButton::Count)] = { false };

        // �O�t���[���̊e�{�^���̉������
        bool prevButtonStates[static_cast<size_t>(MouseButton::Count)] = { false };

        // �}�E�X���E�B���h�E���ɂ��邩�ǂ���
        bool isInWindow = false;

        // �}�E�X�L���v�`�������ǂ���
        bool isCaptured = false;

        // �}�E�X�����΃��[�h�iFPS�Q�[���p�j���ǂ���
        bool isRelativeMode = false;

        // ����X�V�t���O�idelta�v�Z�p�j
        bool firstUpdate = true;

        // �f�t�H���g�R���X�g���N�^
        MouseState() = default;

        // �R�s�[�R���X�g���N�^�ƃR�s�[������Z�q
        MouseState(const MouseState& other) = default;
        MouseState& operator=(const MouseState& other) = default;

        // ���[�u�R���X�g���N�^�ƃ��[�u������Z�q
        MouseState(MouseState&& other) noexcept = default;
        MouseState& operator=(MouseState&& other) noexcept = default;

        // �w�肵���{�^����������Ă��邩�`�F�b�N
        bool isButtonDown(MouseButton button) const
        {
            size_t index = static_cast<size_t>(button);
            return index < static_cast<size_t>(MouseButton::Count) && buttonStates[index];
        }

        // �w�肵���{�^�������t���[���ŉ����ꂽ���`�F�b�N
        bool isButtonPressed(MouseButton button) const
        {
            size_t index = static_cast<size_t>(button);
            return index < static_cast<size_t>(MouseButton::Count) &&
                buttonStates[index] && !prevButtonStates[index];
        }

        // �w�肵���{�^�������t���[���ŗ����ꂽ���`�F�b�N
        bool isButtonReleased(MouseButton button) const
        {
            size_t index = static_cast<size_t>(button);
            return index < static_cast<size_t>(MouseButton::Count) &&
                !buttonStates[index] && prevButtonStates[index];
        }

        // �O�t���[���̏�Ԃ�ۑ�
        void savePreviousState()
        {
            for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i)
            {
                prevButtonStates[i] = buttonStates[i];
            }
        }

        // �{�^����Ԃ�ݒ�
        void setButtonState(MouseButton button, bool pressed)
        {
            size_t index = static_cast<size_t>(button);
            if (index < static_cast<size_t>(MouseButton::Count))
            {
                buttonStates[index] = pressed;
            }
        }

        // �ʒu��ݒ肵�ăf���^���v�Z
        void setPosition(int newX, int newY)
        {
            if (firstUpdate)
            {
                deltaX = 0;
                deltaY = 0;
                firstUpdate = false;
            }
            else
            {
                deltaX = newX - x;
                deltaY = newY - y;
            }

            x = newX;
            y = newY;
        }

        // �z�C�[���l��ݒ�
        void setWheelDelta(float vertical, float horizontal = 0.0f)
        {
            wheelDelta = vertical;
            horizontalWheelDelta = horizontal;
        }

        // ��Ԃ����Z�b�g
        void reset()
        {
            deltaX = 0;
            deltaY = 0;
            wheelDelta = 0.0f;
            horizontalWheelDelta = 0.0f;

            for (size_t i = 0; i < static_cast<size_t>(MouseButton::Count); ++i)
            {
                buttonStates[i] = false;
                prevButtonStates[i] = false;
            }

            isInWindow = false;
            isCaptured = false;
            isRelativeMode = false;
            firstUpdate = true;
        }
    };

    // �}�E�X�{�^���𕶎���ɕϊ��i�f�o�b�O�p�j
    inline const char* mouseButtonToString(MouseButton button)
    {
        switch (button)
        {
        case MouseButton::Left: return "Left";
        case MouseButton::Right: return "Right";
        case MouseButton::Middle: return "Middle";
        case MouseButton::X1: return "X1";
        case MouseButton::X2: return "X2";
        default: return "Unknown";
        }
    }

    // Win32�̃}�E�X�{�^�����b�Z�[�W��MouseButton�ɕϊ�
    inline MouseButton win32ToMouseButton(UINT message, WPARAM wParam = 0)
    {
        switch (message)
        {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
            return MouseButton::Left;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
            return MouseButton::Right;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
            return MouseButton::Middle;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
            // wParam��HIWORD��X1/X2�𔻒�
            return (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? MouseButton::X1 : MouseButton::X2;
        default:
            return static_cast<MouseButton>(255);
        }
    }
}