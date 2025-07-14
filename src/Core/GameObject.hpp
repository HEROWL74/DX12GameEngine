//src/Core/GameObject.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeindex>  //ECS�ł悭�g��
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

namespace Engine::Core
{
	//�O���錾
	class GameObject;
	class Component;
	class Transform;

	//==========================================================
	//Component�x�[�X�N���X
	//==========================================================
	class Component
	{
	public:
		Component() = default;
		virtual ~Component() = default;

		//�R�s�[�E���[�u�֎~
		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(Component&&) = delete;

		//���C�t�T�C�N��
		virtual void start() {}                    //�������̎��Ɉ�x�����Ă�
		virtual void update(float deltaTime) {}    //���t���[���Ă�
		virtual void lateUpdate(float deltaTime) {}//update�Ƃ��ƂɌĂ�
		virtual void onDestroy() {}                //�j�����ɌĂ�

		//�Q�[���I�u�W�F�N�g�̎擾
		GameObject* getGameObject() const { return m_gameObject; }

		//�L����������
		bool isEnabled() const { return m_enabled; }
		void setEnabled(bool enabled) { m_enabled = enabled; }

	protected:
		GameObject* m_gameObject = nullptr;
		bool m_enabled = true;

		friend class GameObject;
	};

	//=========================================================
	//Transform�R���|�[�l���g�i�K�{�R���|�[�l���g�j
	//=========================================================
	class Transform : public Component
	{
		Transform() = default;
		~Transform() = default;

		//�ʒu�E��]�E�X�P�[��
		const Math::Vector3& getPosition() const { return m_position; }
		const Math::Vector3& getRotation() const { return m_rotation; }
		const Math::Vector3& getScale() const { return m_scale; }

		void setPosition(const Math::Vector3& position) { m_position = position; m_isDirty = true; }
		void setRotation(const Math::Vector3& rotation) { m_rotation = rotation; m_isDirty = true; }
		void setScale(const Math::Vector3& scale) { m_scale = scale; m_isDirty = true; }

		//�ړ��E��]
		void translate(const Math::Vector3& transition) { m_position += transition; m_isDirty = true; }
		void rotate(const Math::Vector3& rotation) { m_rotation += rotation; m_isDirty = true; }

		//���[���h�s��̎擾
		const Math::Matrix4& getWorldMatrix();

		//�����x�N�g��
		Math::Vector3 getForward() const;
		Math::Vector3 getRight() const;
		Math::Vector3 getUp() const;
	private:
		Math::Vector3 m_position = Math::Vector3::zero();
		Math::Vector3 m_rotation = Math::Vector3::zero();
		Math::Vector3 m_scale = Math::Vector3::one();

		mutable Math::Matrix4 m_worldMatrix;
		mutable bool m_isDirty = true; //isDirty�Ə������̂́A��Ԃ��ŐV����Ȃ��ꍇ�ɏ�����ǉ����邽��

		void updateWorldMatrix() const;
	};

	//=========================================================
	//�Q�[���I�u�W�F�N�g�N���X
	//=========================================================
	class GameObject
	{
	public:
		explicit GameObject(const std::string& name = "GameObject");

		~GameObject();

		//�R�s�[�E���[�u�֎~
		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(GameObject&&) = delete;

		//��{���
		const std::string& getName() const { return m_name; }
		void setName(const std::string& name) { m_name = name; }

		bool isActive() const { return m_active; }
		void setActive(bool active) { m_active = active; }

		//Transform�i�K�{�̃R���|�[�l���g�j
		Transform* getTransform() const { return m_transform; }

		//�R���|�[�l���g�̒ǉ��E�擾�E�폜
		template<typename T, typename... Args>
		T* addComponent(Args&&... args);

		template<typename T>
		T* getComponent() const;

		template<typename T>
		void removeComponent();

		bool hasComponent(std::type_index type) const;

		template<typename T>
		bool hasComponent() const { return hasComponent(std::type_index(typeid(T))); }

		//���C�t�T�C�N��
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);
		void destroy();

		//�q�I�u�W�F�N�g�Ǘ�
		void addChild(std::unique_ptr<GameObject> child);
		void removeChild(GameObject* child);
		GameObject* findChild(const std::string& name) const;
		const std::vector<std::unique_ptr<GameObject>>& getChildren() const { return m_children; }

		//�e�I�u�W�F�N�g
		GameObject* getParent() const { return m_parent; }

	private:
		std::string m_name;
		bool m_active = true;
		bool m_started = false;

		//�K�{�R���|�[�l���g
		Transform* m_transform = nullptr;

		//�R���|�[�l���g�Ǘ�
		std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;

		//�K�w�\��
		GameObject* m_parent = nullptr;
		std::vector<std::unique_ptr<GameObject>> m_children;
	};

	//=============================================================
	//�e���v���[�g����
	//=============================================================
	template<typename T, typename... Args>
	T* GameObject::addComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));

		//���ɑ��݂���ꍇ�͒ǉ����Ȃ�
		if (hasComponent(typeIndex))
		{
			return static_cast<T*>(m_components[typeIndex].get());
		}

		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		T* componentPtr = component.get();

		component->m_gameObject = this;
		m_components[typeIndex] = std::move(component);

		return componentPtr;

	}

	template<typename T>
	T* GameObject::getComponent() const
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));
		auto it = m_components.find(typeIndex);

		if (it != m_components.end())
		{
			return static_cast<T*>(it->second.get());
		}

		return nullptr;
	}

	template<typename T>
	void GameObject::removeComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));
		auto it = m_components.find(typeIndex);

		if (it != m_components.end())
		{
			it->second->onDestroy();
			m_components.erase(it);
		}
	}
}


