//src/Graphics/ShaderManager.cpp
#include "ShaderManager.hpp"
#include <format>
#include <fstream>
#include <filesystem>
#include <sstream>

namespace Engine::Graphics
{
    //=========================================================================
    // Shader����
    //=========================================================================

    Utils::Result<std::shared_ptr<Shader>> Shader::compileFromFile(const ShaderCompileDesc& desc)
    {
        // �t�@�C���ǂݍ���
        auto codeResult = readShaderFile(desc.filePath);
        if (!codeResult)
        {
            return std::unexpected(codeResult.error());
        }

        // �C���N���[�h����
        std::string baseDir = std::filesystem::path(desc.filePath).parent_path().string();
        std::string processedCode = processIncludes(*codeResult, baseDir);

        auto shader = std::make_shared<Shader>();
        auto initResult = shader->initialize(processedCode, desc.entryPoint, desc.type, desc.macros, desc.enableDebug, desc.filePath);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        return shader;
    }

    Utils::Result<std::shared_ptr<Shader>> Shader::compileFromString(
        const std::string& shaderCode,
        const std::string& entryPoint,
        ShaderType type,
        const std::vector<ShaderMacro>& macros,
        bool enableDebug)
    {
        auto shader = std::make_shared<Shader>();
        auto initResult = shader->initialize(shaderCode, entryPoint, type, macros, enableDebug);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        return shader;
    }

    Utils::VoidResult Shader::initialize(
        const std::string& shaderCode,
        const std::string& entryPoint,
        ShaderType type,
        const std::vector<ShaderMacro>& macros,
        bool enableDebug,
        const std::string& filePath)
    {
        m_type = type;
        m_entryPoint = entryPoint;
        m_filePath = filePath;

        // �R���p�C���t���O
        UINT compileFlags = 0;
        if (enableDebug)
        {
            compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        }
        else
        {
            compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
        }

        // �}�N���ϊ�
        auto d3dMacros = convertMacros(macros);

        // �V�F�[�_�[�^�[�Q�b�g
        std::string target = shaderTypeToTarget(type);

        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompile(
            shaderCode.c_str(),
            shaderCode.size(),
            filePath.empty() ? nullptr : filePath.c_str(),
            d3dMacros.empty() ? nullptr : d3dMacros.data(),
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            entryPoint.c_str(),
            target.c_str(),
            compileFlags,
            0,
            &m_bytecode,
            &errorBlob
        );

        if (FAILED(hr))
        {
            std::string errorMsg = "Shader compilation failed";
            if (errorBlob)
            {
                errorMsg += std::format(": {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }

            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, errorMsg, hr));
        }

        return {};
    }

    std::string Shader::shaderTypeToTarget(ShaderType type)
    {
        switch (type)
        {
        case ShaderType::Vertex:   return "vs_5_1";
        case ShaderType::Pixel:    return "ps_5_1";
        case ShaderType::Geometry: return "gs_5_1";
        case ShaderType::Hull:     return "hs_5_1";
        case ShaderType::Domain:   return "ds_5_1";
        case ShaderType::Compute:  return "cs_5_1";
        default:                   return "vs_5_1";
        }
    }

    std::vector<D3D_SHADER_MACRO> Shader::convertMacros(const std::vector<ShaderMacro>& macros)
    {
        std::vector<D3D_SHADER_MACRO> d3dMacros;
        d3dMacros.reserve(macros.size() + 1);

        for (const auto& macro : macros)
        {
            D3D_SHADER_MACRO d3dMacro;
            d3dMacro.Name = macro.name.c_str();
            d3dMacro.Definition = macro.definition.c_str();
            d3dMacros.push_back(d3dMacro);
        }

        // �I�[
        D3D_SHADER_MACRO endMacro = { nullptr, nullptr };
        d3dMacros.push_back(endMacro);

        return d3dMacros;
    }

    //=========================================================================
    // PipelineState����
    //=========================================================================

