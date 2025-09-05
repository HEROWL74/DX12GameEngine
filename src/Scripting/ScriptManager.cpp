﻿#include "ScriptManager.hpp"
#include "../Utils/Common.hpp" // log_info などがある前提

namespace Engine::Scripting
{
    ScriptManager::ScriptManager()
    {
        // 初期化は initialize() で分けておく
    }

    void ScriptManager::initialize()
    {
        m_lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os, sol::lib::string, sol::lib::table);
        Utils::log_info("Lua VM initialized.");
    }

    bool ScriptManager::loadScript(const std::string& path)
    {
        try {
            // ファイルの更新時刻を記録
            auto lastWrite = std::filesystem::last_write_time(path);

            // 実行
            m_lua.script_file(path);

            ScriptData data;
            data.lastWriteTime = lastWrite;

            // 関数のキャッシュ（あれば）
            const std::vector<std::string> knownFunctions = { "onUpdate", "onStart" };
            for (const auto& name : knownFunctions) {
                sol::object obj = m_lua[name];
                if (obj.is<sol::function>()) {
                    data.functions[name] = obj.as<sol::function>();
                    Utils::log_info(std::format("Loaded Lua function: {}", name));
                }
            }

            m_scripts[path] = std::move(data);
            return true;
        }
        catch (const std::exception& e)
        {
            Utils::log_warning(std::format("Lua error in '{}': {}", path, e.what()));
            return false;
        }
    }

    sol::function ScriptManager::getFunction(const std::string& path, const std::string& functionName) const
    {
        auto it = m_scripts.find(path);
        if (it != m_scripts.end())
        {
            auto fit = it->second.functions.find(functionName);
            if (fit != it->second.functions.end())
            {
                return fit->second;
            }
        }
        return sol::nil;
    }

    void ScriptManager::reloadAll()
    {
        for (auto& [path, script] : m_scripts)
        {
            loadScript(path);
        }
        Utils::log_info("All Lua scripts reloaded.");
    }
}

