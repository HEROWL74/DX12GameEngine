// src/Graphics/CubeRenderer.cpp
#include "CubeRenderer.hpp"
#include <format>

namespace Engine::Graphics
{
    Utils::VoidResult CubeRenderer::initialize(Device* device, ShaderManager* shaderManager)
    {
        // �f�o�b�O�p: null�`�F�b�N
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        m_shaderManager = shaderManager;
        Utils::log_info("Initializing Cube Renderer...");

        // �萔�o�b�t�@�}�l�[�W���[��������
        auto constantBufferResult = m_constantBufferManager.initialize(device);
        if (!constantBufferResult) {
            Utils::log_error(constantBufferResult.error());
            return constantBufferResult;
        }

        // �����̂̒��_�f�[�^��ݒ�
        setupCubeVertices();

        // ���[���h�s���������
        updateWorldMatrix();

        // �e�R���|�[�l���g�����Ԃɏ�����
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
        // �f�o�b�O�p: null�`�F�b�N
        if (!commandList) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "CommandList is null"));
            return;
        }

        if (!m_constantBufferManager.isValid()) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "ConstantBufferManager is not valid"));
            return;
        }

        if (!m_rootSignature) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "RootSignature is not initialized"));
            return;
        }

        if (!m_pipelineState) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "PipelineState is not initialized"));
            return;
        }
        // �萔�o�b�t�@���X�V
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

        // ���[�g�V�O�l�`���ƃp�C�v���C���X�e�[�g��ݒ�
        commandList->SetGraphicsRootSignature(m_rootSignature.Get());
        commandList->SetPipelineState(m_pipelineState.Get());

        // �萔�o�b�t�@��ݒ�
        commandList->SetGraphicsRootConstantBufferView(0, m_constantBufferManager.getCameraConstantsGPUAddress(frameIndex));
        commandList->SetGraphicsRootConstantBufferView(1, m_constantBufferManager.getObjectConstantsGPUAddress(frameIndex));

        // �v���~�e�B�u�g�|���W��ݒ�i�O�p�`���X�g�j
        commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        // ���_�o�b�t�@�ƃC���f�b�N�X�o�b�t�@��ݒ�
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
        commandList->IASetIndexBuffer(&m_indexBufferView);

        // �����̂�`��i36�C���f�b�N�X�A1�C���X�^���X�j
        commandList->DrawIndexedInstanced(36, 1, 0, 0, 0);
    }

    Utils::VoidResult CubeRenderer::createRootSignature()
    {
        // 2�̒萔�o�b�t�@�p���[�g�V�O�l�`���iTriangleRenderer�Ɠ����j
        D3D12_ROOT_PARAMETER rootParameters[2];

        // �J�����萔�o�b�t�@
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[0].Descriptor.ShaderRegister = 0;
        rootParameters[0].Descriptor.RegisterSpace = 0;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        // �I�u�W�F�N�g�萔�o�b�t�@
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[1].Descriptor.ShaderRegister = 1;
        rootParameters[1].Descriptor.RegisterSpace = 0;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.NumParameters = _countof(rootParameters);
        rootSignatureDesc.pParameters = rootParameters;
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

    Utils::VoidResult CubeRenderer::createShaders()
    {
        // ShaderManager ��������
        //auto initResult = m_shaderManager->initialize(m_device);
        //if (!initResult) return initResult;
        

        // ShaderCompileDesc ���g�p���ăV�F�[�_�[�����[�h
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
        // �܂��V�F�[�_�[�����[�h�i�L���b�V���ɑ��݂��Ȃ��ꍇ�̂��߁j
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

        // ���[�h���ꂽ�V�F�[�_�[���g�p�iloadShader�̖߂�l�𒼐ڎg�p�j
        auto vertexShader = vertexShaderResult;
        auto pixelShader = pixelShaderResult;

        CHECK_CONDITION(vertexShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Vertex shader is null");
        CHECK_CONDITION(pixelShader != nullptr, Utils::ErrorType::ShaderCompilation,
            "Pixel shader is null");

        // ���̓��C�A�E�g���`
        D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // �p�C�v���C���X�e�[�g�̐ݒ�
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
        psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
        psoDesc.pRootSignature = m_rootSignature.Get();
        psoDesc.VS = { vertexShader->getBytecode(), vertexShader->getBytecodeSize() };
        psoDesc.PS = { pixelShader->getBytecode(), pixelShader->getBytecodeSize() };

        // �c��̐ݒ�͓���...
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

        // �[�x�X�e���V���X�e�[�g
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

        // ���̑��̐ݒ�
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
        D3D12_RANGE readRange{ 0, 0 };

        CHECK_HR(m_vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map vertex buffer");

        memcpy(pVertexDataBegin, m_cubeVertices.data(), sizeof(m_cubeVertices));
        m_vertexBuffer->Unmap(0, nullptr);

        // ���_�o�b�t�@�r���[��ݒ�
        m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
        m_vertexBufferView.StrideInBytes = sizeof(Vertex);
        m_vertexBufferView.SizeInBytes = vertexBufferSize;

        return {};
    }

    Utils::VoidResult CubeRenderer::createIndexBuffer()
    {
        const UINT indexBufferSize = sizeof(m_cubeIndices);

        // �C���f�b�N�X�o�b�t�@�p�̃q�[�v�v���p�e�B
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

        // �C���f�b�N�X�f�[�^���o�b�t�@�ɃR�s�[
        UINT8* pIndexDataBegin;
        D3D12_RANGE readRange{ 0, 0 };

        CHECK_HR(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)),
            Utils::ErrorType::ResourceCreation, "Failed to map index buffer");

        memcpy(pIndexDataBegin, m_cubeIndices.data(), sizeof(m_cubeIndices));
        m_indexBuffer->Unmap(0, nullptr);

        // �C���f�b�N�X�o�b�t�@�r���[��ݒ�
        m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
        m_indexBufferView.Format = DXGI_FORMAT_R16_UINT;
        m_indexBufferView.SizeInBytes = indexBufferSize;

        return {};
    }

    void CubeRenderer::setupCubeVertices()
    {
        // �����̂�24���_�i�e�ʂ�4���_�A�قȂ�F��ݒ�j
        m_cubeVertices = { {
                // �O�ʁiZ+�j- ��
                {{ -0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // ����
                {{  0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // �E��
                {{  0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // �E��
                {{ -0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}}, // ����

                // �w�ʁiZ-�j- ��
                {{  0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // ����
                {{ -0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // �E��
                {{ -0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // �E��
                {{  0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}}, // ����

                // ���ʁiX-�j- ��
                {{ -0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // ����
                {{ -0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // �E��
                {{ -0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}}, // �E��
                {{ -0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}}, // ����

                // �E�ʁiX+�j- ��
                {{  0.5f, -0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // ����
                {{  0.5f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // �E��
                {{  0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}}, // �E��
                {{  0.5f,  0.5f,  0.5f}, {1.0f, 1.0f, 0.0f}}, // ����

                // ��ʁiY+�j- �}�[���^
                {{ -0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}}, // ����
                {{  0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 1.0f}}, // �E��
                {{  0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // �E��
                {{ -0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 1.0f}}, // ����

                // ���ʁiY-�j- �V�A��
                {{ -0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // ����
                {{  0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 1.0f}}, // �E��
                {{  0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}}, // �E��
                {{ -0.5f, -0.5f,  0.5f}, {0.0f, 1.0f, 1.0f}}  // ����
            } };

        // �����̂̃C���f�b�N�X�i36�C���f�b�N�X�j
        m_cubeIndices = { {
                // �O��
                0, 1, 2,  2, 3, 0,
                // �w��
                4, 5, 6,  6, 7, 4,
                // ����
                8, 9, 10,  10, 11, 8,
                // �E��
                12, 13, 14,  14, 15, 12,
                // ���
                16, 17, 18,  18, 19, 16,
                // ����
                20, 21, 22,  22, 23, 20
            } };
    }

    void CubeRenderer::updateWorldMatrix()
    {
        // �X�P�[�� -> ��] -> �ړ��̏��ōs�������
        Math::Matrix4 scaleMatrix = Math::Matrix4::scaling(m_scale);
        Math::Matrix4 rotationMatrix = Math::Matrix4::rotationX(Math::radians(m_rotation.x)) *
            Math::Matrix4::rotationY(Math::radians(m_rotation.y)) *
            Math::Matrix4::rotationZ(Math::radians(m_rotation.z));
        Math::Matrix4 translationMatrix = Math::Matrix4::translation(m_position);

        m_worldMatrix = translationMatrix * rotationMatrix * scaleMatrix;
    }
}