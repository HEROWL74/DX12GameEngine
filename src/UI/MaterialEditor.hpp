//src/UI/MaterialEditor.hpp
#pragma once

#include <memory>
#include <vector>
#include <functional>
#include "ImGuiManager.hpp"
#include "../Graphics/Material.hpp"

namespace Engine::UI
{
    //=========================================================================
    // マテリアルエディターウィンドウ
    //=========================================================================
    class MaterialEditorWindow : public ImGuiWindow
    {
    public:
        MaterialEditorWindow();
        ~MaterialEditorWindow() = default;

        void draw() override;

        // マテリアル設定
        void setMaterial(std::shared_ptr<Graphics::Material> material);
        std::shared_ptr<Graphics::Material> getCurrentMaterial() const { return m_currentMaterial; }

        // マネージャー設定
        void setMaterialManager(Graphics::MaterialManager* manager) { m_materialManager = manager; }

    private:
        std::shared_ptr<Graphics::Material> m_currentMaterial;
        Graphics::MaterialManager* m_materialManager = nullptr;

        // UI描画関数
        void drawMaterialProperties();
        //void drawBasicProperties();
        //void drawPBRProperties();
        //void drawAdvancedProperties();
    };
}