// src/Graphics/CubeRenderer.cpp
#include "CubeRenderer.hpp"
#include <format>

namespace Engine::Graphics
{
    Utils::VoidResult CubeRenderer::initialize(Device* device, ShaderManager* shaderManager)
    {
        // 繝・ヰ繝・げ逕ｨ: null繝√ぉ繝・け
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        m_shaderManager = shaderManager;
        Utils::log_info("Initializing Cube Renderer...");

        // 螳壽焚繝舌ャ繝輔ぃ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ繧貞・譛溷喧
        auto constantBufferResult = m_constantBufferManager.initialize(device);
        if (!constantBufferResult) {
            Utils::log_error(constantBufferResult.error());
            return constantBufferResult;
        }

        // 遶区婿菴薙・鬆らせ繝・・繧ｿ繧定ｨｭ螳・
        setupCubeVertices();

        // 繝ｯ繝ｼ繝ｫ繝芽｡悟・繧貞・譛溷喧
        updateWorldMatrix();

        // 蜷・さ繝ｳ繝昴・繝阪Φ繝医ｒ鬆・ｬ｡蛻晄悄蛹・
        auto rootSigResult = createRootSignature();
        if (!rootSigResult) {
            Utils::log_error(rootSigResult.error());
            return rootSigResult;
        }

        auto shaderResult = createShaders();
        if (!shaderResult) {
            Utils::log_error(shaderResult.error());
            return shaderResult;
        }

        auto pipelineResult = createPipelineState();
        if (!pipelineResult) {
            Utils::log_error(pipelineResult.error());
            return pipelineResult;
        }

        auto vertexBufferResult = createVertexBuffer();
        if (!vertexBufferResult) {
            Utils::log_error(vertexBufferResult.error());
            return vertexBufferResult;
        }

        auto indexBufferResult = createIndexBuffer();
        if (!indexBufferResult) {
            Utils::log_error(indexBufferResult.error());
            return indexBufferResult;
        }

        Utils::log_info("Cube Renderer initialized successfully!");
        return {};
    }

    void CubeRenderer::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
    {
        // 繝・ヵ繧ｩ繝ｫ繝医・繝・Μ繧｢繝ｫ縺後↑縺・ｴ蜷医・險ｭ螳・
        if (!m_material && m_materialManager)
        {
            m_material = m_materialManager->getDefaultMaterial();
        }

        // 螳壽焚繝舌ャ繝輔ぃ繧呈峩譁ｰ
        CameraConstants cameraConstants{};
        cameraConstants.viewMatrix = camera.getViewMatrix();
        cameraConstants.projectionMatrix = camera.getProjectionMatrix();
        cameraConstants.viewProjectionMatrix = camera.getViewProjectionMatrix();
        cameraConstants.cameraPosition = camera.getPosition();

        ObjectConstants objectConstants{};
        objectConstants.worldMatrix = m_worldMatrix;
        objectConstants.worldViewProjectionMatrix = camera.getViewProjectionMatrix() * m_worldMatrix;
        objectConstants.objectPosition = m_position;

        m_constantBufferManager.updateCameraConstants(frameIndex, cameraConstants);
        m_constantBufferManager.updateObjectConstants(frameIndex, objectConstants);

        // 繝ｫ繝ｼ繝医す繧ｰ繝阪メ繝｣縺ｨ繝代う繝励Λ繧､繝ｳ繧ｹ繝・・繝医ｒ險ｭ螳・
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        // 螳壽焚繝舌ャ繝輔ぃ繧定ｨｭ螳・
        commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferManager.getCameraConstantsGPUAddress(frameIndex));
        commandList->SetGraphicsRootConstantBufferView(1, m_constantBufferManager.getObjectConstantsGPUAddress(frameIndex));

        // 繝槭ユ繝ｪ繧｢繝ｫ螳壽焚繝舌ャ繝輔ぃ繧偵ヰ繧､繝ｳ繝・
        if (m_material && m_material->getConstantBuffer())
        {
            commandList->SetGraphicsRootConstantBufferView(2,
                m_material->getConstantBuffer()->GetGPUVirtualAddress());
        }

        // 繝励Μ繝溘ユ繧｣繝悶ヨ繝昴Ο繧ｸ繧定ｨｭ螳夲ｼ井ｸ芽ｧ貞ｽ｢繝ｪ繧ｹ繝茨ｼ・
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // 鬆らせ繝舌ャ繝輔ぃ縺ｨ繧､繝ｳ繝・ャ繧ｯ繧ｹ繝舌ャ繝輔ぃ繧定ｨｭ螳・
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        commandList->IASetIndexBuffer(&m_indexBufferView);

