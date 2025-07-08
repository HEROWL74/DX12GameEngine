// src/Graphics/Shader.cpp
#include "Shader.hpp"
#include <format>
#include <fstream>
#include <sstream>

namespace Engine::Graphics
{
    Utils::Result<ShaderInfo> ShaderManager::compileFromString(
        const std::string& shaderCode,
        const std::string& entryPoint,
        ShaderType type,
        const std::string& shaderName)
    {
        Utils::log_info(std::format("Compiling shader: {} ({})", shaderName, entryPoint));

        ComPtr<ID3DBlob> shaderBlob;
        ComPtr<ID3DBlob> errorBlob;

        std::string target = getTargetProfile(type);
        UINT flags = getCompileFlags();

        HRESULT hr = D3DCompile(
            shaderCode.c_str(),         // �V�F�[�_�[�R�[�h
            shaderCode.size(),          // �R�[�h�T�C�Y
            shaderName.c_str(),         // �t�@�C�����i�f�o�b�O�p�j
            nullptr,                    // �}�N����`
            nullptr,                    // �C���N���[�h�n���h���[
            entryPoint.c_str(),         // �G���g���[�|�C���g
            target.c_str(),             // �^�[�Q�b�g�v���t�@�C��
            flags,                      // �R���p�C���t���O
            0,                          // �G�t�F�N�g�t���O
            &shaderBlob,                // �o�́F�R���p�C���ς݃V�F�[�_�[
            &errorBlob                  // �o�́F�G���[���b�Z�[�W
        );

        if (FAILED(hr))
        {
            std::string errorMsg = "Shader compilation failed";
            if (errorBlob)
            {
                errorMsg += std::format(": {}", static_cast<char*>(errorBlob->GetBufferPointer()));
            }
            return std::unexpected(Utils::make_error(Utils::ErrorType::ShaderCompilation, errorMsg, hr));
        }

        ShaderInfo info{};
        info.blob = shaderBlob;
        info.entryPoint = entryPoint;
        info.target = target;
        info.type = type;

        Utils::log_info(std::format("Shader compiled successfully: {} bytes", shaderBlob->GetBufferSize()));
        return info;
    }

    Utils::Result<ShaderInfo> ShaderManager::compileFromFile(
        const std::wstring& filePath,
        const std::string& entryPoint,
        ShaderType type)
    {
        // �t�@�C����ǂݍ���
        std::ifstream file(filePath);
        if (!file.is_open())
        {
            return std::unexpected(Utils::make_error(
                Utils::ErrorType::FileI0,
                std::format("Failed to open shader file: {}",
                    std::string(filePath.begin(), filePath.end()))
            ));
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        std::string shaderCode = buffer.str();

        // �t�@�C�������擾
        size_t lastSlash = filePath.find_last_of(L"/\\");
        std::string fileName = "Unknown";
        if (lastSlash != std::wstring::npos)
        {
            std::wstring fileNameW = filePath.substr(lastSlash + 1);
            fileName = std::string(fileNameW.begin(), fileNameW.end());
        }

        return compileFromString(shaderCode, entryPoint, type, fileName);
    }

    void ShaderManager::registerShader(const std::string& name, const ShaderInfo& shader)
    {
        m_shaderCache[name] = shader;
        Utils::log_info(std::format("Shader registered: {}", name));
    }

    const ShaderInfo* ShaderManager::getShader(const std::string& name) const
    {
        auto it = m_shaderCache.find(name);
        return (it != m_shaderCache.end()) ? &it->second : nullptr;
    }

    void ShaderManager::clear()
    {
        m_shaderCache.clear();
        Utils::log_info("Shader cache cleared");
    }

    std::string ShaderManager::getTargetProfile(ShaderType type) const
    {
        switch (type)
        {
        case ShaderType::Vertex:   return "vs_5_0";
        case ShaderType::Pixel:    return "ps_5_0";
        case ShaderType::Geometry: return "gs_5_0";
        case ShaderType::Hull:     return "hs_5_0";
        case ShaderType::Domain:   return "ds_5_0";
        case ShaderType::Compute:  return "cs_5_0";
        default:                   return "vs_5_0";
        }
    }

    UINT ShaderManager::getCompileFlags() const
    {
        UINT flags = 0;

#ifdef _DEBUG
        // �f�o�b�O�r���h�ł͏ڍׂȏ���L����
        flags |= D3DCOMPILE_DEBUG;
        flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        // �����[�X�r���h�ł͍œK����L����
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        return flags;
    }

    // BasicShaders����
    std::string BasicShaders::getBasicVertexShader()
    {
        return R"(
// ��{�I�Ȓ��_�V�F�[�_�[�i�P�F�O�p�`�p�j
struct VSOutput
{
    float4 position : SV_POSITION;
};

VSOutput main(float3 position : POSITION)
{
    VSOutput output;
    output.position = float4(position, 1.0f);
    return output;
}
)";
    }

    std::string BasicShaders::getBasicPixelShader()
    {
        return R"(
// ��{�I�ȃs�N�Z���V�F�[�_�[�i�P�F�O�p�`�p�j
float4 main() : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f); // �ԐF
}
)";
    }

    std::string BasicShaders::getColorVertexShader()
    {
        return R"(
// �J���[�t�����_�V�F�[�_�[
struct VSInput
{
    float3 position : POSITION;
    float3 color : COLOR;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0f);
    output.color = input.color;
    return output;
}
)";
    }

    std::string BasicShaders::getColorPixelShader()
    {
        return R"(
// �J���[�t���s�N�Z���V�F�[�_�[
struct PSInput
{
    float4 position : SV_POSITION;
    float3 color : COLOR;
};

float4 main(PSInput input) : SV_TARGET
{
    return float4(input.color, 1.0f);
}
)";
    }
}