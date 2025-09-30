#pragma once
#include <string>

namespace Engine::Scripting
{
	class IScript
	{
	public:
		virtual ~IScript() = default;

		//���ʃ��C�t�T�C�N��
		virtual void OnStart(){}
		virtual void OnUpdate(float deltaTime){}
		virtual void OnDestroy(){}

		//�f�o�b�O�p�ɃX�N���v�g����Ԃ�
		virtual std::string GetName() const {return  "UnnamedScript";}
	};
}