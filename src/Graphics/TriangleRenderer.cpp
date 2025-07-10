// src/Graphics/TriangleRenderer.cpp
#include "TriangleRenderer.hpp"
#include <format>

namespace Engine::Graphics
{
    Utils::VoidResult TriangleRenderer::initialize(Device* device)
    {
        m_device = device;
        Utils::log_info("Initializing Triangle Renderer...");

        // �萔�o�b�t�@�}�l�[�W���[��������
        auto constantBufferResult = m_constantBufferManager.initialize(device);
        if (!constantBufferResult) return constantBufferResult;

        // �O�p�`�̒��_�f�[�^��ݒ�
        setupTriangleVertices();

        // ���[���h�s���������
        updateWorldMatrix();

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

    void TriangleRenderer::render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex)
    {
        // �f�o�b�O���O��ǉ�
        static bool firstFrame = true;
        if (firstFrame) {
            // �J�����̍s��������O�o��
            Math::Matrix4 viewMatrix = camera.getViewMatrix();
            Math::Matrix4 projMatrix = camera.getProjectionMatrix();

            Utils::log_info("=== Camera Matrix Debug ===");
            Utils::log_info(std::format("View Matrix [0]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                viewMatrix.m[0][0], viewMatrix.m[0][1], viewMatrix.m[0][2], viewMatrix.m[0][3]));
            Utils::log_info(std::format("View Matrix [1]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                viewMatrix.m[1][0], viewMatrix.m[1][1], viewMatrix.m[1][2], viewMatrix.m[1][3]));
            Utils::log_info(std::format("View Matrix [2]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                viewMatrix.m[2][0], viewMatrix.m[2][1], viewMatrix.m[2][2], viewMatrix.m[2][3]));
            Utils::log_info(std::format("View Matrix [3]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                viewMatrix.m[3][0], viewMatrix.m[3][1], viewMatrix.m[3][2], viewMatrix.m[3][3]));

            Utils::log_info("=== Projection Matrix Debug ===");
            Utils::log_info(std::format("Proj Matrix [0]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                projMatrix.m[0][0], projMatrix.m[0][1], projMatrix.m[0][2], projMatrix.m[0][3]));
            Utils::log_info(std::format("Proj Matrix [1]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                projMatrix.m[1][0], projMatrix.m[1][1], projMatrix.m[1][2], projMatrix.m[1][3]));
            Utils::log_info(std::format("Proj Matrix [2]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                projMatrix.m[2][0], projMatrix.m[2][1], projMatrix.m[2][2], projMatrix.m[2][3]));
            Utils::log_info(std::format("Proj Matrix [3]: {:.3f}, {:.3f}, {:.3f}, {:.3f}",
                projMatrix.m[3][0], projMatrix.m[3][1], projMatrix.m[3][2], projMatrix.m[3][3]));

            // ���_�f�[�^�̊m�F
            Utils::log_info("=== Vertex Data Debug ===");
            for (size_t i = 0; i < m_triangleVertices.size(); ++i) {
                const auto& vertex = m_triangleVertices[i];
                Utils::log_info(std::format("Vertex {}: Pos({:.3f}, {:.3f}, {:.3f}), Color({:.3f}, {:.3f}, {:.3f})",
                    i, vertex.position[0], vertex.position[1], vertex.position[2],
                    vertex.color[0], vertex.color[1], vertex.color[2]));
            }

            firstFrame = false;
        }

        // �萔�o�b�t�@���X�V
        CameraConstants cameraConstants{};
        cameraConstants.viewMatrix = camera.getViewMatrix();
        cameraConstants.projectionMatrix = camera.getProjectionMatrix();
        cameraConstants.viewProjectionMatrix = camera.getViewProjectionMatrix();
        cameraConstants.cameraPosition = camera.getPosition();

        ObjectConstants objectConstants{};
        objectConstants.worldMatrix = m_worldMatrix;
        // �C��: �s��̊|���Z�����𐳂����ݒ� (VP * World)
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

        // ���_�o�b�t�@��ݒ�
        commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

        // �O�p�`��`��i3���_�A1�C���X�^���X�j
        commandList->DrawInstanced(3, 1, 0, 0);
    }

    Utils::VoidResult TriangleRenderer::createRootSignature()
    {
        // 2�̒萔�o�b�t�@�p���[�g�V�O�l�`��
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

    Utils::VoidResult TriangleRenderer::createShaders()
    {
        // �t�@�C�����璸�_�V�F�[�_�[���R���p�C��
        auto vsResult = m_shaderManager.compileFromFile(
            L"shaders/BasicVertex.hlsl",
            "main",
            ShaderType::Vertex
        );
        if (!vsResult)
        {
            return std::unexpected(vsResult.error());
        }

        // �t�@�C������s�N�Z���V�F�[�_�[���R���p�C��
        auto psResult = m_shaderManager.compileFromFile(
            L"shaders/BasicPixel.hlsl",
            "main",
            ShaderType::Pixel
        );
        if (!psResult)
        {
            return std::unexpected(psResult.error());
        }

        // �V�F�[�_�[��o�^
        m_shaderManager.registerShader("basic_vertex", vsResult.value());
        m_shaderManager.registerShader("basic_pixel", psResult.value());

        return {};
    }

    Utils::VoidResult TriangleRenderer::createPipelineState()
    {
        // �V�F�[�_�[���擾
        const ShaderInfo* vertexShader = m_shaderManager.getShader("basic_vertex");
        const ShaderInfo* pixelShader = m_shaderManager.getShader("basic_pixel");

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

        //�[�x�X�e�C�V���X�e�[�g
        psoDesc.DepthStencilState.DepthEnable = TRUE;    //�[�x�e�X�g�L��
        psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;//�[�x�������ݗL��
        psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        psoDesc.DepthStencilState.StencilEnable = FALSE;
        psoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
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
        psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; //�[�x�o�b�t�@�t�H�[�}�b�g�w��
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
        // �f�o�b�O�p�F�����߂̎O�p�`
        m_triangleVertices = { {
            { { 0.0f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f} },  // ��F��
            { { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f} },  // �E���F��
            { {-0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 1.0f} }   // �����F��
        } };
    }

    void TriangleRenderer::updateWorldMatrix()
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