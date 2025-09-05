//src/UI/ImGuiManager.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include <functional>
#include "../Graphics/Device.hpp"
#include "../Utils/Common.hpp"
#include "../Core/GameObject.hpp"
#include "../Graphics/RenderComponent.hpp"
#include "../Graphics/Material.hpp"
#include "../Graphics/Texture.hpp"
#include "ContextMenu.hpp"

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
			ID3D12CommandQueue* commandQueue,  // �R�}���h�L���[��3�ԖڂɈړ�
			DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
			UINT frameCount = 2
		);

		Utils::VoidResult createFontTextureManually();

		//�I������
		void shutdown();

		//�t���[���J�n
		void newFrame();

		//�`��
		void render(ID3D12GraphicsCommandList* commandList) const;

		//Win32���b�Z�[�W����
		void handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const;

		// ���T�C�Y�����i���P�Łj
		void onWindowResize(int width, int height);

		//�L�����̃`�F�b�N
		[[nodiscard]] bool isInitialized() const { return m_initialized; }

		//ImGui�R���e�L�X�g�擾
		ImGuiContext* getContext() const { return m_context; }

		// ���T�C�Y�����ʒm�i�V�����ǉ��j
		void invalidateDeviceObjects();
		void createDeviceObjects();

	private:
		bool m_initialized = false;
		ImGuiContext* m_context = nullptr;
		Graphics::Device* m_device = nullptr;
		HWND m_hwnd = nullptr;
		DXGI_FORMAT m_rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

		//DX12�p�̃f�B�X�N���v�^�q�[�v
		ComPtr<ID3D12DescriptorHeap> m_srvDescHeap;
		ID3D12CommandQueue* m_commandQueue = nullptr;  // �R�}���h�L���[�ւ̎Q��
		UINT m_frameCount = 2;

		//�������w���p�[
		[[nodiscard]] Utils::VoidResult createDescriptorHeap();
		[[nodiscard]] Utils::VoidResult reinitializeForResize();
	};

	//======================================================================
	//ImGui�E�B���h�E�̊��N���X
	//======================================================================
	class ImGuiWindow
	{
	public:
		ImGuiWindow(const std::string& title, bool visible = true)
			:m_title(title), m_visible(visible) {
		}
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
		SceneHierarchyWindow();

		void draw() override;

		// �V�[���̐ݒ�
		void setScene(Graphics::Scene* scene) { m_scene = scene; }

		// �I���@�\
		Core::GameObject* getSelectedObject() const { return m_selectedObject; }
		void setSelectedObject(Core::GameObject* object) { m_selectedObject = object; }

		// �I��ύX�R�[���o�b�N
		void setSelectionChangedCallback(std::function<void(Core::GameObject*)> callback);

		// �R���e�L�X�g���j���[�R�[���o�b�N
		void setCreateObjectCallback(std::function<Core::GameObject* (UI::PrimitiveType, const std::string&)> callback);
		void setDeleteObjectCallback(std::function<void(Core::GameObject*)> callback);
		void setDuplicateObjectCallback(std::function<Core::GameObject* (Core::GameObject*)> callback);
		void setRenameObjectCallback(std::function<void(Core::GameObject*, const std::string&)> callback);

	private:
		Graphics::Scene* m_scene = nullptr;
		Core::GameObject* m_selectedObject = nullptr;
		std::function<void(Core::GameObject*)> m_onSelectionChanged;

		std::unique_ptr<ContextMenu> m_contextMenu;
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
		void setSelectedObject(Core::GameObject* object) { m_selectedObject = object; }

		// �}�e���A���E�e�N�X�`���Ǘ��ݒ�
		void setMaterialManager(Graphics::MaterialManager* manager) { m_materialManager = manager; }
		void setTextureManager(Graphics::TextureManager* manager) { m_textureManager = manager; }
		Core::GameObject* getSelectedObject() const { return m_selectedObject; }

	private:
		Core::GameObject* m_selectedObject = nullptr;
		Graphics::MaterialManager* m_materialManager = nullptr;
		Graphics::TextureManager* m_textureManager = nullptr;

		void drawTransformComponent(Core::Transform* transform);
		void drawRenderComponent(Graphics::RenderComponent* renderComponent);

		// �}�e���A���ҏW�@�\
		void drawMaterialEditor(Graphics::RenderComponent* renderComponent);
		void drawTextureSlot(const char* name, Graphics::TextureType textureType,
			std::shared_ptr<Graphics::Material> material);
	};
}