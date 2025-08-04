//src/UI/ProjectWindow.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <filesystem>
#include <unordered_map>
#include "ImGuiManager.hpp"
#include "../Graphics/Texture.hpp"
#include "../Graphics/Material.hpp"

namespace Engine::UI
{
    //=========================================================================
    // �A�Z�b�g���\����
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
            Unknown
        } type;

        std::shared_ptr<Graphics::Texture> texture; // �e�N�X�`���v���r���[�p
        std::shared_ptr<Graphics::Material> material; // �}�e���A���p
    };

    //=========================================================================
    // �v���W�F�N�g�E�B���h�E
    //=========================================================================
    class ProjectWindow : public ImGuiWindow
    {
    public:
        ProjectWindow();
        ~ProjectWindow() = default;

        void draw() override;

        // �ˑ��֌W�ݒ�
        void setTextureManager(Graphics::TextureManager* textureManager) { m_textureManager = textureManager; }
        void setMaterialManager(Graphics::MaterialManager* materialManager) { m_materialManager = materialManager; }

        // �v���W�F�N�g�p�X�ݒ�
        void setProjectPath(const std::string& path);
        const std::string& getProjectPath() const { return m_projectPath; }

        // �I�����ꂽ�A�Z�b�g�擾
        const AssetInfo* getSelectedAsset() const { return m_selectedAsset; }

        // �A�Z�b�g����R�[���o�b�N
        void setAssetDropCallback(std::function<void(const AssetInfo&)> callback) { m_assetDropCallback = callback; }

    private:
        Graphics::TextureManager* m_textureManager = nullptr;
        Graphics::MaterialManager* m_materialManager = nullptr;

        std::string m_projectPath = "assets";
        std::vector<AssetInfo> m_assets;
        AssetInfo* m_selectedAsset = nullptr;

        // UI���
        float m_iconSize = 64.0f;
        bool m_showGrid = true;
        std::string m_searchFilter;

        // �R�[���o�b�N
        std::function<void(const AssetInfo&)> m_assetDropCallback;

        // �v���C�x�[�g���\�b�h
        void refreshAssets();
        void drawToolbar();
        void drawAssetGrid();
        void drawAssetList();
        void drawAssetPreview();
        void drawContextMenu();

        AssetInfo::Type getAssetType(const std::filesystem::path& path);
        void loadAssetPreview(AssetInfo& asset);
        bool matchesFilter(const AssetInfo& asset) const;

        // �h���b�O&�h���b�v
        void handleDragDrop(const AssetInfo& asset);
    };
}