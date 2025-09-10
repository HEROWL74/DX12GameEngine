// src/Graphics/RenderComponent.cpp
#include "RenderComponent.hpp"
#include <algorithm>

namespace Engine::Graphics
{
	//==========================================================================================
	//RenderComponent螳溯｣・
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

		// ShaderManager縺ｮ譛牙柑諤ｧ繧貞・遒ｺ隱・
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

		// Transform縺九ｉ菴咲ｽｮ繝ｻ蝗櫁ｻ｢繝ｻ繧ｹ繧ｱ繝ｼ繝ｫ繧貞叙蠕・
		auto* transform = getGameObject()->getTransform();
		if (!transform)
		{
			return;
		}

		// 繝・ヵ繧ｩ繝ｫ繝医・繝・Μ繧｢繝ｫ縺後↑縺・ｴ蜷医・險ｭ螳・
		if (!m_material && m_materialManager)
		{
			m_material = m_materialManager->getDefaultMaterial();
		}
		/*
		// 繝槭ユ繝ｪ繧｢繝ｫ縺ｮ濶ｲ繧坦enderComponent縺ｮ濶ｲ縺ｨ邨・∩蜷医ｏ縺帙ｋ
		if (m_material)
		{
			auto props = m_material->getProperties();

			// RenderComponent縺ｮm_color繧偵・繝・Μ繧｢繝ｫ縺ｮalbedo縺ｫ蜿肴丐
			props.albedo = m_color;
			m_material->setProperties(props);

			// 繝槭ユ繝ｪ繧｢繝ｫ縺ｮ螳壽焚繝舌ャ繝輔ぃ繧呈峩譁ｰ
			auto updateResult = m_material->updateConstantBuffer();
			if (!updateResult)
			{
				Utils::log_warning(std::format("Failed to update material constant buffer: {}",
					updateResult.error().message));
			}
		}
		*/

		// 繝ｬ繝ｳ繝繝ｩ繝ｼ縺ｮTransform繧呈峩譁ｰ
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
		// 譌｢蟄倥・繝ｬ繝ｳ繝繝ｩ繝ｼ繧偵け繝ｪ繧｢
		m_triangleRenderer.reset();
		m_cubeRenderer.reset();

		// ShaderManager縺ｮ譛牙柑諤ｧ繧堤｢ｺ隱・
		if (!m_shaderManager) {
			Utils::log_warning("ShaderManager is null in RenderComponent::initializeRenderer");
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null in RenderComponent"));
		}

		// 繝ｬ繝ｳ繝繝ｩ繝ｼ繧ｿ繧､繝励↓蠢懊§縺ｦ菴懈・
		switch (m_renderableType)
		{
		case RenderableType::Triangle:
		{
			m_triangleRenderer.reset(new TriangleRenderer());
			auto result = m_triangleRenderer->initialize(m_device, m_shaderManager);
			if (!result) {
				m_triangleRenderer.reset();
				return result;
			}
			if (m_materialManager) {
				m_triangleRenderer->setMaterialManager(m_materialManager);
			}
		}
		break;

		case RenderableType::Cube:
		{
			m_cubeRenderer.reset(new CubeRenderer());
			auto result = m_cubeRenderer->initialize(m_device, m_shaderManager);
			if (!result) {
				m_cubeRenderer.reset();
				return result;
			}
			if (m_materialManager) {
				m_cubeRenderer->setMaterialManager(m_materialManager);
			}
		}
		break;

		default:
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unknown renderable type"));
		}

		return {};
	}

	//==========================================================================================
	//Scene螳溯｣・
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
		if (!gameObject)
		{
			Utils::log_warning("Attempted to destroy null GameObject");
			return;
		}

		// 繧ｪ繝悶ず繧ｧ繧ｯ繝亥錐繧剃ｿ晏ｭ・
		std::string objectName = gameObject->getName();

		// 繧､繝・Ξ繝ｼ繧ｿ縺ｧ讀懃ｴ｢
		auto it = m_gameObjects.begin();
		while (it != m_gameObjects.end())
		{
			if (it->get() == gameObject)
			{
				// 繧ｪ繝悶ず繧ｧ繧ｯ繝医ｒ髱槭い繧ｯ繝・ぅ繝門喧
				(*it)->setActive(false);

				// 遐ｴ譽・・逅・ｒ螳溯｡・
				(*it)->destroy();

				// vector縺九ｉ蜑企勁
				it = m_gameObjects.erase(it);

				Utils::log_info(std::format("GameObject '{}' destroyed successfully", objectName));
				return;
			}
			else
			{
				++it;
			}
		}

		Utils::log_warning(std::format("GameObject '{}' not found in scene", objectName));
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

	Core::GameObject* Scene::findObjectByName(const std::string& name)
	{
		for (auto& obj : m_gameObjects)
		{
			if (obj && obj->getName() == name)
				return obj.get();
		}
		return nullptr;
	}
}
