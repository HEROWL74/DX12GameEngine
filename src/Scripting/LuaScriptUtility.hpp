//src/Scripting/LuaScriptUtility
#pragma once
#include <string>

namespace Engine::Scripting
{
	class LuaScriptUtility
	{
	public:
		//�V����Lua�t�@�C�����쐬�i�f�t�H���g���e���܂ށj
		static bool createNewScript(const std::string& path);

		//VSCode�ŊJ��
		static void openInVSCode(const std::string& path);

		//�t�@�C����lua�␳
		static std::string normalizePath(const std::string& name);
	};
}
