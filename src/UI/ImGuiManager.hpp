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
	//ImGuiクラス
	//======================================================================
	class ImGuiManager
	{
	public:
		ImGuiManager() = default;
		~ImGuiManager();

		//コピー・ムーブ禁止
		ImGuiManager(const ImGuiManager&) = delete;
		ImGuiManager& operator=(const ImGuiManager&) = delete;
		ImGuiManager(ImGuiManager&&) = delete;
		ImGuiManager& operator=(ImGuiManager&&) = delete;

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(
			Graphics::Device* device,
			HWND hwnd,
			DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
			UINT frameCount = 2
		);

		//終了処理
		void shutdown();

		//フレーム開始
		void newFrame();

		//描画
		void render(ID3D12GraphicsCommandList* commandList);

		//Win32メッセージ処理
		void handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

		//有効性のチェック
		[[nodiscard]] bool isInitialized() const { return m_initialized; }

		//ImGuiコンテキスト取得
		ImGuiContext* getContext() const { return m_context; }

	private:
		bool m_initialized = false;
		ImGuiContext* m_context = nullptr;
		Graphics::Device* m_device = nullptr;

		//DX12用のディスクリプタヒープ
		ComPtr<ID3D12DescriptorHeap> m_srvDescHeap;
		UINT m_frameCount = 2;

		//初期化ヘルパー
		[[nodiscard]] Utils::VoidResult createDescriptorHeap();
	}; 

	//======================================================================
	//ImGuiウィンドウの基底クラス
	//======================================================================
	class ImGuiWindow
	{
	public:
		ImGuiWindow(const std::string& title, bool visible = true)
			:m_title(title), m_visible(visible) { }
		virtual ~ImGuiWindow() = default;

		//描画
		virtual void draw() = 0;

		//表示/非表示
		void setVisible(bool visible) { m_visible = visible; }
		bool isVisible() const { return m_visible; }

		//タイトル
		const std::string& getTitle() const { return m_title; }
		void setTitle(const std::string& title) { m_title = title; }

	protected:
		std::string m_title;
		bool m_visible;
	};

	//=======================================================================
	//デバッグ情報ウィンドウ
	//=======================================================================
	class DebugWindow : public ImGuiWindow
	{
	public:
		DebugWindow() : ImGuiWindow("Debug info") {}

		void draw() override;

		//デバッグ情報の設定
		void setFPS(float fps) { m_fps = fps; }
		void setFrameTime(float frameTime) { m_frameTime = frameTime; }
		void setObjectCount(size_t count) { m_objectCount = count; }

	private:
		float m_fps = 0.0f;
		float m_frameTime = 0.0f;
		size_t m_objectCount = 0;
	};

	//======================================================================
	//オブジェクト階層ウィンドウ
	//======================================================================
	class SceneHierarchyWindow : public ImGuiWindow
	{
	public:
		SceneHierarchyWindow() : ImGuiWindow("Scene Hierarchy") {}

		void draw() override; 

		//シーンの設定
		void setScene(Graphics::Scene* scene) { m_scene = scene; }

	private:
		Graphics::Scene* m_scene = nullptr;
		Core::GameObject* m_selectedObject = nullptr;

		void drawGameObject(Core::GameObject* gameObject);
	};

	//=======================================================================
	//オブジェクトインスペクター
	//=======================================================================
	class InspectorWindow : public ImGuiWindow
	{
	public:
		InspectorWindow() : ImGuiWindow("Inspector") {}

		void draw() override;

		//選択オブジェクトの設定
		void setSelectedObject(Core::GameObject* object) {}

	private:
		Core::GameObject* m_selectedObject = nullptr;

		void drawTransformComponent(Core::Transform* transform);
		void drawRenderComponent(Graphics::RenderComponent* renderComponent);
		
	};
}
