// src/Graphics/TriangleRenderer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Device.hpp"
#include "Shader.hpp"
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // 頂点データ構造体
    struct Vertex
    {
        float position[3];  // x, y, z
        float color[3];     // r, g, b
    };

    // 三角形描画専用のレンダラー
    class TriangleRenderer
    {
    public:
        TriangleRenderer() = default;
        ~TriangleRenderer() = default;

        // コピー・ムーブ禁止
        TriangleRenderer(const TriangleRenderer&) = delete;
        TriangleRenderer& operator=(const TriangleRenderer&) = delete;
        TriangleRenderer(TriangleRenderer&&) = delete;
        TriangleRenderer& operator=(TriangleRenderer&&) = delete;

        // 初期化
        [[nodiscard]] Utils::VoidResult initialize(Device* device);

        // 三角形を描画
        void render(ID3D12GraphicsCommandList* commandList);

        // 有効かチェック
        [[nodiscard]] bool isValid() const noexcept { return m_rootSignature != nullptr; }

    private:
        Device* m_device = nullptr;
        ShaderManager m_shaderManager;

        // 描画リソース
        ComPtr<ID3D12RootSignature> m_rootSignature;        // ルートシグネチャ
        ComPtr<ID3D12PipelineState> m_pipelineState;        // パイプラインステート
        ComPtr<ID3D12Resource> m_vertexBuffer;              // 頂点バッファ
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};      // 頂点バッファビュー

        // 三角形の頂点データ
        std::array<Vertex, 3> m_triangleVertices;

        // 初期化用メソッド
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createShaders();
        [[nodiscard]] Utils::VoidResult createPipelineState();
        [[nodiscard]] Utils::VoidResult createVertexBuffer();

        // 三角形の頂点データを設定
        void setupTriangleVertices();
    };
}