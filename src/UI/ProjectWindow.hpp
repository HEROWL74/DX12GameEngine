﻿//src/UI/ProjectWindow.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include "ImGuiManager.hpp"
#include "../Graphics/Texture.hpp"
#include "../Graphics/Material.hpp"
#include "../Scripting/LuaScriptUtility.hpp"

namespace Engine::UI
{
    //=========================================================================
    // アセット情報構造体
    //=========================================================================
    struct AssetInfo
    {
        std::filesystem::path path;
        std::string name;
        std::string extension;
        enum class Type
        {
            Folder,
            Texture,
            Material,
            Shader,
            Script,
            Unknown
        } type{};

        //リネーム情報格納
        bool renaming = false;
        char renameBuffer[256]{};

        std::shared_ptr<Graphics::Texture> texture; // テクスチャプレビュー用
        std::shared_ptr<Graphics::Material> material; // マテリアル用
    };

    struct AssetPayload
    {
        char path[256];
        int type;     // AssetInfo::Type を int にキャストして保存
    };

    //=========================================================================
    // プロジェクトウィンドウ
    //=========================================================================
    class ProjectWindow : public ImGuiWindow
    {
    public:
        ProjectWindow();
        ~ProjectWindow() = default;

        void draw() override;

        // 依存関係設定
        void setTextureManager(Graphics::TextureManager* textureManager) { m_textureManager = textureManager; }
        void setMaterialManager(Graphics::MaterialManager* materialManager) { m_materialManager = materialManager; }

        // プロジェクトパス設定
        void setProjectPath(const std::string& path);
        const std::string& getProjectPath() const { return m_projectPath; }

        std::string generateUniqueScriptPath();

        // 選択されたアセット取得
        const AssetInfo* getSelectedAsset() const { return m_selectedAsset; }

        // アセット操作コールバック
        void setAssetDropCallback(std::function<void(const AssetInfo&)> callback) { m_assetDropCallback = callback; }

    private:
        Graphics::TextureManager* m_textureManager = nullptr;
        Graphics::MaterialManager* m_materialManager = nullptr;

        std::string m_projectPath = "assets";
        std::vector<AssetInfo> m_assets;
        AssetInfo* m_selectedAsset = nullptr;

        // UI状態
        float m_iconSize = 64.0f;
        bool m_showGrid = true;
        std::string m_searchFilter;

        // コールバック
        std::function<void(const AssetInfo&)> m_assetDropCallback;

        // プライベートメソッド
        void refreshAssets();
        void drawToolbar();
        void drawAssetGrid();
        void drawAssetList();
        void drawAssetPreview();
        void drawContextMenu();

        AssetInfo::Type getAssetType(const std::filesystem::path& path);
        void loadAssetPreview(AssetInfo& asset);
        bool matchesFilter(const AssetInfo& asset) const;

        // ドラッグ&ドロップ
        void handleDragDrop(const AssetInfo& asset);
    };
}
