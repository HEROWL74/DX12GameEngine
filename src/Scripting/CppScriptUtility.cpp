#include "CppScriptUtility.hpp"
#include <fstream>
#include <Windows.h>

namespace Engine::Scripting
{
	bool CppScriptUtility::createNewScript(const std::string& path,const std::string& className)
	{
		std::ofstream file(path);
		if (!file.is_open()) return false;

		file << "#include \"Scripts.hpp\"\n\n";
		file << "class " << className << " : public Script::Cpp {\n";
		file << "public:\n";
		file << "    using Script::Cpp::Cpp;\n\n";
		file << "    void OnStart() override {\n";
		file << "        // ‰Šú‰»ˆ—\n";
		file << "    }\n\n";
		file << "    void OnUpdate(float dt) override {\n";
		file << "        // –ˆƒtƒŒ[ƒ€ˆ—\n";
		file << "    }\n";
		file << "};\n";

		return true;
	}

	void CppScriptUtility::openInVSCode(const std::string& path)
	{
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