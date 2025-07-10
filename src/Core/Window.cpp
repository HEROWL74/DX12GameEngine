// src/Core/Window.cpp
#include "Window.hpp"
#include <format>

namespace Engine::Core {
	Window::~Window()
	{
		destroy();
	}

	Window::Window(Window&& other) noexcept
		:m_handle(other.m_handle)
		, m_className(std::move(other.m_className))
		, m_hInstance(other.m_hInstance)
		, m_inputManager(std::move(other.m_inputManager))
		, m_resizeCallback(std::move(other.m_resizeCallback))
		, m_closeCallback(std::move(other.m_closeCallback))
	{
		other.m_handle = nullptr;
		other.m_hInstance = nullptr;
	}

	Window& Window::operator=(Window&& other) noexcept
	{
		if (this != &other)
		{
			destroy();

			m_handle = other.m_handle;
			m_className = std::move(other.m_className);
			m_hInstance = other.m_hInstance;
			m_inputManager = std::move(other.m_inputManager);
			m_resizeCallback = std::move(other.m_resizeCallback);
			m_closeCallback = std::move(other.m_closeCallback);

			other.m_handle = nullptr;
			other.m_hInstance = nullptr;

		}
		return *this;
	}

	// �p�u���b�N���\�b�h

	Utils::VoidResult Window::create(HINSTANCE hInstance, const WindowSettings& settings)
	{
		m_hInstance = hInstance;

		// �E�B���h�E�N���X��o�^����
		auto result = registerWindowClass(hInstance);
		if (!result)
		{
			return result;
		}

		// �E�B���h�E�X�^�C��������
		DWORD windowStyle = WS_OVERLAPPEDWINDOW;
		if (!settings.resizable)
		{
			windowStyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}

		// �N���C�A���g�̈�̃T�C�Y����E�B���h�E�T�C�Y���v�Z
		RECT windowRect = { 0,0,settings.width, settings.height };
		AdjustWindowRect(&windowRect, windowStyle, FALSE);

		const int windowWidth = windowRect.right - windowRect.left;
		const int windowHeight = windowRect.bottom - windowRect.top;

		// �E�B���h�E�쐬
		m_handle = CreateWindowExW(
			0,
			m_className.c_str(),
			settings.title.c_str(),
			windowStyle,
			settings.x,
			settings.y,
			windowWidth,
			windowHeight,
			nullptr,
			nullptr,
			hInstance,
			this
		);

		CHECK_CONDITION(m_handle != nullptr, Utils::ErrorType::WindowCreation,
			"Failed to create window");

		// ���̓V�X�e����������
		m_inputManager = std::make_unique<Input::InputManager>();
		m_inputManager->initialize(m_handle);

		Utils::log_info(std::format("Window created: {}x{}", settings.width, settings.height));

		return {};
	}

	void Window::show(int nCmdShow) const noexcept
	{
		if (m_handle)
		{
			ShowWindow(m_handle, nCmdShow);
			UpdateWindow(m_handle);
		}
	}

	bool Window::processMessages() const noexcept
	{
		MSG msg{};
		while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				return false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		// ���̓V�X�e�����X�V
		if (m_inputManager)
		{
			m_inputManager->update();
		}

		return true;
	}

	std::pair<int, int> Window::getClientSize() const noexcept
	{
		if (!m_handle)
		{
			return { 0,0 };
		}

		RECT clientRect;
		GetClientRect(m_handle, &clientRect);
		return { clientRect.right - clientRect.left, clientRect.bottom - clientRect.top };
	}

	void Window::setTitle(std::wstring_view title) const noexcept
	{
		if (m_handle)
		{
			SetWindowTextW(m_handle, title.data());
		}
	}

	Input::InputManager* Window::getInputManager() const noexcept
	{
		return m_inputManager.get();
	}

