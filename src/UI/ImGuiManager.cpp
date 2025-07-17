//src/UI/ImGuiManager.cpp
#include "ImGuiManager.hpp"
#include <format>

//�O����Win32 ���b�Z�[�W�n���h���[
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine::UI
{
	//================================================================
	//ImGuiManager����
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

		//ImGui�R���e�L�X�g�쐬
		IMGUI_CHECKVERSION();
		m_context = ImGui::CreateContext();
		ImGui::SetCurrentContext(m_context);

		//�ݒ�
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	}

}