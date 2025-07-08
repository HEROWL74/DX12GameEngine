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
            shaderCode.c_str(),         // シェーダーコード
            shaderCode.size(),          // コードサイズ
            shaderName.c_str(),         // ファイル名（デバッグ用）
            nullptr,                    // マクロ定義
            nullptr,                    // インクルードハンドラー
            entryPoint.c_str(),         // エントリーポイント
            target.c_str(),             // ターゲットプロファイル
            flags,                      // コンパイルフラグ
            0,                          // エフェクトフラグ
            &shaderBlob,                // 出力：コンパイル済みシェーダー
            &errorBlob                  // 出力：エラーメッセージ
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
        // ファイルを読み込み
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

        // ファイル名を取得
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
        // デバッグビルドでは詳細な情報を有効化
        flags |= D3DCOMPILE_DEBUG;
        flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        // リリースビルドでは最適化を有効化
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        return flags;
    }

    // BasicShaders実装
    std::string BasicShaders::getBasicVertexShader()
    {
        return R"(
// 基本的な頂点シェーダー（単色三角形用）
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
// 基本的なピクセルシェーダー（単色三角形用）
float4 main() : SV_TARGET
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f); // 赤色
}
)";
    }

    std::string BasicShaders::getColorVertexShader()
    {
        return R"(
// カラー付き頂点シェーダー
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
// カラー付きピクセルシェーダー
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