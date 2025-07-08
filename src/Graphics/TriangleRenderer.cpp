// src/Graphics/TriangleRenderer.cpp
#include "TriangleRenderer.hpp"
#include <format>

namespace Engine::Graphics
{
    Utils::VoidResult TriangleRenderer::initialize(Device* device)
    {
        m_device = device;
        Utils::log_info("Initializing Triangle Renderer...");

        // 三角形の頂点データを設定
        setupTriangleVertices();

        // 各コンポーネントを順番に初期化
        auto rootSigResult = createRootSignature();
        if (!rootSigResult) return rootSigResult;

        auto shaderResult = createShaders();
        if (!shaderResult) return shaderResult;

        auto pipelineResult = createPipelineState();
        if (!pipelineResult) return pipelineResult;

        auto vertexBufferResult = createVertexBuffer();
        if (!vertexBufferResult) return vertexBufferResult;

        Utils::log_info("Triangle Renderer initialized successfully!");
        return {};
    }

    void TriangleRenderer::render(ID3D12GraphicsCommandList* commandList)
    {
        // ルートシグネチャとパイプラインステートを設定
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        // プリミティブトポロジを設定（三角形リスト）
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 頂点バッファを設定
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // 三角形を描画（3頂点、1インスタンス）
        commandList->DrawInstanced(3, 1, 0, 0);
    }

    Utils::VoidResult TriangleRenderer::createRootSignature()
    {
        // 空のルートシグネチャを作成（今回は定数バッファやテクスチャを使わない）
        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = 0;
        rootSignatureDesc.pParameters = nullptr;
        rootSignatureDesc.NumStaticSamplers = 0;
        rootSignatureDesc.pStaticSamplers = nullptr;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;

        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
        if (FAILED(hr))
        {
            std::string errorMsg = "Failed to serialize root signature";
            if (error)
            {
                errorMsg += std::format(": {}", static_cast<char*>(error->GetBufferPointer()));
            }
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation, errorMsg, hr));
        }

        CHECK_HR(m_device->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation, "Failed to create root signature");

        return {};
    }

    Utils::VoidResult TriangleRenderer::createShaders()
    {
        // 頂点シェーダーをコンパイル
        auto vsResult = m_shaderManager.compileFromString(
            BasicShaders::getColorVertexShader(),
            "main",
            ShaderType::Vertex,
            "ColorVertexShader"
        );
        if (!vsResult)
        {
            return std::unexpected(vsResult.error());
        }

        // ピクセルシェーダーをコンパイル
        auto psResult = m_shaderManager.compileFromString(
            BasicShaders::getColorPixelShader(),
            "main",
            ShaderType::Pixel,
            "ColorPixelShader"
        );
        if (!psResult)
        {
            return std::unexpected(psResult.error());
        }

        // シェーダーを登録
        m_shaderManager.registerShader("vs_color", vsResult.value());
        m_shaderManager.registerShader("ps_color", psResult.value());

        return {};
    }

    Utils::VoidResult TriangleRenderer::createPipelineState()
    {
        // シェーダーを取得
        const ShaderInfo* vertexShader = m_shaderManager.getShader("vs_color");
        const ShaderInfo* pixelShader = m_shaderManager.getShader("ps_color");

        CHECK_CONDITION(vertexShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Vertex shader not found");
        CHECK_CONDITION(pixelShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Pixel shader not found");

        // 入力レイアウトを定義
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // パイプラインステートの設定
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->blob->GetBufferPointer(), vertexShader->blob->GetBufferSize() };
        psoDesc.PS = { pixelShader->blob->GetBufferPointer(), pixelShader->blob->GetBufferSize() };

        // ラスタライザーステート
        psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = TRUE;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // ブレンドステート
        psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
        psoDesc.BlendState.IndependentBlendEnable = FALSE;
        const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
            FALSE, FALSE,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
            D3D12_LOGIC_OP_NOOP,
            D3D12_COLOR_WRITE_ENABLE_ALL,
        };
        for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        {
            psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
        }

        // その他の設定
        psoDesc.DepthStencilState.DepthEnable = FALSE;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.SampleDesc.Count = 1;

        CHECK_HR(m_device->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)),
            Utils::ErrorType::ResourceCreation, "Failed to create graphics pipeline state");

        return {};
    }

    Utils::VoidResult TriangleRenderer::createVertexBuffer()
    {
        const UINT vertexBufferSize = sizeof(m_triangleVertices);

        // 頂点バッファ用のヒーププロパティ（アップロードヒープ）
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // リソース記述子
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = vertexBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        CHECK_HR(m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_vertexBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create vertex buffer");

        // 頂点データをバッファにコピー
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange{ 0, 0 }; // CPUから読み取らないので範囲は0

        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map vertex buffer");

        memcpy(pVertexDataBegin, m_triangleVertices.data(), sizeof(m_triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // 頂点バッファビューを設定
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return {};
    }

    void TriangleRenderer::setupTriangleVertices()
    {
        // カラフルな三角形の頂点データ
        m_triangleVertices = { {
            { { 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} },  // 上：赤
            { { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} },  // 右下：緑
            { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} }   // 左下：青
        } };
    }
}