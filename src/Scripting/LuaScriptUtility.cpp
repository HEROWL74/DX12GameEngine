//src/Scripting/LuaScriptUtility.cpp
#include "LuaScriptUtility.hpp"
#include <fstream>
#include <Windows.h>

namespace Engine::Scripting
{
	bool LuaScriptUtility::createNewScript(const std::string& path)
	{
		std::ofstream file(path);
		if (!file.is_open()) return false;

		file << "-- " << path << "\n";
		file << "function onStart(obj)\n";
		file << "    print(\"Hello from " << path << "\")\n";
		file << "end\n\n";
		file << "function onUpdate(obj, dt)\n";
		file << "    -- update logic here\n";
		file << "end\n";
		file.close();
		return true;
	}

	void LuaScriptUtility::openInVSCode(const std::string& path)
	{
		//"code"コマンドを使用
		ShellExecuteA(
			nullptr,
			"open",
			"code",
			path.c_str(),
			nullptr,
			SW_HIDE
		);
	}
}