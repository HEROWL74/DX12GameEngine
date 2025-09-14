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
                drawContextMenu();
            }
            ImGui::EndChild();

 
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

        char searchBuffer[256];
        strncpy_s(searchBuffer, m_searchFilter.c_str(), sizeof(searchBuffer) - 1);
        if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer)))
        {
            m_searchFilter = searchBuffer;
        }

        ImGui::SameLine();

        if (ImGui::Button(m_showGrid ? "List" : "Grid"))
        {
            m_showGrid = !m_showGrid;
        }

        ImGui::SameLine();


        if (ImGui::Button("Up"))
        {
            std::filesystem::path parent = std::filesystem::path(m_projectPath).parent_path();
            if (!parent.empty() && std::filesystem::exists(parent))
            {
                setProjectPath(parent.string());
            }
        }
        ImGui::SameLine();


        if (ImGui::Button("Refresh"))
        {
            refreshAssets();
        }

        ImGui::SameLine();


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

         
            if (index > 0 && (index % columnCount) != 0)
            {
                ImGui::SameLine();
            }

          
            ImGui::BeginGroup();

      
            bool isSelected = (m_selectedAsset == &asset);
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            if (asset.type == AssetInfo::Type::Folder && m_folderIcon && m_imguiManager)
            {
                static ImTextureID folderIconID = 0;
                if (!folderIconID)
                    folderIconID = m_imguiManager->registerTexture(m_folderIcon.get());

                ImGui::Image(folderIconID, ImVec2(m_iconSize, m_iconSize));

                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
                {
                    m_selectedAsset = &asset;
                    loadAssetPreview(asset);
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    setProjectPath(asset.path.string());
                }
            }
            else
            {
                if (ImGui::Button("##icon", ImVec2(m_iconSize, m_iconSize)))
                {
                    m_selectedAsset = &asset;
                    loadAssetPreview(asset);
                }
            }


            if (isSelected)
            {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                if (asset.type == AssetInfo::Type::Folder)
                {
                    setProjectPath(asset.path.string());
                }
                else if (asset.type == AssetInfo::Type::Script)
                {
                    Engine::Scripting::LuaScriptUtility::openInVSCode(asset.path.string());
                }
                else if (m_assetDropCallback)
                {
                    m_assetDropCallback(asset);
                }
            }

        
            handleDragDrop(asset);
            // 名前部分の描画
            if (asset.renaming)
            {
                ImGui::SetNextItemWidth(m_iconSize + 20);

                if (ImGui::InputText("##rename", asset.renameBuffer, sizeof(asset.renameBuffer),
                    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                {
                    // Enterで確定 → ファイルリネーム
                    std::filesystem::path newPath = asset.path.parent_path() / asset.renameBuffer;
                    try {
                        std::filesystem::rename(asset.path, newPath);
                        asset.path = newPath;
                        asset.name = newPath.filename().string();
                        asset.renaming = false;
                        refreshAssets();
                    }
                    catch (...) {
                        Utils::log_warning("Rename failed");
                    }
                }

                // Escでキャンセルできるようにする（任意）
                if (ImGui::IsItemDeactivatedAfterEdit() && !ImGui::IsItemActive())
                {
                    asset.renaming = false;
                }
            }
            else
            {
                ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + m_iconSize);
                ImGui::TextWrapped("%s", asset.name.c_str());
                ImGui::PopTextWrapPos();
            }


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

                ImGui::TableNextColumn();
                bool isSelected = (m_selectedAsset == &asset);

                if (asset.renaming)
                {
                    ImGui::SetNextItemWidth(200);

                    if (ImGui::InputText("##rename", asset.renameBuffer, sizeof(asset.renameBuffer),
                        ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
                    {
                        std::filesystem::path newPath = asset.path.parent_path() / asset.renameBuffer;
                        try {
                            std::filesystem::rename(asset.path, newPath);
                            asset.path = newPath;
                            asset.name = newPath.filename().string();
                            asset.renaming = false;
                            refreshAssets();
                        }
                        catch (...) {
                            Utils::log_warning("Rename failed");
                        }
                    }

                    if (ImGui::IsItemDeactivatedAfterEdit() && !ImGui::IsItemActive())
                    {
                        asset.renaming = false;
                    }
                }
                else
                {
                    if (ImGui::Selectable(asset.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                    {
                        m_selectedAsset = &asset;
                        loadAssetPreview(asset);
                    }
                }




                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                {
                    if (asset.type == AssetInfo::Type::Folder)
                    {
                        setProjectPath(asset.path.string());
                    }
                    else if (asset.type == AssetInfo::Type::Script)
                    {
                        Engine::Scripting::LuaScriptUtility::openInVSCode(asset.path.string());
                    }
                    else if (m_assetDropCallback)
                    {
                        m_assetDropCallback(asset);
                    }
                }

             
                handleDragDrop(asset);

      
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
        if (ImGui::BeginPopupContextWindow("ProjectWindowContext", ImGuiPopupFlags_MouseButtonRight))
        {
            if (ImGui::BeginMenu("Create"))
            {
                if (ImGui::MenuItem("Lua Script"))
                {
                    std::string newScriptPath = generateUniqueScriptPath();

                    if (Engine::Scripting::LuaScriptUtility::createNewScript(newScriptPath))
                    {
                        Utils::log_info(std::format("Lua script created: {}", newScriptPath));

                        //リスト更新
                        refreshAssets();

                        //新しく作ったアセットを選択 & Renameモードにする
                        for (auto& asset : m_assets)
                        {
                            if (asset.path == newScriptPath)
                            {
                                m_selectedAsset = &asset;
                                asset.renaming = true;
                                strncpy_s(asset.renameBuffer, asset.name.c_str(), sizeof(asset.renameBuffer));
                                break;
                            }
                        }
                    }
                }

                ImGui::EndMenu();
            }


            if (m_selectedAsset && ImGui::MenuItem("Delete"))
            {
                try {
                    std::filesystem::remove(m_selectedAsset->path);
                    Utils::log_info(std::format("Deleted asset: {}", m_selectedAsset->path.string()));
                    refreshAssets();
                    m_selectedAsset = nullptr;
                }
                catch (...) {
                    Utils::log_warning("Failed to delete asset");
                }
            }
            ImGui::EndPopup();
        }
        
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
        else if (ext == ".lua") 
        {
            return AssetInfo::Type::Script;
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
            AssetPayload payload{};
            auto s = asset.path.string();
            strncpy_s(payload.path, s.c_str(), sizeof(payload.path) - 1);
            payload.type = static_cast<int>(asset.type);

            //POD を渡す（ImGui 内部でコピーされるのでローカルでOK）
            ImGui::SetDragDropPayload("ASSET", &payload, sizeof(payload));

            ImGui::Text("Dragging: %s", asset.name.c_str());
            ImGui::EndDragDropSource();
        }
    }

    std::string ProjectWindow::generateUniqueScriptPath()
    {
        int counter = 0;
        std::string baseName = "NewLuaScript";
        std::string fileName;
        std::filesystem::path scriptPath;

        do
        {
            fileName = (counter == 0)
                ? baseName + ".lua"
                : baseName + " " + std::to_string(counter) + ".lua";

            scriptPath = std::filesystem::path(m_projectPath) / fileName;
            counter++;
        } while (std::filesystem::exists(scriptPath));

        return scriptPath.string();
    }

    void ProjectWindow::setTextureManager(Graphics::TextureManager* textureManager) 
    {
        m_textureManager = textureManager; 

        if (m_textureManager)
        {
            m_folderIcon = m_textureManager->loadTexture("assets/images/ProjectWindowFolder.png");
        }
    }
}