    Utils::Result<std::shared_ptr<PipelineState>> PipelineState::create(
        Device* device,
        const PipelineStateDesc& desc)
    {
        auto pipelineState = std::make_shared<PipelineState>();
        auto initResult = pipelineState->initialize(device, desc);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        return pipelineState;
    }

    void PipelineState::setDebugName(const std::string& name)
    {
        if (m_pipelineState)
        {
            std::wstring wname(name.begin(), name.end());
            m_pipelineState->SetName(wname.c_str());
        }
        if (m_rootSignature)
        {
            std::wstring wname(name.begin(), name.end());
            wname += L"_RootSignature";
            m_rootSignature->SetName(wname.c_str());
        }
    }

    Utils::VoidResult PipelineState::initialize(Device* device, const PipelineStateDesc& desc)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        m_desc = desc;

        auto rootSigResult = createRootSignature();
        if (!rootSigResult)
        {
            return rootSigResult;
        }

        auto pipelineResult = createPipelineState();
        if (!pipelineResult)
        {
            return pipelineResult;
        }

        if (!m_desc.debugName.empty())
        {
            setDebugName(m_desc.debugName);
        }

        return {};
    }

    Utils::VoidResult PipelineState::createRootSignature()
    {
        std::vector<D3D12_ROOT_PARAMETER1> rootParams;
        rootParams.reserve(m_desc.rootParameters.size());

        for (const auto& param : m_desc.rootParameters)
        {
            D3D12_ROOT_PARAMETER1 rootParam = {};
            rootParam.ShaderVisibility = param.visibility;

            switch (param.type)
            {
            case RootParameterDesc::ConstantBufferView:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
                rootParam.Descriptor.ShaderRegister = param.shaderRegister;
                rootParam.Descriptor.RegisterSpace = param.registerSpace;
                break;

            case RootParameterDesc::ShaderResourceView:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
                rootParam.Descriptor.ShaderRegister = param.shaderRegister;
                rootParam.Descriptor.RegisterSpace = param.registerSpace;
                break;

            case RootParameterDesc::UnorderedAccessView:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
                rootParam.Descriptor.ShaderRegister = param.shaderRegister;
                rootParam.Descriptor.RegisterSpace = param.registerSpace;
                break;

            case RootParameterDesc::Constants:
                rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
                rootParam.Constants.ShaderRegister = param.shaderRegister;
                rootParam.Constants.RegisterSpace = param.registerSpace;
                rootParam.Constants.Num32BitValues = param.numConstants;
                break;

            default:
                return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unsupported root parameter type"));
            }

            rootParams.push_back(rootParam);
        }

        // �X�^�e�B�b�N�T���v���[
        std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;
        staticSamplers.reserve(m_desc.staticSamplers.size());

        for (const auto& sampler : m_desc.staticSamplers)
        {
            D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = sampler.filter;
            samplerDesc.AddressU = sampler.addressModeU;
            samplerDesc.AddressV = sampler.addressModeV;
            samplerDesc.AddressW = sampler.addressModeW;
            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = 1;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
            samplerDesc.MinLOD = 0.0f;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerDesc.ShaderRegister = sampler.shaderRegister;
            samplerDesc.RegisterSpace = sampler.registerSpace;
            samplerDesc.ShaderVisibility = sampler.visibility;

            staticSamplers.push_back(samplerDesc);
        }

        // ���[�g�V�O�l�`���L�q
        D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
        rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParams.size());
        rootSigDesc.Desc_1_1.pParameters = rootParams.empty() ? nullptr : rootParams.data();
        rootSigDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
        rootSigDesc.Desc_1_1.pStaticSamplers = staticSamplers.empty() ? nullptr : staticSamplers.data();
        rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> serializedRootSig;
        ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &serializedRootSig, &errorBlob);
        if (FAILED(hr))
        {
            std::string errorMsg = "Failed to serialize root signature";
            if (errorBlob)
            {
                errorMsg += std::format(": {}", static_cast<const char*>(errorBlob->GetBufferPointer()));
            }
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation, errorMsg, hr));
        }

        CHECK_HR(m_device->getDevice()->CreateRootSignature(
            0,
            serializedRootSig->GetBufferPointer(),
            serializedRootSig->GetBufferSize(),
            IID_PPV_ARGS(&m_rootSignature)),
            Utils::ErrorType::ResourceCreation,
            "Failed to create root signature");

        return {};
    }

    Utils::VoidResult PipelineState::createPipelineState()
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

        // �V�F�[�_�[�ݒ�
        if (m_desc.vertexShader && m_desc.vertexShader->isValid())
        {
            psoDesc.VS.pShaderBytecode = m_desc.vertexShader->getBytecode();
            psoDesc.VS.BytecodeLength = m_desc.vertexShader->getBytecodeSize();
        }

        if (m_desc.pixelShader && m_desc.pixelShader->isValid())
        {
            psoDesc.PS.pShaderBytecode = m_desc.pixelShader->getBytecode();
            psoDesc.PS.BytecodeLength = m_desc.pixelShader->getBytecodeSize();
        }

        if (m_desc.geometryShader && m_desc.geometryShader->isValid())
        {
            psoDesc.GS.pShaderBytecode = m_desc.geometryShader->getBytecode();
            psoDesc.GS.BytecodeLength = m_desc.geometryShader->getBytecodeSize();
        }

        // ���[�g�V�O�l�`��
        psoDesc.pRootSignature = m_rootSignature.Get();

        // ���̓��C�A�E�g
        psoDesc.InputLayout.pInputElementDescs = m_desc.inputLayout.empty() ? nullptr : m_desc.inputLayout.data();
        psoDesc.InputLayout.NumElements = static_cast<UINT>(m_desc.inputLayout.size());

        // �v���~�e�B�u�g�|���W�[
        psoDesc.PrimitiveTopologyType = m_desc.primitiveTopology;

        // �����_�[�^�[�Q�b�g
        psoDesc.NumRenderTargets = static_cast<UINT>(m_desc.rtvFormats.size());
        for (size_t i = 0; i < m_desc.rtvFormats.size() && i < 8; ++i)
        {
            psoDesc.RTVFormats[i] = m_desc.rtvFormats[i];
        }
        psoDesc.DSVFormat = m_desc.dsvFormat;

        // �T���v���ݒ�
        psoDesc.SampleDesc.Count = 1;
        psoDesc.SampleDesc.Quality = 0;
        psoDesc.SampleMask = UINT_MAX;

        // �u�����h�X�e�[�g
        psoDesc.BlendState.RenderTarget[0].BlendEnable = m_desc.enableBlending;
        psoDesc.BlendState.RenderTarget[0].SrcBlend = m_desc.srcBlend;
        psoDesc.BlendState.RenderTarget[0].DestBlend = m_desc.destBlend;
        psoDesc.BlendState.RenderTarget[0].BlendOp = m_desc.blendOp;
        psoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        psoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        psoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        // ���X�^���C�U�[�X�e�[�g
        psoDesc.RasterizerState.FillMode = m_desc.fillMode;
        psoDesc.RasterizerState.CullMode = m_desc.cullMode;
        psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
        psoDesc.RasterizerState.DepthBias = 0;
        psoDesc.RasterizerState.DepthBiasClamp = 0.0f;
        psoDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;
        psoDesc.RasterizerState.DepthClipEnable = m_desc.enableDepthClip;
        psoDesc.RasterizerState.MultisampleEnable = FALSE;
        psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
        psoDesc.RasterizerState.ForcedSampleCount = 0;
        psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

        // �[�x�X�e���V���X�e�[�g
        psoDesc.DepthStencilState.DepthEnable = m_desc.enableDepthTest;
        psoDesc.DepthStencilState.DepthWriteMask = m_desc.enableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
        psoDesc.DepthStencilState.DepthFunc = m_desc.depthFunc;
        psoDesc.DepthStencilState.StencilEnable = FALSE;

        CHECK_HR(m_device->getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)),
            Utils::ErrorType::ResourceCreation,
            std::format("Failed to create graphics pipeline state: {}", m_desc.debugName));

        return {};
    }

    //=========================================================================
    // ShaderManager����
    //=========================================================================

    Utils::VoidResult ShaderManager::initialize(Device* device)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        auto defaultShadersResult = createDefaultShaders();
        if (!defaultShadersResult)
        {
            return defaultShadersResult;
        }

        auto defaultPipelinesResult = createDefaultPipelines();
        if (!defaultPipelinesResult)
        {
            return defaultPipelinesResult;
        }

        m_initialized = true;
        Utils::log_info("ShaderManager initialized successfully");
        return {};
    }

    std::shared_ptr<Shader> ShaderManager::loadShader(const ShaderCompileDesc& desc)
    {
        if (!m_initialized)
        {
            Utils::log_warning("ShaderManager not initialized");
            return nullptr;
        }

        std::string key = generateShaderKey(desc);
        if (hasShader(key))
        {
            return getShader(key);
        }

        auto shaderResult = Shader::compileFromFile(desc);
        if (!shaderResult)
        {
            Utils::log_warning(std::format("Failed to compile shader '{}': {}", desc.filePath, shaderResult.error().message));
            return nullptr;
        }

        m_shaders[key] = *shaderResult;
        Utils::log_info(std::format("Shader compiled: {}", desc.filePath));
        return *shaderResult;
    }

    std::shared_ptr<Shader> ShaderManager::getShader(const std::string& name) const
    {
        auto it = m_shaders.find(name);
        return (it != m_shaders.end()) ? it->second : nullptr;
    }

    bool ShaderManager::hasShader(const std::string& name) const
    {
        return m_shaders.find(name) != m_shaders.end();
    }

    void ShaderManager::removeShader(const std::string& name)
    {
        auto it = m_shaders.find(name);
        if (it != m_shaders.end())
        {
            m_shaders.erase(it);
        }
    }

    std::shared_ptr<PipelineState> ShaderManager::createPipelineState(
        const std::string& name,
        const PipelineStateDesc& desc)
    {
        if (!m_initialized)
        {
            Utils::log_warning("ShaderManager not initialized");
            return nullptr;
        }

        if (hasPipelineState(name))
        {
            Utils::log_warning(std::format("Pipeline state '{}' already exists", name));
            return getPipelineState(name);
        }

        auto pipelineResult = PipelineState::create(m_device, desc);
        if (!pipelineResult)
        {
            Utils::log_warning(std::format("Failed to create pipeline state '{}': {}", name, pipelineResult.error().message));
            return nullptr;
        }

        m_pipelineStates[name] = *pipelineResult;
        Utils::log_info(std::format("Pipeline state created: {}", name));
        return *pipelineResult;
    }

    std::shared_ptr<PipelineState> ShaderManager::getPipelineState(const std::string& name) const
    {
        auto it = m_pipelineStates.find(name);
        return (it != m_pipelineStates.end()) ? it->second : nullptr;
    }

    bool ShaderManager::hasPipelineState(const std::string& name) const
    {
        return m_pipelineStates.find(name) != m_pipelineStates.end();
    }

    void ShaderManager::removePipelineState(const std::string& name)
    {
        auto it = m_pipelineStates.find(name);
        if (it != m_pipelineStates.end())
        {
            m_pipelineStates.erase(it);
        }
    }

    Utils::VoidResult ShaderManager::createDefaultShaders()
    {
        // �f�t�H���gPBR���_�V�F�[�_�[
        std::string pbrVertexShader = R"(
struct VertexInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
};

struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

cbuffer SceneConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float3 cameraPos;
    float padding1;
};

cbuffer ObjectConstants : register(b1)
{
    float4x4 worldMatrix;
    float4x4 normalMatrix;
};

VertexOutput main(VertexInput input)
{
    VertexOutput output;
    
    float4 worldPos = mul(float4(input.position, 1.0), worldMatrix);
    output.worldPos = worldPos.xyz;
    output.position = mul(mul(worldPos, viewMatrix), projMatrix);
    
    output.normal = normalize(mul(input.normal, (float3x3)normalMatrix));
    output.tangent = normalize(mul(input.tangent, (float3x3)normalMatrix));
    output.bitangent = cross(output.normal, output.tangent);
    
    output.uv = input.uv;
    
    return output;
}
)";

        // �f�t�H���gPBR�s�N�Z���V�F�[�_�[
        std::string pbrPixelShader = R"(
struct VertexOutput
{
    float4 position : SV_POSITION;
    float3 worldPos : WORLDPOS;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
};

cbuffer MaterialConstants : register(b2)
{
    float4 albedo;
    float4 roughnessAoEmissiveStrength;
    float4 emissive;
    float4 alphaParams;
    float4 uvTransform;
};

cbuffer SceneConstants : register(b0)
{
    float4x4 viewMatrix;
    float4x4 projMatrix;
    float3 cameraPos;
    float padding1;
};

