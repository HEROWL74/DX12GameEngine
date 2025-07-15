//src/Graphics/RenderComponent.hpp
#pragma once

#include "../Core/GameObject.hpp"
#include "Device.hpp"
#include "Camera.hpp"
#include "TriangleRenderer.hpp"
#include "CubeRenderer.hpp"

namespace Engine::Graphics
{
	//�����_�����O�\�ȃI�u�W�F�N�g�̎��
	enum class RenderableType
	{
		Triangle,
		Cube,
		//Sphere��Plane���ǉ��\��
	};

	//=========================================
	//�����_�����O�R���|�[�l���g
	//=========================================
	class RenderComponent : public Core::Component
	{
	public:
		explicit RenderComponent(RenderableType type = RenderableType::Cube);
		~RenderComponent() = default;

		//������
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//�����_�����O
		void Render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

		//�����_�����O�^�C�v
		RenderableType getRenderableType() const { return m_renderableType; }
		void setRenderableType(RenderableType type);

		//�F�̐ݒ�TODO:�i�}�e���A���ɔ��W����\��j
		void setColor(const Math::Vector3& color) { m_color = color; }
		const Math::Vector3& getColor() const { return m_color; }

		//�L����������
		void setVisible(bool visible) { m_visible = visible; }
		bool isVisible() const { return m_visible; }

		//�L�����`�F�b�N
		[[nidiscard]] bool isValid() const;

	private:
		Device* m_device = nullptr;
		RenderableType m_renderableType;
		Math::Vector3 m_color = Math::Vector3(1.0f, 1.0f, 1.0f);
		bool m_visible = true;
		bool m_initialized = false;

		//�����_���[
		std::unique_ptr<TriangleRenderer> m_triangleRenderer;
		std::unique_ptr<CubeRenderer> m_cubeRenderer;

		//�������w���p�[
		[[nodiscard]] Utils::VoidResult initializeRenderer();
	};

	//==================================================================================
	//�V�[���Ǘ��N���X
	//==================================================================================
	class Scene
	{
	public:
		Scene() = default;
		~Scene() = default;

		//�R�s�[�E���[�u�֎~
		Scene(const Scene&) = delete;
		Scene& operator=(const Scene&) = delete;
		Scene(Scene&&) = delete;
		Scene& operator=(Scene&&) = delete;

		//�Q�[���I�u�W�F�N�g�Ǘ�
		Core::GameObject* createGameObject(const std::string& name = "GameObject");
		void destroyGameObject(Core::GameObject* gameObject);
		Core::GameObject* findGameObject(const std::string& name) const;

		//���C�t�T�C�N��
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);

		//�����_�����O
		void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

		//������
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//�Q�[���I�u�W�F�N�g�ꗗ�擾
		const std::vector<std::unique_ptr<Core::GameObject>>& getGameObjects() const { return m_gameObjects; }

	private:
		Device* m_device = nullptr;
		std::vector<std::unique_ptr<Core::GameObject>> m_gameObjects;
		bool m_initialized = false;
	};
}