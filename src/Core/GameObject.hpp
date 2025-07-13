//src/Core/GameObject.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeindex>
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
}


