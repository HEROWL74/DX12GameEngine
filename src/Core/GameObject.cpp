//src/Core/GameObject.cpp
#include "GameObject.hpp"
#include <algorithm>

namespace Engine::Core
{
	//=========================================
	//Transform螳溯｣・
	//=========================================
	const Math::Matrix4& Transform::getWorldMatrix()
	{
		if (m_isDirty)
		{
			updateWorldMatrix();
			m_isDirty = false;
		}
		return m_worldMatrix;
	}

	Math::Vector3 Transform::getForward() const
	{
		float pitchRad = Math::radians(m_rotation.x);
		float yawRad = Math::radians(m_rotation.y);

		return Math::Vector3(
			std::sin(yawRad) * std::cos(pitchRad),
			-std::sin(pitchRad),
			std::cos(yawRad) * std::cos(pitchRad)
		).normalized();
	}

	Math::Vector3 Transform::getRight() const
	{
		Math::Vector3 forward = getForward();
		return Math::Vector3::cross(forward, Math::Vector3::up()).normalized();
	}

	Math::Vector3 Transform::getUp() const
	{
		Math::Vector3 forward = getForward();
		Math::Vector3 right = getRight();
		return Math::Vector3::cross(right, forward);
	}

	void Transform::updateWorldMatrix() const
	{
		//TODO: 繧ｸ繝ｳ繝舌Ν繝ｭ繝・け繧帝亟縺舌◆繧√↓繧ｯ繧ｩ繝ｼ繧ｿ繝九が繝ｳ繧ょｮ溯｣・ｺ亥ｮ・
		//繧ｹ繧ｱ繝ｼ繝ｫ -> 蝗櫁ｻ｢ -> 遘ｻ蜍輔・鬆・〒陦悟・繧貞粋菴・
		Math::Matrix4 scaleMatrix = Math::Matrix4::scaling(m_scale);
		Math::Matrix4 rotationMatrix = Math::Matrix4::rotationX(Math::radians(m_rotation.x)) *
			Math::Matrix4::rotationY(Math::radians(m_rotation.y)) *
			Math::Matrix4::rotationZ(Math::radians(m_rotation.z));
		Math::Matrix4 translationMatrix = Math::Matrix4::translation(m_position);

		m_worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
	}

	//============================================
	//GameObject螳溯｣・
	//============================================
	GameObject::GameObject(const std::string& name)
		:m_name(name)
	{
		//Transform縺ｯ縲∝ｿ・医さ繝ｳ繝昴・繝阪Φ繝医→縺励※閾ｪ蜍戊ｿｽ蜉
		m_transform = addComponent<Transform>();
	}

	GameObject::~GameObject()
	{
		destroy();
	}

	void GameObject::start()
	{
		if (m_started || !m_active) return;

		//蜈ｨ縺ｦ縺ｮ繧ｳ繝ｳ繝昴・繝阪Φ繝医ｒ髢句ｧ・
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->start();
			}
		}

		//蟄舌が繝悶ず繧ｧ繧ｯ繝医ｂ髢句ｧ・
		for (auto& child : m_children)
		{
			if (child->isActive())
			{
				child->start();
			}
		}

		m_started = true;
	}

	void GameObject::update(float deltaTime)
	{
		if (!m_active) return;

		//縺ｾ縺髢句ｧ九＆繧後※縺・↑縺・ｴ蜷医・髢句ｧ・
		if (!m_started)
		{
			start();
		}

		//蜈ｨ縺ｦ縺ｮ繧ｳ繝ｳ繝昴・繝阪Φ繝医ｒ譖ｴ譁ｰ
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->update(deltaTime);
			}
		}

		//蟄舌が繝悶ず繧ｧ繧ｯ繝医ｂ譖ｴ譁ｰ
		for (auto& child : m_children)
		{
			if (child->isActive())
			{
				child->update(deltaTime);
			}
		}
	}

	void GameObject::lateUpdate(float deltaTime)
	{
		if (!m_active) return;

		//蜈ｨ縺ｦ縺ｮ繧ｳ繝ｳ繝昴・繝阪Φ繝医ｒ蠕梧峩譁ｰ
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->lateUpdate(deltaTime);
			}
		}

		//蟄舌が繝悶ず繧ｧ繧ｯ繝医ｂ蠕梧峩譁ｰ
		for (auto& child : m_children)
		{
			if (child->isActive())
			{
				child->lateUpdate(deltaTime);
			}
		}
	}

	void GameObject::destroy()
	{
		// 繧｢繧ｯ繝・ぅ繝悶ヵ繝ｩ繧ｰ繧貞・縺ｫfalse縺ｫ
		m_active = false;

		// 蟄舌が繝悶ず繧ｧ繧ｯ繝医ｒ蜈医↓遐ｴ譽・
		m_children.clear();

		// 繧ｳ繝ｳ繝昴・繝阪Φ繝医ｒ遐ｴ譽・
		for (auto& [type, component] : m_components)
		{
			if (component)
			{
				component->onDestroy();
			}
		}
		m_components.clear();

		m_transform = nullptr;
		m_parent = nullptr;
	}

	bool GameObject::hasComponent(std::type_index type) const
	{
		return m_components.find(type) != m_components.end();
	}

	void GameObject::addChild(std::unique_ptr<GameObject> child)
	{
		if (child)
		{
			child->m_parent = this;
			m_children.push_back(std::move(child));
		}
	}

	void GameObject::removeChild(GameObject* child)
	{
		auto it = std::find_if(m_children.begin(), m_children.end(),
			[child](const std::unique_ptr<GameObject>& ptr)
			{
				return ptr.get() == child;
			});

		if (it != m_children.end())
		{
			(*it)->m_parent = nullptr;
			m_children.erase(it);
		}
	}

	GameObject* GameObject::findChild(const std::string& name) const
	{
		for (const auto& child : m_children)
		{
			if (child->getName() == name)
			{
				return child.get();
			}
		}

		return nullptr;
	}
} 
