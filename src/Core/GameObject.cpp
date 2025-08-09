//src/Core/GameObject.cpp
#include "GameObject.hpp"
#include <algorithm>

namespace Engine::Core
{
	//=========================================
	//Transform����
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
		//TODO: �W���o�����b�N��h�����߂ɃN�H�[�^�j�I���������\��
		//�X�P�[�� -> ��] -> �ړ��̏��ōs�������
		Math::Matrix4 scaleMatrix = Math::Matrix4::scaling(m_scale);
		Math::Matrix4 rotationMatrix = Math::Matrix4::rotationX(Math::radians(m_rotation.x)) *
			Math::Matrix4::rotationY(Math::radians(m_rotation.y)) *
			Math::Matrix4::rotationZ(Math::radians(m_rotation.z));
		Math::Matrix4 translationMatrix = Math::Matrix4::translation(m_position);

		m_worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
	}

	//============================================
	//GameObject����
	//============================================
	GameObject::GameObject(const std::string& name)
		:m_name(name)
	{
		//Transform�́A�K�{�R���|�[�l���g�Ƃ��Ď����ǉ�
		m_transform = addComponent<Transform>();
	}

	GameObject::~GameObject()
	{
		destroy();
	}

	void GameObject::start()
	{
		if (m_started || !m_active) return;

		//�S�ẴR���|�[�l���g���J�n
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->start();
			}
		}

		//�q�I�u�W�F�N�g���J�n
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

		//�܂��J�n����Ă��Ȃ��ꍇ�͊J�n
		if (!m_started)
		{
			start();
		}

		//�S�ẴR���|�[�l���g���X�V
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->update(deltaTime);
			}
		}

		//�q�I�u�W�F�N�g���X�V
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

		//�S�ẴR���|�[�l���g����X�V
		for (auto& [type, component] : m_components)
		{
			if (component->isEnabled())
			{
				component->lateUpdate(deltaTime);
			}
		}

		//�q�I�u�W�F�N�g����X�V
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
		// �A�N�e�B�u�t���O����false��
		m_active = false;

		// �q�I�u�W�F�N�g���ɔj��
		m_children.clear();

		// �R���|�[�l���g��j��
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