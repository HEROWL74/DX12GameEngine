// src/Graphics/RenderComponent.cpp
#include "RenderComponent.hpp"
#include <algorithm>

namespace Engine::Graphics
{
	//==========================================================================================
	//RenderComponent����
	//==========================================================================================
	RenderComponent::RenderComponent(RenderableType type)
		:m_renderableType(type)
	{
	}


	Utils::VoidResult RenderComponent::initialize(Device* device, ShaderManager* shaderManager)
	{
		if (m_initialized)
		{
			return {};
		}

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(shaderManager != nullptr, Utils::ErrorType::Unknown, "ShaderManager is null in RenderComponent::initialize");

		m_device = device;
		m_shaderManager = shaderManager;

		Utils::log_info("RenderComponent::initialize - Device and ShaderManager assigned successfully");

		// ShaderManager�̗L�������Ċm�F
		if (!m_shaderManager) {
			Utils::log_warning("ShaderManager became null after assignment");
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null after assignment"));
		}

		auto result = initializeRenderer();
		if (!result)
		{
			Utils::log_error(result.error());
			return result;
		}

		m_initialized = true;
		Utils::log_info("RenderComponent initialized successfully");
		return {};
	}

	void RenderComponent::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
	{
		if (!m_visible || !m_initialized || !getGameObject())
		{
			return;
		}

		// Transform����ʒu�E��]�E�X�P�[�����擾
		auto* transform = getGameObject()->getTransform();
		if (!transform)
		{
			return;
		}

		// �f�t�H���g�}�e���A�����Ȃ��ꍇ�͐ݒ�
		if (!m_material && m_materialManager)
		{
			m_material = m_materialManager->getDefaultMaterial();
		}
		/*
		// �}�e���A���̐F��RenderComponent�̐F�Ƒg�ݍ��킹��
		if (m_material)
		{
			auto props = m_material->getProperties();

			// RenderComponent��m_color���}�e���A����albedo�ɔ��f
			props.albedo = m_color;
			m_material->setProperties(props);

			// �}�e���A���̒萔�o�b�t�@���X�V
			auto updateResult = m_material->updateConstantBuffer();
			if (!updateResult)
			{
				Utils::log_warning(std::format("Failed to update material constant buffer: {}",
					updateResult.error().message));
			}
		}
		*/

		// �����_���[��Transform���X�V
		switch (m_renderableType)
		{
		case RenderableType::Triangle:
			if (m_triangleRenderer && m_triangleRenderer->isValid())
			{
				m_triangleRenderer->setPosition(transform->getPosition());
				m_triangleRenderer->setRotation(transform->getRotation());
				m_triangleRenderer->setScale(transform->getScale());
				m_triangleRenderer->setMaterial(m_material);
				m_triangleRenderer->render(commandList, camera, frameIndex);
			}
			break;

		case RenderableType::Cube:
			if (m_cubeRenderer && m_cubeRenderer->isValid())
			{
				m_cubeRenderer->setPosition(transform->getPosition());
				m_cubeRenderer->setRotation(transform->getRotation());
				m_cubeRenderer->setScale(transform->getScale());
				m_cubeRenderer->setMaterial(m_material);
				m_cubeRenderer->render(commandList, camera, frameIndex);
			}
			break;
		}
	}

	void RenderComponent::setRenderableType(RenderableType type)
	{
		if (m_renderableType != type)
		{
			m_renderableType = type;
			if (m_initialized)
			{
				initializeRenderer();
			}
		}
	}

	bool RenderComponent::isValid() const
	{
		if (!m_initialized || !m_device)
		{
			return false;
		}

		switch (m_renderableType)
		{
		case RenderableType::Triangle:
			return m_triangleRenderer && m_triangleRenderer->isValid();
		case RenderableType::Cube:
			return m_cubeRenderer && m_cubeRenderer->isValid();
		default:
			return false;
		}
	}

	Utils::VoidResult RenderComponent::initializeRenderer()
	{
		// �����̃����_���[���N���A
		m_triangleRenderer.reset();
		m_cubeRenderer.reset();

		// ShaderManager�̗L�������m�F
		if (!m_shaderManager) {
			Utils::log_warning("ShaderManager is null in RenderComponent::initializeRenderer");
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null in RenderComponent"));
		}

		std::expected<void, Utils::Error> result;

		// �V���������_���[���쐬�E������
		switch (m_renderableType)
		{
		case RenderableType::Triangle:
			m_triangleRenderer = std::make_unique<TriangleRenderer>();
			if (!m_triangleRenderer) {
				result = std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to create TriangleRenderer"));
				break;
			}
			Utils::log_info("Created TriangleRenderer, calling initialize with valid ShaderManager");
			result = m_triangleRenderer->initialize(m_device, m_shaderManager);
			if (result && m_materialManager) {
				m_triangleRenderer->setMaterialManager(m_materialManager);
			}
			break;

		case RenderableType::Cube:
			m_cubeRenderer = std::make_unique<CubeRenderer>();
			if (!m_cubeRenderer) {
				result = std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to create CubeRenderer"));
				break;
			}
			Utils::log_info("Created CubeRenderer, calling initialize with valid ShaderManager");
			result = m_cubeRenderer->initialize(m_device, m_shaderManager);
			if (result && m_materialManager) {
				m_cubeRenderer->setMaterialManager(m_materialManager);
			}
			break;

		default:
			result = std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unknown renderable type"));
			break;
		}

		return result;
	}

	//==========================================================================================
	//Scene����
	//==========================================================================================
	Utils::VoidResult Scene::initialize(Device* device)
	{
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

		m_device = device;
		m_initialized = true;
		return {};
	}

	Core::GameObject* Scene::createGameObject(const std::string& name)
	{
		auto gameObject = std::make_unique<Core::GameObject>(name);
		Core::GameObject* ptr = gameObject.get();
		m_gameObjects.push_back(std::move(gameObject));
		return ptr;
	}

	void Scene::destroyGameObject(Core::GameObject* gameObject)
	{
		auto it = std::find_if(m_gameObjects.begin(), m_gameObjects.end(),
			[gameObject](const std::unique_ptr<Core::GameObject>& ptr) {
				return ptr.get() == gameObject;
			});

		if (it != m_gameObjects.end())
		{
			(*it)->destroy();
			m_gameObjects.erase(it);
		}
	}

	Core::GameObject* Scene::findGameObject(const std::string& name) const
	{
		for (const auto& gameObject : m_gameObjects)
		{
			if (gameObject->getName() == name)
			{
				return gameObject.get();
			}
		}
		return nullptr;
	}

	void Scene::start()
	{
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive())
			{
				gameObject->start();
			}
		}
	}

	void Scene::update(float deltaTime)
	{
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive())
			{
				gameObject->update(deltaTime);
			}
		}
	}

	void Scene::lateUpdate(float deltaTime)
	{
		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive())
			{
				gameObject->lateUpdate(deltaTime);
			}
		}
	}

	void Scene::render(ID3D12GraphicsCommandList* commandList,const Camera& camera, UINT frameIndex)
	{
		if (!m_initialized)
		{
			return;
		}

		for (auto& gameObject : m_gameObjects)
		{
			if (gameObject->isActive())
			{
				auto* renderComponent = gameObject->getComponent<RenderComponent>();
				if (renderComponent && renderComponent->isEnabled() && renderComponent->isVisible())
				{
					renderComponent->render(commandList, camera, frameIndex);
				}
			}
		}
	}

}