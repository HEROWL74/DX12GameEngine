// src/Graphics/Shader.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <string>
#include <unordered_map>
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // シェーダーの種類
    enum class ShaderType
    {
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        Compute
    };

    // シェーダー情報
    struct ShaderInfo
    {
        ComPtr<ID3DBlob> blob;          // コンパイル済みシェーダーコード
        std::string entryPoint;         // エントリーポイント名
        std::string target;             // ターゲットプロファイル
        ShaderType type;                // シェーダー種類
    };

    // シェーダー管理クラス
    class ShaderManager
    {
    public:
        ShaderManager() = default;
        ~ShaderManager() = default;

        // コピー・ムーブ禁止
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;
        ShaderManager(ShaderManager&&) = delete;
        ShaderManager& operator=(ShaderManager&&) = delete;

        // 文字列からシェーダーをコンパイル
        [[nodiscard]] Utils::Result<ShaderInfo> compileFromString(
            const std::string& shaderCode,
            const std::string& entryPoint,
            ShaderType type,
            const std::string& shaderName = "InlineShader"
        );

        // ファイルからシェーダーをコンパイル
        [[nodiscard]] Utils::Result<ShaderInfo> compileFromFile(
            const std::wstring& filePath,
            const std::string& entryPoint,
            ShaderType type
        );

        // シェーダーをキャッシュに登録
        void registerShader(const std::string& name, const ShaderInfo& shader);

        // キャッシュからシェーダーを取得
        [[nodiscard]] const ShaderInfo* getShader(const std::string& name) const;

        // すべてのシェーダーをクリア
        void clear();

    private:
        // シェーダーキャッシュ
        std::unordered_map<std::string, ShaderInfo> m_shaderCache;

        // シェーダータイプからターゲットプロファイルを取得
        [[nodiscard]] std::string getTargetProfile(ShaderType type) const;

        // コンパイルフラグを取得
        [[nodiscard]] UINT getCompileFlags() const;
    };
}