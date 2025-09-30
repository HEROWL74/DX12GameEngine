#pragma once
#include "IScript.hpp"
#include "../Core/GameObject.hpp"

namespace Engine::Scripting
{
	class CppScriptComponent : public Core::Component, public IScript
	{
	public:
		explicit CppScriptComponent(Core::GameObject* owner)
			: m_owner(owner) { }

		virtual ~CppScriptComponent() = default;

		//Component �̃��C�t�T�C�N���Ɠ���
		void start()override { OnStart(); }
		void update(float dt)override { OnUpdate(dt); }
		void lateUpdate(float dt) override {}
		void onDestroy()override { OnDestroy(); }

		//IScript�̌p�����\�b�h
		virtual void OnStart() override {};
		virtual void OnUpdate(float dt) override{}
		virtual void OnDestroy() override {}

	private:
		Core::GameObject* m_owner = nullptr;
	}; 
}