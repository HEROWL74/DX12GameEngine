//src/Graphics/RenderComponent.hpp
#pragma once

#include "../Core/GameObject.hpp"
#include "Device.hpp"
#include "Camera.hpp"
#include "TriangleRenderer.hpp"
#include "CubeRenderer.hpp"

namespace Engine::Graphics
{
	//レンダリング可能なオブジェクトの種類
	enum class RenderableType
	{
		Triangle,
		Cube,
		//SphereやPlaneも追加予定
	};

	//=========================================
	//レンダリングコンポーネント
	//=========================================
	class RenderComponent : public Core::Component
	{
	public:
		explicit RenderComponent(RenderableType type = RenderableType::Cube);
		~RenderComponent() = default;

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//レンダリング
		void Render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

		//レンダリングタイプ
		RenderableType getRenderableType() const { return m_renderableType; }
		void setRenderableType(RenderableType type);

		//色の設定TODO:（マテリアルに発展する予定）
		void setColor(const Math::Vector3& color) { m_color = color; }
		const Math::Vector3& getColor() const { return m_color; }

		//有効か無効か
		void setVisible(bool visible) { m_visible = visible; }
		bool isVisible() const { return m_visible; }

		//有効性チェック
		[[nidiscard]] bool isValid() const;

	private:
		Device* m_device = nullptr;
		RenderableType m_renderableType;
		Math::Vector3 m_color = Math::Vector3(1.0f, 1.0f, 1.0f);
		bool m_visible = true;
		bool m_initialized = false;

		//レンダラー
		std::unique_ptr<TriangleRenderer> m_triangleRenderer;
		std::unique_ptr<CubeRenderer> m_cubeRenderer;

		//初期化ヘルパー
		[[nodiscard]] Utils::VoidResult initializeRenderer();
	};

	//==================================================================================
	//シーン管理クラス
	//==================================================================================
	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		//コピー・ムーブ禁止
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		//ゲームオブジェクト管理
		Core::GameObject* createGameObject(const std::string& name = "GameObject");
		void destroyGameObject(Core::GameObject* gameObject);
		Core::GameObject* findGameObject(const std::string& name) const;

		//ライフサイクル
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);

		//レンダリング
		void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

		//初期化
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//ゲームオブジェクト一覧取得
		const std::vector<std::unique_ptr<Core::GameObject>>& getGameObjects() const { return m_gameObjects; }

	private:
		Device* m_device = nullptr;
		std::vector<std::unique_ptr<Core::GameObject>> m_gameObjects;
		bool m_initialized = false;
	};
}