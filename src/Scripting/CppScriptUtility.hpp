#pragma once
#include <string>

namespace Engine::Scripting
{
	class CppScriptUtility
	{
	public:
		//�t�@�C�����쐬����i�f�t�H���g���e�܂ށj
		static bool createNewScript(const std::string& path);

		//VSCode�J��
		static void openInVSCode(const std::string& path);

		//�t�@�C����cpp�␳
		static std::string normalizePath(const std::string& name);
	};
}