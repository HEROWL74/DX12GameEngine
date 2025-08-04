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

            // メインコンテンツエリア
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

            // プレビューエリア
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

            // 名前でソート
            std::sort(m_assets.begin(), m_assets.end(),
                [](const AssetInfo& a, const AssetInfo& b) {
                    if (a.type != b.type)
                    {
                        return a.type < b.type; // フォルダを先に
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
        // 検索フィルター
        char searchBuffer[256];
        strncpy_s(searchBuffer, m_searchFilter.c_str(), sizeof(searchBuffer) - 1);
        if (ImGui::InputText("Search", searchBuffer, sizeof(searchBuffer)))
        {
            m_searchFilter = searchBuffer;
        }

        ImGui::SameLine();

        // 表示切り替え
        if (ImGui::Button(m_showGrid ? "List" : "Grid"))
        {
            m_showGrid = !m_showGrid;
        }

        ImGui::SameLine();

        // リフレッシュボタン
        if (ImGui::Button("Refresh"))
        {
            refreshAssets();
        }

        ImGui::SameLine();

        // アイコンサイズスライダー（グリッド表示時のみ）
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

            // セルの位置計算
            if (index > 0 && (index % columnCount) != 0)
            {
                ImGui::SameLine();
            }

            // アセットアイコン描画
            ImGui::BeginGroup();

            // 選択状態の背景
            bool isSelected = (m_selectedAsset == &asset);
            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            // アイコンボタン
            if (ImGui::Button("##icon", ImVec2(m_iconSize, m_iconSize)))
            {
                m_selectedAsset = &asset;
                loadAssetPreview(asset);
            }

            if (isSelected)
            {
                ImGui::PopStyleColor();
            }

            // ダブルクリック処理
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

            // ドラッグ＆ドロップ
            handleDragDrop(asset);

            // アセット名
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

                // 名前
                ImGui::TableNextColumn();
                bool isSelected = (m_selectedAsset == &asset);
                if (ImGui::Selectable(asset.name.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_selectedAsset = &asset;
                    loadAssetPreview(asset);
                }

                // ダブルクリック処理
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

                // ドラッグ＆ドロップ
                handleDragDrop(asset);

                // タイプ
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

                // サイズ
                ImGui::TableNextColumn();
                if (asset.type != AssetInfo::Type::Folder)
                {
                    try
                    {
                        auto size = std::filesystem::file_size(asset.path);
                        if (size < 1024)
                            ImGui::Text("%llu B", size);
                        else if (size < 1024 * 1024)
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

            // タイプ別のプレビュー
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
        // 右クリックメニューの実装（必要に応じて）
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
                // マテリアルファイルの読み込み（簡易実装）
                // 実際はMaterialSerializerを使用
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