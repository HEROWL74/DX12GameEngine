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

	Utils::VoidResult RenderComponent::initialize(Device* device)
	{
		if (m_initialized)
		{
			return {};
		}

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

		m_device = device;

		auto result = initializeRenderer();
		if (!result)
		{
			return result;
		}

		m_initialized = true;
		return {};
	}

	void RenderComponent::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
	{
		if (!m_visible || !m_initialized || !getGameObject())
		{
			return;
		}

		//Transform����ʒu�E��]�E�X�P�[�����擾
		auto* transform = getGameObject()->getTransform();
		if (!transform)
		{
			return;
		}

		//�����_���[��Transform���X�V
		switch (m_renderableType)
		{
		case RenderableType::Triangle:
			if (m_triangleRenderer && m_triangleRenderer->isValid())
			{
				m_triangleRenderer->setPosition(transform->getPosition());
				m_triangleRenderer->setRotation(transform->getRotation());
				m_triangleRenderer->setScale(transform->getScale());
				m_triangleRenderer->render(commandList, camera, frameIndex);
			}
			break;

		case RenderableType::Cube:
			if (m_cubeRenderer && m_cubeRenderer->isValid()) {
				m_cubeRenderer->setPosition(transform->getPosition());
				m_cubeRenderer->setRotation(transform->getRotation());
				m_cubeRenderer->setScale(transform->getScale());
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
		//�����̃����_���[���N���A
		m_triangleRenderer.reset();
		m_cubeRenderer.reset();

		//�V���������_���[���쐬�E������
		switch (m_renderableType)
		{
		case RenderableType::Triangle:
			m_triangleRenderer = std::make_unique<TriangleRenderer>();
			return m_triangleRenderer->initialize(m_device);
		case RenderableType::Cube:
			m_cubeRenderer = std::make_unique<CubeRenderer>();
			return m_cubeRenderer->initialize(m_device);

		default:
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unknown renderable type"));
		}
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