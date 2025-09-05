//src/UI/ProjectWindow.cpp
#include "ProjectWindow.hpp"
#include <format>
#include <algorithm>

namespace Engine::UI
{
    ProjectWindow::ProjectWindow()
        : ImGuiWindow("Project", true)
    {
        refreshAssets();
    }

    void ProjectWindow::draw()
    {
        if (!m_visible) return;

        if (ImGui::Begin(m_title.c_str(), &m_visible))
        {
            drawToolbar();
            ImGui::Separator();

            // 繝｡繧､繝ｳ繧ｳ繝ｳ繝・Φ繝・お繝ｪ繧｢
            if (ImGui::BeginChild("AssetArea", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())))
            {
                if (m_showGrid)
                {
                    drawAssetGrid();
                }
                else
                {
                    drawAssetList();
                }
            }
            ImGui::EndChild();

            // 繝励Ξ繝薙Η繝ｼ繧ｨ繝ｪ繧｢
            drawAssetPreview();
        }
        ImGui::End();
    }

    void ProjectWindow::setProjectPath(const std::string& path)
    {
        m_projectPath = path;
        refreshAssets();
    }

    void ProjectWindow::refreshAssets()
    {
        m_assets.clear();
        m_selectedAsset = nullptr;

        if (!std::filesystem::exists(m_projectPath))
        {
            return;
        }

        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(m_projectPath))
            {
                AssetInfo asset;
                asset.path = entry.path();
                asset.name = entry.path().filename().string();
                asset.extension = entry.path().extension().string();
                asset.type = getAssetType(entry.path());

                m_assets.push_back(asset);
            }

            // 蜷榊燕縺ｧ繧ｽ繝ｼ繝・
            std::sort(m_assets.begin(), m_assets.end(),
                [](const AssetInfo& a, const AssetInfo& b) {
                    if (a.type != b.type)
                    {
                        return a.type < b.type; // 繝輔か繝ｫ繝繧貞・縺ｫ
                    }
                    return a.name < b.name;
                });
        }
        catch (const std::exception& e)
        {
            Utils::log_warning(std::format("Failed to refresh assets: {}", e.what()));
        }
    }

    void ProjectWindow::drawToolbar()
    {
        // 讀懃ｴ｢繝輔ぅ繝ｫ繧ｿ繝ｼ
        char searchBuffer[256];
        strncpy_s(searchBuffer, m_searchFilter.c_str(), sizeof(searchBuffer) - 1);
        if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer)))
        {
            m_searchFilter = searchBuffer;
        }

        ImGui::SameLine();

        // 陦ｨ遉ｺ蛻・ｊ譖ｿ縺・
        if (ImGui::Button(m_showGrid ? "List" : "Grid"))
        {
            m_showGrid = !m_showGrid;
        }

        ImGui::SameLine();

        // 繝ｪ繝輔Ξ繝・す繝･繝懊ち繝ｳ
        if (ImGui::Button("Refresh"))
        {
            refreshAssets();
        }

        ImGui::SameLine();

        // 繧｢繧､繧ｳ繝ｳ繧ｵ繧､繧ｺ繧ｹ繝ｩ繧､繝繝ｼ・医げ繝ｪ繝・ラ陦ｨ遉ｺ譎ゅ・縺ｿ・・
        if (m_showGrid)
        {
            ImGui::SetNextItemWidth(100);
            ImGui::SliderFloat("Size", &m_iconSize, 32.0f, 128.0f);
        }
    }

    void ProjectWindow::drawAssetGrid()
    {
        const float panelWidth = ImGui::GetContentRegionAvail().x;
        const float cellSize = m_iconSize + 16.0f;
        const int columnCount = max(1, (int)(panelWidth / cellSize));

        int index = 0;
        for (auto& asset : m_assets)
        {
            if (!matchesFilter(asset)) continue;

            ImGui::PushID(index);

            // 繧ｻ繝ｫ縺ｮ菴咲ｽｮ險育ｮ・
            if (index > 0 && (index % columnCount) != 0)
            {
                ImGui::SameLine();
            }

            // 繧｢繧ｻ繝・ヨ繧｢繧､繧ｳ繝ｳ謠冗判
            ImGui::BeginGroup();

            // 驕ｸ謚樒憾諷九・閭梧勹
            bool isSelected = (m_selectedAsset == &asset);
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            // 繧｢繧､繧ｳ繝ｳ繝懊ち繝ｳ
            if (ImGui::Button("##icon", ImVec2(m_iconSize, m_iconSize)))
            {
                m_selectedAsset = &asset;
                loadAssetPreview(asset);
            }

            if (isSelected)
            {
                ImGui::PopStyleColor();
            }

            // 繝繝悶Ν繧ｯ繝ｪ繝・け蜃ｦ逅・
            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (asset.type == AssetInfo::Type::Folder)
                {
                    setProjectPath(asset.path.string());
                }
                else if (m_assetDropCallback)
                {
                    m_assetDropCallback(asset);
                }
            }

            // 繝峨Λ繝・げ・・ラ繝ｭ繝・・
            handleDragDrop(asset);

            // 繧｢繧ｻ繝・ヨ蜷・
            ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + m_iconSize);
            ImGui::TextWrapped("%s", asset.name.c_str());
            ImGui::PopTextWrapPos();

            ImGui::EndGroup();

            ImGui::PopID();
            index++;
        }
    }

    void ProjectWindow::drawAssetList()
    {
        if (ImGui::BeginTable("AssetTable", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_Sortable))
        {
            ImGui::TableSetupColumn("Name");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Size");
            ImGui::TableHeadersRow();

            int index = 0;
            for (auto& asset : m_assets)
            {
                if (!matchesFilter(asset)) continue;

                ImGui::PushID(index);
                ImGui::TableNextRow();

                // 蜷榊燕
                ImGui::TableNextColumn();
                bool isSelected = (m_selectedAsset == &asset);
                if (ImGui::Selectable(asset.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_selectedAsset = &asset;
                    loadAssetPreview(asset);
                }

                // 繝繝悶Ν繧ｯ繝ｪ繝・け蜃ｦ逅・


                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    if (asset.type == AssetInfo::Type::Folder)
                    {
                        setProjectPath(asset.path.string());
                    }
                    else if (m_assetDropCallback)
                    {
                        m_assetDropCallback(asset);
                    }
                }

                // 繝峨Λ繝・げ・・ラ繝ｭ繝・・
                handleDragDrop(asset);

                // 繧ｿ繧､繝・
                ImGui::TableNextColumn();
                const char* typeStr = "Unknown";
                switch (asset.type)
                {
                case AssetInfo::Type::Folder: typeStr = "Folder"; break;
                case AssetInfo::Type::Texture: typeStr = "Texture"; break;
                case AssetInfo::Type::Material: typeStr = "Material"; break;
                case AssetInfo::Type::Shader: typeStr = "Shader"; break;
                }
                ImGui::Text("%s", typeStr);

                // 繧ｵ繧､繧ｺ
                ImGui::TableNextColumn();
                if (asset.type != AssetInfo::Type::Folder)
                {
                    try
                    {
                        auto size = std::filesystem::file_size(asset.path);
                        if (size < 1024)
                            ImGui::Text("%llu B", size);
                        else if (size < static_cast<unsigned long long>(1024) * 1024)
                            ImGui::Text("%.1f KB", size / 1024.0f);
                        else
                            ImGui::Text("%.1f MB", size / (1024.0f * 1024.0f));
                    }
                    catch (...)
                    {
                        ImGui::Text("-");
                    }
                }

                ImGui::PopID();
                index++;
            }

            ImGui::EndTable();
        }
    }

    void ProjectWindow::drawAssetPreview()
    {
        if (!m_selectedAsset) return;

        if (ImGui::BeginChild("Preview", ImVec2(0, 100), true))
        {
            ImGui::Text("Selected: %s", m_selectedAsset->name.c_str());
            ImGui::Text("Path: %s", m_selectedAsset->path.string().c_str());

            // 繧ｿ繧､繝怜挨縺ｮ繝励Ξ繝薙Η繝ｼ
            switch (m_selectedAsset->type)
            {
            case AssetInfo::Type::Texture:
                if (m_selectedAsset->texture)
                {
                    ImGui::Text("Texture: %dx%d",
                        m_selectedAsset->texture->getWidth(),
                        m_selectedAsset->texture->getHeight());
                }
                break;

            case AssetInfo::Type::Material:
                if (m_selectedAsset->material)
                {
                    const auto& props = m_selectedAsset->material->getProperties();
                    ImGui::Text("Albedo: %.2f, %.2f, %.2f", props.albedo.x, props.albedo.y, props.albedo.z);
                    ImGui::Text("Metallic: %.2f, Roughness: %.2f", props.metallic, props.roughness);
                }
                break;
            }
        }
        ImGui::EndChild();
    }

    void ProjectWindow::drawContextMenu()
    {
        // 蜿ｳ繧ｯ繝ｪ繝・け繝｡繝九Η繝ｼ縺ｮ螳溯｣・ｼ亥ｿ・ｦ√↓蠢懊§縺ｦ・・
    }

    AssetInfo::Type ProjectWindow::getAssetType(const std::filesystem::path& path)
    {
        if (std::filesystem::is_directory(path))
        {
            return AssetInfo::Type::Folder;
        }

        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".dds")
        {
            return AssetInfo::Type::Texture;
        }
        else if (ext == ".json" && path.stem().string().find("_material") != std::string::npos)
        {
            return AssetInfo::Type::Material;
        }
        else if (ext == ".hlsl")
        {
            return AssetInfo::Type::Shader;
        }

        return AssetInfo::Type::Unknown;
    }

    void ProjectWindow::loadAssetPreview(AssetInfo& asset)
    {
        switch (asset.type)
        {
        case AssetInfo::Type::Texture:
            if (m_textureManager && !asset.texture)
            {
                asset.texture = m_textureManager->loadTexture(asset.path.string());
            }
            break;

        case AssetInfo::Type::Material:
            if (m_materialManager && !asset.material)
            {
                // 繝槭ユ繝ｪ繧｢繝ｫ繝輔ぃ繧､繝ｫ縺ｮ隱ｭ縺ｿ霎ｼ縺ｿ・育ｰ｡譏灘ｮ溯｣・ｼ・
                // 螳滄圀縺ｯMaterialSerializer繧剃ｽｿ逕ｨ
                asset.material = m_materialManager->createMaterial(asset.name);
            }
            break;
        }
    }

    bool ProjectWindow::matchesFilter(const AssetInfo& asset) const
    {
        if (m_searchFilter.empty()) return true;

        std::string lowerName = asset.name;
        std::string lowerFilter = m_searchFilter;

        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        std::transform(lowerFilter.begin(), lowerFilter.end(), lowerFilter.begin(), ::tolower);

        return lowerName.find(lowerFilter) != std::string::npos;
    }

    void ProjectWindow::handleDragDrop(const AssetInfo& asset)
    {
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("ASSET", &asset, sizeof(AssetInfo));
            ImGui::Text("Dragging: %s", asset.name.c_str());
            ImGui::EndDragDropSource();
        }
    }
}
