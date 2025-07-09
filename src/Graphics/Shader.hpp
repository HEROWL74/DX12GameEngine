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
    // �V�F�[�_�[�̎��
    enum class ShaderType
    {
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        Compute
    };

    // �V�F�[�_�[���
    struct ShaderInfo
    {
        ComPtr<ID3DBlob> blob;          // �R���p�C���ς݃V�F�[�_�[�R�[�h
        std::string entryPoint;         // �G���g���[�|�C���g��
        std::string target;             // �^�[�Q�b�g�v���t�@�C��
        ShaderType type;                // �V�F�[�_�[���
    };

    // �V�F�[�_�[�Ǘ��N���X
    class ShaderManager
    {
    public:
        ShaderManager() = default;
        ~ShaderManager() = default;

        // �R�s�[�E���[�u�֎~
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;
        ShaderManager(ShaderManager&&) = delete;
        ShaderManager& operator=(ShaderManager&&) = delete;

        // �����񂩂�V�F�[�_�[���R���p�C��
        [[nodiscard]] Utils::Result<ShaderInfo> compileFromString(
            const std::string& shaderCode,
            const std::string& entryPoint,
            ShaderType type,
            const std::string& shaderName = "InlineShader"
        );

        // �t�@�C������V�F�[�_�[���R���p�C��
        [[nodiscard]] Utils::Result<ShaderInfo> compileFromFile(
            const std::wstring& filePath,
            const std::string& entryPoint,
            ShaderType type
        );

        // �V�F�[�_�[���L���b�V���ɓo�^
        void registerShader(const std::string& name, const ShaderInfo& shader);

        // �L���b�V������V�F�[�_�[���擾
        [[nodiscard]] const ShaderInfo* getShader(const std::string& name) const;

        // ���ׂẴV�F�[�_�[���N���A
        void clear();

    private:
        // �V�F�[�_�[�L���b�V��
        std::unordered_map<std::string, ShaderInfo> m_shaderCache;

        // �V�F�[�_�[�^�C�v����^�[�Q�b�g�v���t�@�C�����擾
        [[nodiscard]] std::string getTargetProfile(ShaderType type) const;

        // �R���p�C���t���O���擾
        [[nodiscard]] UINT getCompileFlags() const;
    };
}