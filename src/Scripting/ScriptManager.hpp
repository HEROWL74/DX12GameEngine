#pragma once

#include <sol/sol.hpp>
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>

namespace Engine::Scripting
{
	class ScriptManager
	{
	public:
		ScriptManager();
		~ScriptManager() = default;

		//Luaライブラリの初期化
		void initialize();

		//スクリプトを読み込み&キャッシュ
		bool loadScript(const std::string& path);

		//script_path -> onUpdate関数
		sol::function getFunction(const std::string& path, const std::string& functionName) const;

		sol::state& getLuaState() { return m_lua; }

		//全スクリプトを再読み込み
		void reloadAll();

	private:
		sol::state m_lua;

		//スクリプトごとの関数キャッシュ
		struct ScriptData
		{
			std::filesystem::file_time_type lastWriteTime;
			std::unordered_map<std::string, sol::function> functions;
		};

		std::unordered_map<std::string, ScriptData> m_scripts;
	};
}
