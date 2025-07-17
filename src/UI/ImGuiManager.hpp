//src/UI/ImGuiManager.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "../Graphics/Device.hpp"
#include "../Utils/Common.hpp"
#include "../Core/GameObject.hpp"
#include "../Graphics/RenderComponent.hpp"

//ImGui includes
#include "imgui.h"
#include "../UI/backends/imgui_impl_dx12.h"
#include "../UI/backends/imgui_impl_win32.h"

using Microsoft::WRL::ComPtr;

namespace Engine::UI
{
	//======================================================================
	//ImGui�N���X
	//======================================================================
	class ImGuiManager
	{
	public:
		ImGuiManager() = default;
		~ImGuiManager();

		//�R�s�[�E���[�u�֎~
		ImGuiManager(const ImGuiManager&) = delete;
		ImGuiManager& operator=(const ImGuiManager&) = delete;
		ImGuiManager(ImGuiManager&&) = delete;
		ImGuiManager& operator=(ImGuiManager&&) = delete;

		//������
		[[nodiscard]] Utils::VoidResult initialize(
			Graphics::Device* device,
			HWND hwnd,
			DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
			UINT frameCount = 2
		);

		//�I������
		void shutdown();

		//�t���[���J�n
		void newFrame();

		//�`��
		void render(ID3D12GraphicsCommandList* commandList);

		//Win32���b�Z�[�W����
		void handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		//�L�����̃`�F�b�N
		[[nodiscard]] bool isInitialized() const { return m_initialized; }

		//ImGui�R���e�L�X�g�擾
		ImGuiContext* getContext() const { return m_context; }

	private:
		bool m_initialized = false;
		ImGuiContext* m_context = nullptr;
		Graphics::Device* m_device = nullptr;

		//DX12�p�̃f�B�X�N���v�^�q�[�v
		ComPtr<ID3D12DescriptorHeap> m_srvDescHeap;
		UINT m_frameCount = 2;

		//�������w���p�[
		[[nodiscard]] Utils::VoidResult createDescriptorHeap();
	}; 

	//======================================================================
	//ImGui�E�B���h�E�̊��N���X
	//======================================================================
	class ImGuiWindow
	{
	public:
		ImGuiWindow(const std::string& title, bool visible = true)
			:m_title(title), m_visible(visible) { }
		virtual ~ImGuiWindow() = default;

		//�`��
		virtual void draw() = 0;

		//�\��/��\��
		void setVisible(bool visible) { m_visible = visible; }
		bool isVisible() const { return m_visible; }

		//�^�C�g��
		const std::string& getTitle() const { return m_title; }
		void setTitle(const std::string& title) { m_title = title; }

	protected:
		std::string m_title;
		bool m_visible;
	};

	//=======================================================================
	//�f�o�b�O���E�B���h�E
	//=======================================================================
	class DebugWindow : public ImGuiWindow
	{
	public:
		DebugWindow() : ImGuiWindow("Debug info") {}

		void draw() override;

		//�f�o�b�O���̐ݒ�
		void setFPS(float fps) { m_fps = fps; }
		void setFrameTime(float frameTime) { m_frameTime = frameTime; }
		void setObjectCount(size_t count) { m_objectCount = count; }

	private:
		float m_fps = 0.0f;
		float m_frameTime = 0.0f;
		size_t m_objectCount = 0;
	};

	//======================================================================
	//�I�u�W�F�N�g�K�w�E�B���h�E
	//======================================================================
	class SceneHierarchyWindow : public ImGuiWindow
	{
	public:
		SceneHierarchyWindow() : ImGuiWindow("Scene Hierarchy") {}

		void draw() override; 

		//�V�[���̐ݒ�
		void setScene(Graphics::Scene* scene) { m_scene = scene; }

	private:
		Graphics::Scene* m_scene = nullptr;
		Core::GameObject* m_selectedObject = nullptr;

		void drawGameObject(Core::GameObject* gameObject);
	};

	//=======================================================================
	//�I�u�W�F�N�g�C���X�y�N�^�[
	//=======================================================================
	class InspectorWindow : public ImGuiWindow
	{
	public:
		InspectorWindow() : ImGuiWindow("Inspector") {}

		void draw() override;

		//�I���I�u�W�F�N�g�̐ݒ�
		void setSelectedObject(Core::GameObject* object) {}

	private:
		Core::GameObject* m_selectedObject = nullptr;

		void drawTransformComponent(Core::Transform* transform);
		void drawRenderComponent(Graphics::RenderComponent* renderComponent);
		
	};
}