	// �v���C�x�[�g���\�b�h
	Utils::VoidResult Window::registerWindowClass(HINSTANCE hInstance)
	{
		// ���j�[�N�ȃN���X���𐶐�
		m_className = std::format(L"GameEngineWindow_{}",
			reinterpret_cast<uintptr_t>(this));

		WNDCLASSEXW wcex{};
		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = staticWindowProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = hInstance;
		wcex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
		wcex.lpszMenuName = nullptr;
		wcex.lpszClassName = m_className.c_str();
		wcex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

		const ATOM atom = RegisterClassExW(&wcex);
		CHECK_CONDITION(atom != 0, Utils::ErrorType::WindowCreation,
			"Failed to register window class");

		return {};
	}

	LRESULT CALLBACK Window::staticWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		Window* window = nullptr;

		if (uMsg == WM_NCCREATE)
		{
			// �E�B���h�E�쐬���ɓn���ꂽthis�|�C���^���擾
			CREATESTRUCTW* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
			window = static_cast<Window*>(createStruct->lpCreateParams);
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
		}
		else
		{
			// ���ɐݒ肳�ꂽthis�|�C���^���擾
			window = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		}

		if (window)
		{
			return window->windowProc(hWnd, uMsg, wParam, lParam);
		}

		return DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}

	LRESULT Window::windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		// ���̓V�X�e���Ƀ��b�Z�[�W��]��
		if (m_inputManager && m_inputManager->handleWindowMessage(hWnd, uMsg, wParam, lParam))
		{
			return 0;
		}

		switch (uMsg)
		{
		case WM_SIZE:
		{
			const int width = LOWORD(lParam);
			const int height = HIWORD(lParam);

			if (m_resizeCallback && width > 0 && height > 0)
			{
				m_resizeCallback(width, height);
			}
		}
		break;

		case WM_ACTIVATE:
		{
			// �E�B���h�E���A�N�e�B�u�łȂ��Ȃ����瑊�΃��[�h������
			if (LOWORD(wParam) == WA_INACTIVE && m_inputManager)
			{
				if (m_inputManager->getMouseState().isRelativeMode)
				{
					m_inputManager->setRelativeMouseMode(false);
					Utils::log_info("Window lost focus - disabled relative mouse mode");
				}
			}
		}
		break;

		case WM_KILLFOCUS:
		{
			// �t�H�[�J�X���������������΃��[�h������
			if (m_inputManager && m_inputManager->getMouseState().isRelativeMode)
			{
				m_inputManager->setRelativeMouseMode(false);
				Utils::log_info("Window lost focus - disabled relative mouse mode");
			}
		}
		break;

		case WM_CLOSE:
		{
			// �I���O�ɑ��΃��[�h������
			if (m_inputManager)
			{
				m_inputManager->setRelativeMouseMode(false);
			}
			if (m_closeCallback)
			{
				m_closeCallback();
			}
			PostQuitMessage(0);
		}
		break;

		case WM_KEYDOWN:
		{
			// Alt+F4�ł̏I��������
			if (wParam == VK_F4 && (GetKeyState(VK_MENU) & 0x8000))
			{
				if (m_inputManager)
				{
					m_inputManager->setRelativeMouseMode(false);
				}
				PostQuitMessage(0);
				return 0;
			}
			// ESC�L�[�ł̏I��
			else if (wParam == VK_ESCAPE)
			{
				if (m_inputManager)
				{
					m_inputManager->setRelativeMouseMode(false);
				}
				PostQuitMessage(0);
				return 0;
			}
		}
		break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;

		default:
			return DefWindowProcW(hWnd, uMsg, wParam, lParam);
		}

		return 0;
	}

	void Window::destroy() noexcept
	{
		// ���̓V�X�e�����V���b�g�_�E��
		if (m_inputManager)
		{
			m_inputManager->shutdown();
			m_inputManager.reset();
		}

		if (m_handle)
		{
			DestroyWindow(m_handle);
			m_handle = nullptr;
		}

		if (m_hInstance && !m_className.empty())
		{
			UnregisterClassW(m_className.c_str(), m_hInstance);
			m_className.clear();
		}

		m_hInstance = nullptr;
	}
}