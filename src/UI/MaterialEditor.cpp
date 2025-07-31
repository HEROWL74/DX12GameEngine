//src/UI/MaterialEditor.cpp
#include "MaterialEditor.hpp"

namespace Engine::UI
{
    MaterialEditorWindow::MaterialEditorWindow()
        : ImGuiWindow("Material Editor", true)
    {
    }

    void MaterialEditorWindow::draw()
    {
        if (!m_visible) return;

        if (ImGui::Begin(m_title.c_str(), &m_visible))
        {
            if (m_currentMaterial)
            {
                ImGui::Text("Material: %s", m_currentMaterial->getName().c_str());
                ImGui::Separator();
                drawMaterialProperties();
            }
            else
            {
                ImGui::Text("No material selected");
            }
        }
        ImGui::End();
    }

    void MaterialEditorWindow::setMaterial(std::shared_ptr<Graphics::Material> material)
    {
        m_currentMaterial = material;
    }

    void MaterialEditorWindow::drawMaterialProperties()
    {
        if (!m_currentMaterial) return;

        auto properties = m_currentMaterial->getProperties();
        bool changed = false;

        if (ImGui::CollapsingHeader("Basic Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // アルベド
            float albedo[3] = { properties.albedo.x, properties.albedo.y, properties.albedo.z };
            if (ImGui::ColorEdit3("Albedo", albedo))
            {
                properties.albedo = Math::Vector3(albedo[0], albedo[1], albedo[2]);
                changed = true;
            }

            // アルファ
            if (ImGui::SliderFloat("Alpha", &properties.alpha, 0.0f, 1.0f))
            {
                changed = true;
            }
        }

        if (ImGui::CollapsingHeader("PBR Properties", ImGuiTreeNodeFlags_DefaultOpen))
        {
            // メタリック
            if (ImGui::SliderFloat("Metallic", &properties.metallic, 0.0f, 1.0f))
            {
                changed = true;
            }

            // ラフネス
            if (ImGui::SliderFloat("Roughness", &properties.roughness, 0.0f, 1.0f))
            {
                changed = true;
            }

            // AO
            if (ImGui::SliderFloat("AO", &properties.ao, 0.0f, 1.0f))
            {
                changed = true;
            }
        }

        if (ImGui::CollapsingHeader("Emission"))
        {
            // 発光色
            float emissive[3] = { properties.emissive.x, properties.emissive.y, properties.emissive.z };
            if (ImGui::ColorEdit3("Emissive", emissive))
            {
                properties.emissive = Math::Vector3(emissive[0], emissive[1], emissive[2]);
                changed = true;
            }

            // 発光強度
            if (ImGui::SliderFloat("Emissive Strength", &properties.emissiveStrength, 0.0f, 5.0f))
            {
                changed = true;
            }
        }

        if (changed)
        {
            m_currentMaterial->setProperties(properties);
        }
    }
}