Texture2D albedoTexture : register(t0);
Texture2D normalTexture : register(t1);
Texture2D metallicTexture : register(t2);
Texture2D roughnessTexture : register(t3);
SamplerState linearSampler : register(s0);

float4 main(VertexOutput input) : SV_TARGET
{
    float2 uv = input.uv * uvTransform.xy + uvTransform.zw;
    
    // �A���x�h
    float3 baseColor = albedo.rgb * albedoTexture.Sample(linearSampler, uv).rgb;
    
    // �@���}�b�s���O
    float3 normalMap = normalTexture.Sample(linearSampler, uv).rgb * 2.0 - 1.0;
    float3x3 TBN = float3x3(input.tangent, input.bitangent, input.normal);
    float3 normal = normalize(mul(normalMap, TBN));
    
    // PBR�p�����[�^
    float metallic = albedo.a * metallicTexture.Sample(linearSampler, uv).r;
    float roughness = roughnessAoEmissiveStrength.x * roughnessTexture.Sample(linearSampler, uv).r;
    
    // �V���v���ȃ��C�e�B���O�i���݂͊����̂݁j
    float3 viewDir = normalize(cameraPos - input.worldPos);
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    
    float NdotL = saturate(dot(normal, lightDir));
    float3 diffuse = baseColor * NdotL * 0.8;
    float3 ambient = baseColor * 0.2;
    
    // ����
    float3 emission = emissive.rgb * roughnessAoEmissiveStrength.z;
    
    float3 finalColor = diffuse + ambient + emission;
    
    return float4(finalColor, alphaParams.x);
}
)";

        // �V�F�[�_�[���R���p�C��
        auto vsResult = Shader::compileFromString(pbrVertexShader, "main", ShaderType::Vertex);
        if (!vsResult)
        {
            return std::unexpected(vsResult.error());
        }

        auto psResult = Shader::compileFromString(pbrPixelShader, "main", ShaderType::Pixel);
        if (!psResult)
        {
            return std::unexpected(psResult.error());
        }

        m_shaders["DefaultPBR_VS"] = *vsResult;
        m_shaders["DefaultPBR_PS"] = *psResult;

        return {};
    }

    Utils::VoidResult ShaderManager::createDefaultPipelines()
    {
        // PBR�p�C�v���C���쐬
        PipelineStateDesc pbrDesc;
        pbrDesc.vertexShader = getShader("DefaultPBR_VS");
        pbrDesc.pixelShader = getShader("DefaultPBR_PS");
        pbrDesc.inputLayout = StandardInputLayouts::PBRVertex;
        pbrDesc.debugName = "DefaultPBR";

        // ���[�g�p�����[�^�ݒ�
        RootParameterDesc sceneConstants;
        sceneConstants.type = RootParameterDesc::ConstantBufferView;
        sceneConstants.shaderRegister = 0;
        sceneConstants.visibility = D3D12_SHADER_VISIBILITY_ALL;

        RootParameterDesc objectConstants;
        objectConstants.type = RootParameterDesc::ConstantBufferView;
        objectConstants.shaderRegister = 1;
        objectConstants.visibility = D3D12_SHADER_VISIBILITY_VERTEX;

        RootParameterDesc materialConstants;
        materialConstants.type = RootParameterDesc::ConstantBufferView;
        materialConstants.shaderRegister = 2;
        materialConstants.visibility = D3D12_SHADER_VISIBILITY_PIXEL;

        pbrDesc.rootParameters = { sceneConstants, objectConstants, materialConstants };

        // �X�^�e�B�b�N�T���v���[
        StaticSamplerDesc linearSampler;
        linearSampler.shaderRegister = 0;
        linearSampler.filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        linearSampler.addressModeU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        linearSampler.addressModeV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;

        pbrDesc.staticSamplers = { linearSampler };

        auto pbrPipelineResult = PipelineState::create(m_device, pbrDesc);
        if (!pbrPipelineResult)
        {
            return std::unexpected(pbrPipelineResult.error());
        }

        m_defaultPBRPipeline = *pbrPipelineResult;
        m_pipelineStates["DefaultPBR"] = m_defaultPBRPipeline;

        return {};
    }

    std::string ShaderManager::generateShaderKey(const ShaderCompileDesc& desc) const
    {
        std::string key = desc.filePath + "_" + desc.entryPoint + "_" + std::to_string(static_cast<int>(desc.type));

        for (const auto& macro : desc.macros)
        {
            key += "_" + macro.name + "=" + macro.definition;
        }

        if (desc.enableDebug)
        {
            key += "_DEBUG";
        }

        return key;
    }

    //=========================================================================
    // �W�����̓��C�A�E�g��`
    //=========================================================================

    namespace StandardInputLayouts
    {
        const std::vector<D3D12_INPUT_ELEMENT_DESC> Position =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionUV =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionNormalUV =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        const std::vector<D3D12_INPUT_ELEMENT_DESC> PBRVertex =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };
    }

    //=========================================================================
    // ���[�e�B���e�B�֐�����
    //=========================================================================

    Utils::Result<std::string> readShaderFile(const std::string& filePath)
    {
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Cannot open shader file: {}", filePath)));
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

    std::string processIncludes(const std::string& shaderCode, const std::string& baseDir)
    {
        // �ȒP�ȃC���N���[�h�����i#include "filename"�̌`���̂ݑΉ��j
        std::string result = shaderCode;
        std::string includePattern = "#include \"";

        size_t pos = 0;
        while ((pos = result.find(includePattern, pos)) != std::string::npos)
        {
            size_t startQuote = pos + includePattern.length();
            size_t endQuote = result.find("\"", startQuote);

            if (endQuote != std::string::npos)
            {
                std::string includeFile = result.substr(startQuote, endQuote - startQuote);
                std::string fullPath = baseDir.empty() ? includeFile : baseDir + "/" + includeFile;

                auto includeResult = readShaderFile(fullPath);
                if (includeResult)
                {
                    // �C���N���[�h�������ۂ̃t�@�C�����e�Œu��
                    size_t lineEnd = result.find("\n", pos);
                    if (lineEnd == std::string::npos) lineEnd = result.length();

                    result.replace(pos, lineEnd - pos, *includeResult);
                    pos += includeResult->length();
                }
                else
                {
                    Utils::log_warning(std::format("Failed to include file: {}", fullPath));
                    pos = endQuote + 1;
                }
            }
            else
            {
                pos += includePattern.length();
            }
        }

        return result;
    }
}