        // 遶区婿菴薙ｒ謠冗判・・6繧､繝ｳ繝・ャ繧ｯ繧ｹ縲・繧､繝ｳ繧ｹ繧ｿ繝ｳ繧ｹ・・
        commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
    }

    Utils::VoidResult CubeRenderer::createRootSignature()
    {
        // 3縺､縺ｮ螳壽焚繝舌ャ繝輔ぃ逕ｨ繝ｫ繝ｼ繝医す繧ｰ繝阪メ繝｣・・amera, Object, Material・・
        D3D12_ROOT_PARAMETER rootParameters[3];

        // 繧ｫ繝｡繝ｩ螳壽焚繝舌ャ繝輔ぃ (b0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // 繧ｪ繝悶ず繧ｧ繧ｯ繝亥ｮ壽焚繝舌ャ繝輔ぃ (b1)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        rootParameters[1].Descriptor.RegisterSpace = 0;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // 繝槭ユ繝ｪ繧｢繝ｫ螳壽焚繝舌ャ繝輔ぃ (b2)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[2].Descriptor.ShaderRegister = 2;
        rootParameters[2].Descriptor.RegisterSpace = 0;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // Static Sampler繧定ｿｽ蜉・医す繧ｧ繝ｼ繝繝ｼ縺ｮs0縺ｫ蟇ｾ蠢懶ｼ・
        D3D12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderRegister = 0; // s0
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = _countof(rootParameters);
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 1; // 1縺､縺ｮ繧ｵ繝ｳ繝励Λ繝ｼ繧定ｿｽ蜉
        rootSignatureDesc.pStaticSamplers = &samplerDesc;
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

    Utils::VoidResult CubeRenderer::createPBRRootSignature()
    {
        // 4縺､縺ｮ繝ｫ繝ｼ繝医ヱ繝ｩ繝｡繝ｼ繧ｿ: Camera, Object, Material, Textures
        D3D12_ROOT_PARAMETER rootParameters[4];

        // 繧ｫ繝｡繝ｩ螳壽焚繝舌ャ繝輔ぃ (b0)
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // 繧ｪ繝悶ず繧ｧ繧ｯ繝亥ｮ壽焚繝舌ャ繝輔ぃ (b1)
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        rootParameters[1].Descriptor.RegisterSpace = 0;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // 繝槭ユ繝ｪ繧｢繝ｫ螳壽焚繝舌ャ繝輔ぃ (b2)
        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[2].Descriptor.ShaderRegister = 2;
        rootParameters[2].Descriptor.RegisterSpace = 0;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // 繝・け繧ｹ繝√Ε繝・せ繧ｯ繝ｪ繝励ち繝・・繝悶Ν (t0-t5)
        static D3D12_DESCRIPTOR_RANGE textureRange{};
        textureRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        textureRange.NumDescriptors = 6;
        textureRange.BaseShaderRegister = 0;
        textureRange.RegisterSpace = 0;
        textureRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[3].DescriptorTable.pDescriptorRanges = &textureRange;
        rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        // 繧ｹ繧ｿ繝・ぅ繝・け繧ｵ繝ｳ繝励Λ繝ｼ
        D3D12_STATIC_SAMPLER_DESC samplerDesc{};
        samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 1;
        samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
        samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
        samplerDesc.ShaderRegister = 0;
        samplerDesc.RegisterSpace = 0;
        samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = _countof(rootParameters);
        rootSignatureDesc.pParameters = rootParameters;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &samplerDesc;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> signature;
        ComPtr<ID3DBlob> error;
        HRESULT hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);

        if (FAILED(hr))
        {
            std::string errorMsg = "Failed to serialize PBR root signature";
            if (error)
            {
                errorMsg += std::format(": {}", static_cast<char*>(error->GetBufferPointer()));
            }
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation, errorMsg, hr));
        }

        CHECK_HR(m_device->getDevice()->CreateRootSignature(0, signature->GetBufferPointer(),
            signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation, "Failed to create PBR root signature");

        return {};
    }

    Utils::VoidResult CubeRenderer::createShaders()
    {
        // ShaderCompileDesc 繧剃ｽｿ逕ｨ縺励※繧ｷ繧ｧ繝ｼ繝繝ｼ繧偵Ο繝ｼ繝・
        ShaderCompileDesc vsDesc;
        vsDesc.filePath = "assets/shaders/BasicVertex.hlsl";
        vsDesc.entryPoint = "main";
        vsDesc.type = ShaderType::Vertex;
        vsDesc.enableDebug = true;

        auto vertexShader = m_shaderManager->loadShader(vsDesc);
        if (!vertexShader)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load vertex shader"));
        }

        ShaderCompileDesc psDesc;
        psDesc.filePath = "assets/shaders/BasicPixel.hlsl";
        psDesc.entryPoint = "main";
        psDesc.type = ShaderType::Pixel;
        psDesc.enableDebug = true;

        auto pixelShader = m_shaderManager->loadShader(psDesc);
        if (!pixelShader)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load pixel shader"));
        }

        return {};
    }

    Utils::VoidResult CubeRenderer::createPipelineState()
    {
        // 縺ｾ縺壹す繧ｧ繝ｼ繝繝ｼ繧偵Ο繝ｼ繝会ｼ医く繝｣繝・す繝･縺ｫ蟄伜惠縺励↑縺・ｴ蜷医・縺溘ａ・・
        ShaderCompileDesc vsDesc;
        vsDesc.filePath = "assets/shaders/BasicVertex.hlsl";
        vsDesc.entryPoint = "main";
        vsDesc.type = ShaderType::Vertex;
        vsDesc.enableDebug = true;

        auto vertexShaderResult = m_shaderManager->loadShader(vsDesc);
        if (!vertexShaderResult)
        {
            Utils::log_warning("Failed to load vertex shader for CubeRenderer");
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load vertex shader"));
        }

        ShaderCompileDesc psDesc;
        psDesc.filePath = "assets/shaders/BasicPixel.hlsl";
        psDesc.entryPoint = "main";
        psDesc.type = ShaderType::Pixel;
        psDesc.enableDebug = true;

        auto pixelShaderResult = m_shaderManager->loadShader(psDesc);
        if (!pixelShaderResult)
        {
            Utils::log_warning("Failed to load pixel shader for CubeRenderer");
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, "Failed to load pixel shader"));
        }

        // 繝ｭ繝ｼ繝峨＆繧後◆繧ｷ繧ｧ繝ｼ繝繝ｼ繧剃ｽｿ逕ｨ・・oadShader縺ｮ謌ｻ繧雁､繧堤峩謗･菴ｿ逕ｨ・・
        auto vertexShader = vertexShaderResult;
        auto pixelShader = pixelShaderResult;

        CHECK_CONDITION(vertexShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Vertex shader is null");
        CHECK_CONDITION(pixelShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Pixel shader is null");

        // 蜈･蜉帙Ξ繧､繧｢繧ｦ繝医ｒ螳夂ｾｩ
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // 繝代う繝励Λ繧､繝ｳ繧ｹ繝・・繝医・險ｭ螳・
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->getBytecode(), vertexShader->getBytecodeSize() };
        psoDesc.PS = { pixelShader->getBytecode(), pixelShader->getBytecodeSize() };

        // 霆ｽ繧翫・險ｭ螳壹・蜷後§...
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

        // 繝悶Ξ繝ｳ繝峨せ繝・・繝・
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

        // 豺ｱ蠎ｦ繧ｹ繝・Φ繧ｷ繝ｫ繧ｹ繝・・繝・
        psoDesc.DepthStencilState.DepthEnable = TRUE;
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
        psoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;

        const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp = {
            D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
        };
        psoDesc.DepthStencilState.FrontFace = defaultStencilOp;
        psoDesc.DepthStencilState.BackFace = defaultStencilOp;

        // 縺昴・莉悶・險ｭ螳・
        psoDesc.SampleMask = UINT_MAX;
        psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
        psoDesc.SampleDesc.Count = 1;

        CHECK_HR(m_device->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)),
            Utils::ErrorType::ResourceCreation, "Failed to create graphics pipeline state");

        return {};
    }

    Utils::VoidResult CubeRenderer::createVertexBuffer()
    {
        const UINT vertexBufferSize = sizeof(m_cubeVertices);

        // 鬆らせ繝舌ャ繝輔ぃ逕ｨ縺ｮ繝偵・繝励・繝ｭ繝代ユ繧｣・医い繝・・繝ｭ繝ｼ繝峨ヲ繝ｼ繝暦ｼ・
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // 繝ｪ繧ｽ繝ｼ繧ｹ險倩ｿｰ蟄・
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

        // 鬆らせ繝・・繧ｿ繧偵ヰ繝・ヵ繧｡縺ｫ繧ｳ繝斐・
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange{ 0, 0 };

        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map vertex buffer");

        memcpy(pVertexDataBegin, m_cubeVertices.data(), sizeof(m_cubeVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // 鬆らせ繝舌ャ繝輔ぃ繝薙Η繝ｼ繧定ｨｭ螳・
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return {};
    }

    Utils::VoidResult CubeRenderer::createIndexBuffer()
    {
        const UINT indexBufferSize = sizeof(m_cubeIndices);

        // 繧､繝ｳ繝・ャ繧ｯ繧ｹ繝舌ャ繝輔ぃ逕ｨ縺ｮ繝偵・繝励・繝ｭ繝代ユ繧｣
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // 繝ｪ繧ｽ繝ｼ繧ｹ險倩ｿｰ蟄・
        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = indexBufferSize;
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
            IID_PPV_ARGS(&m_indexBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create index buffer");

        // 繧､繝ｳ繝・ャ繧ｯ繧ｹ繝・・繧ｿ繧偵ヰ繝・ヵ繧｡縺ｫ繧ｳ繝斐・
        UINT8* pIndexDataBegin;
        D3D12_RANGE readRange{ 0, 0 };

        CHECK_HR(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map index buffer");

        memcpy(pIndexDataBegin, m_cubeIndices.data(), sizeof(m_cubeIndices));
        m_indexBuffer->Unmap(0, nullptr);

        // 繧､繝ｳ繝・ャ繧ｯ繧ｹ繝舌ャ繝輔ぃ繝薙Η繝ｼ繧定ｨｭ螳・
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;

        return {};
    }

    void CubeRenderer::setupCubeVertices()
    {
        // 遶区婿菴薙・24鬆らせ・亥推髱｢縺ｫ4鬆らせ縲∫焚縺ｪ繧玖牡繧定ｨｭ螳夲ｼ・
        m_cubeVertices = { {
                // 蜑埼擇・・+・・ 襍､
                {{ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 蟾ｦ荳・
                {{  0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 蜿ｳ荳・
                {{  0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 蜿ｳ荳・
                {{ -0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // 蟾ｦ荳・

                // 閭碁擇・・-・・ 邱・
                {{  0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 蟾ｦ荳・
                {{ -0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 蜿ｳ荳・
                {{ -0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 蜿ｳ荳・
                {{  0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // 蟾ｦ荳・

                // 蟾ｦ髱｢・・-・・ 髱・
                {{ -0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // 蟾ｦ荳・
                {{ -0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 蜿ｳ荳・
                {{ -0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // 蜿ｳ荳・
                {{ -0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // 蟾ｦ荳・

                // 蜿ｳ髱｢・・+・・ 鮟・
                {{  0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // 蟾ｦ荳・
                {{  0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // 蜿ｳ荳・
                {{  0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // 蜿ｳ荳・
                {{  0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // 蟾ｦ荳・

                // 荳企擇・・+・・ 繝槭ぞ繝ｳ繧ｿ
                {{ -0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}}, // 蟾ｦ荳・
                {{  0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}}, // 蜿ｳ荳・
                {{  0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 蜿ｳ荳・
                {{ -0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // 蟾ｦ荳・

                // 荳矩擇・・-・・ 繧ｷ繧｢繝ｳ
                {{ -0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 蟾ｦ荳・
                {{  0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // 蜿ｳ荳・
                {{  0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}}, // 蜿ｳ荳・
                {{ -0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}}  // 蟾ｦ荳・
            } };

        // 遶区婿菴薙・繧､繝ｳ繝・ャ繧ｯ繧ｹ・・6繧､繝ｳ繝・ャ繧ｯ繧ｹ・・
        m_cubeIndices = { {
                // 蜑埼擇
                0, 1, 2,  2, 3, 0,
                // 閭碁擇
                4, 5, 6,  6, 7, 4,
                // 蟾ｦ髱｢
                8, 9, 10,  10, 11, 8,
                // 蜿ｳ髱｢
                12, 13, 14,  14, 15, 12,
                // 荳企擇
                16, 17, 18,  18, 19, 16,
                // 荳矩擇
                20, 21, 22,  22, 23, 20
            } };
    }

    void CubeRenderer::updateWorldMatrix()
    {
        // 繧ｹ繧ｱ繝ｼ繝ｫ -> 蝗櫁ｻ｢ -> 遘ｻ蜍輔・鬆・〒陦悟・繧貞粋謌・
        Math::Matrix4 scaleMatrix = Math::Matrix4::scaling(m_scale);
        Math::Matrix4 rotationMatrix = Math::Matrix4::rotationX(Math::radians(m_rotation.x)) *
            Math::Matrix4::rotationY(Math::radians(m_rotation.y)) *
            Math::Matrix4::rotationZ(Math::radians(m_rotation.z));
        Math::Matrix4 translationMatrix = Math::Matrix4::translation(m_position);

        m_worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    }
}
