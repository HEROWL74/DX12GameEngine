#pragma once
#include <string>

namespace Engine::Scripting
{
	class CppScriptUtility
	{
	public:
		static bool createNewScript(const std::string& path, const std::string& className);
		static void openInVSCode(const std::string& path);
	};
}