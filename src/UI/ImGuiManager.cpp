//src/UI/ImGuiManager.cpp
#include "ImGuiManager.hpp"
#include <format>

//外部のWin32 メッセージハンドラー
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine::UI
{
	//================================================================
	//ImGuiManager実装
	//================================================================
	ImGuiManager::~ImGuiManager()
	{
		shutdown();
	}

	Utils::VoidResult ImGuiManager::initialize(Graphics::Device* device, HWND hwnd, DXGI_FORMAT rtvFormat, UINT frameCount)
	{
		if (m_initialized)
		{
			return {};
		}

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

		m_device = device;
		m_frameCount = frameCount;

		Utils::log_info("Initializing ImGui...");

		//ImGuiコンテキスト作成
		IMGUI_CHECKVERSION();
		m_context = ImGui::CreateContext();
		ImGui::SetCurrentContext(m_context);

		//設定
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

}