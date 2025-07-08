// src/Graphics/TriangleRenderer.cpp
#include "TriangleRenderer.hpp"
#include <format>

namespace Engine::Graphics
{
    Utils::VoidResult TriangleRenderer::initialize(Device* device)
    {
        m_device = device;
        Utils::log_info("Initializing Triangle Renderer...");

        // �O�p�`�̒��_�f�[�^��ݒ�
        setupTriangleVertices();

        // �e�R���|�[�l���g�����Ԃɏ�����
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
        // ���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g��ݒ�
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        // �v���~�e�B�u�g�|���W��ݒ�i�O�p�`���X�g�j
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ���_�o�b�t�@��ݒ�
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // �O�p�`��`��i3���_�A1�C���X�^���X�j
        commandList->DrawInstanced(3, 1, 0, 0);
    }

    Utils::VoidResult TriangleRenderer::createRootSignature()
    {
        // ��̃��[�g�V�O�l�`�����쐬�i����͒萔�o�b�t�@��e�N�X�`�����g��Ȃ��j
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
        // ���_�V�F�[�_�[���R���p�C��
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

        // �s�N�Z���V�F�[�_�[���R���p�C��
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

        // �V�F�[�_�[��o�^
        m_shaderManager.registerShader("vs_color", vsResult.value());
        m_shaderManager.registerShader("ps_color", psResult.value());

        return {};
    }

    Utils::VoidResult TriangleRenderer::createPipelineState()
    {
        // �V�F�[�_�[���擾
        const ShaderInfo* vertexShader = m_shaderManager.getShader("vs_color");
        const ShaderInfo* pixelShader = m_shaderManager.getShader("ps_color");

        CHECK_CONDITION(vertexShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Vertex shader not found");
        CHECK_CONDITION(pixelShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Pixel shader not found");

        // ���̓��C�A�E�g���`
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // �p�C�v���C���X�e�[�g�̐ݒ�
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->blob->GetBufferPointer(), vertexShader->blob->GetBufferSize() };
        psoDesc.PS = { pixelShader->blob->GetBufferPointer(), pixelShader->blob->GetBufferSize() };

        // ���X�^���C�U�[�X�e�[�g
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

        // �u�����h�X�e�[�g
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

        // ���̑��̐ݒ�
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

        // ���_�o�b�t�@�p�̃q�[�v�v���p�e�B�i�A�b�v���[�h�q�[�v�j
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // ���\�[�X�L�q�q
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

        // ���_�f�[�^���o�b�t�@�ɃR�s�[
        UINT8* pVertexDataBegin;
        D3D12_RANGE readRange{ 0, 0 }; // CPU����ǂݎ��Ȃ��̂Ŕ͈͂�0

        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map vertex buffer");

        memcpy(pVertexDataBegin, m_triangleVertices.data(), sizeof(m_triangleVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // ���_�o�b�t�@�r���[��ݒ�
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return {};
    }

    void TriangleRenderer::setupTriangleVertices()
    {
        // �J���t���ȎO�p�`�̒��_�f�[�^
        m_triangleVertices = { {
            { { 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} },  // ��F��
            { { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} },  // �E���F��
            { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} }   // �����F��
        } };
    }
}