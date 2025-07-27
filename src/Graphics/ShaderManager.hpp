//src/Graphics/ShaderManager.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <wrl.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include "../Utils/Common.hpp"
#include "Device.hpp"

#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    //=========================================================================
    // �V�F�[�_�[�^�C�v�񋓌^
    //=========================================================================
    enum class ShaderType
    {
        Vertex,
        Pixel,
        Geometry,
        Hull,
        Domain,
        Compute
    };

    //=========================================================================
    // �V�F�[�_�[�}�N���\����
    //=========================================================================
    struct ShaderMacro
    {
        std::string name;
        std::string definition;

        ShaderMacro(const std::string& n, const std::string& d) : name(n), definition(d) {}
    };

    //=========================================================================
    // �V�F�[�_�[�R���p�C���ݒ�
    //=========================================================================
    struct ShaderCompileDesc
    {
        std::string filePath;
        std::string entryPoint;
        ShaderType type;
        std::vector<ShaderMacro> macros;
        bool enableDebug = false;
        bool enableOptimization = true;
    };

    //=========================================================================
    // �V�F�[�_�[�N���X
    //=========================================================================
    class Shader
    {
    public:
        Shader() = default;
        ~Shader() = default;

        // �R�s�[�E���[�u
        Shader(const Shader&) = delete;
        Shader& operator=(const Shader&) = delete;
        Shader(Shader&&) = default;
        Shader& operator=(Shader&&) = default;

        // �t�@�C������̃R���p�C��
        [[nodiscard]] static Utils::Result<std::shared_ptr<Shader>> compileFromFile(
            const ShaderCompileDesc& desc
        );

        // �����񂩂�̃R���p�C��
        [[nodiscard]] static Utils::Result<std::shared_ptr<Shader>> compileFromString(
            const std::string& shaderCode,
            const std::string& entryPoint,
            ShaderType type,
            const std::vector<ShaderMacro>& macros = {},
            bool enableDebug = false
        );

        // ��{���̎擾
        ShaderType getType() const { return m_type; }
        const std::string& getEntryPoint() const { return m_entryPoint; }
        const std::string& getFilePath() const { return m_filePath; }

        // �o�C�g�R�[�h�̎擾
        const void* getBytecode() const { return m_bytecode->GetBufferPointer(); }
        size_t getBytecodeSize() const { return m_bytecode->GetBufferSize(); }
        ID3DBlob* getBytecodeBlob() const { return m_bytecode.Get(); }

        // �L�����`�F�b�N
        bool isValid() const { return m_bytecode != nullptr; }

    private:
        ShaderType m_type = ShaderType::Vertex;
        std::string m_entryPoint;
        std::string m_filePath;
        ComPtr<ID3DBlob> m_bytecode;

        // ����������
        [[nodiscard]] Utils::VoidResult initialize(
            const std::string& shaderCode,
            const std::string& entryPoint,
            ShaderType type,
            const std::vector<ShaderMacro>& macros,
            bool enableDebug,
            const std::string& filePath = ""
        );

        // �w���p�[�֐�
        static std::string shaderTypeToTarget(ShaderType type);
        static std::vector<D3D_SHADER_MACRO> convertMacros(const std::vector<ShaderMacro>& macros);
    };

    //=========================================================================
    // ���[�g�V�O�l�`���L�q
    //=========================================================================
    struct RootParameterDesc
    {
        enum Type
        {
            ConstantBufferView,
            ShaderResourceView,
            UnorderedAccessView,
            DescriptorTable,
            Constants
        };

        Type type;
        uint32_t shaderRegister;
        uint32_t registerSpace = 0;
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;

        // DescriptorTable�p
        std::vector<D3D12_DESCRIPTOR_RANGE1> ranges;

        // Constants�p
        uint32_t numConstants = 0;
    };

    struct StaticSamplerDesc
    {
        uint32_t shaderRegister;
        uint32_t registerSpace = 0;
        D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL;
        D3D12_FILTER filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
        D3D12_TEXTURE_ADDRESS_MODE addressModeU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        D3D12_TEXTURE_ADDRESS_MODE addressModeV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        D3D12_TEXTURE_ADDRESS_MODE addressModeW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    };

    //=========================================================================
    // �p�C�v���C���X�e�[�g�L�q
    //=========================================================================
    struct PipelineStateDesc
    {
        std::shared_ptr<Shader> vertexShader;
        std::shared_ptr<Shader> pixelShader;
        std::shared_ptr<Shader> geometryShader;

        // ���̓��C�A�E�g
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;

        // ���[�g�V�O�l�`��
        std::vector<RootParameterDesc> rootParameters;
        std::vector<StaticSamplerDesc> staticSamplers;

        // �����_�[�X�e�[�g
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        std::vector<DXGI_FORMAT> rtvFormats = { DXGI_FORMAT_R8G8B8A8_UNORM };
        DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT;

        // �u�����h�X�e�[�g
        bool enableBlending = false;
        D3D12_BLEND srcBlend = D3D12_BLEND_ONE;
        D3D12_BLEND destBlend = D3D12_BLEND_ZERO;
        D3D12_BLEND_OP blendOp = D3D12_BLEND_OP_ADD;

        // ���X�^���C�U�[�X�e�[�g
        D3D12_CULL_MODE cullMode = D3D12_CULL_MODE_BACK;
        D3D12_FILL_MODE fillMode = D3D12_FILL_MODE_SOLID;
        bool enableDepthClip = true;

        // �[�x�X�e���V���X�e�[�g
        bool enableDepthTest = true;
        bool enableDepthWrite = true;
        D3D12_COMPARISON_FUNC depthFunc = D3D12_COMPARISON_FUNC_LESS;

        std::string debugName;
    };

    //=========================================================================
    // �p�C�v���C���X�e�[�g�N���X
    //=========================================================================
    class PipelineState
    {
    public:
        PipelineState() = default;
        ~PipelineState() = default;

        // �R�s�[�E���[�u
        PipelineState(const PipelineState&) = delete;
        PipelineState& operator=(const PipelineState&) = delete;
        PipelineState(PipelineState&&) = default;
        PipelineState& operator=(PipelineState&&) = default;

        // �쐬
        [[nodiscard]] static Utils::Result<std::shared_ptr<PipelineState>> create(
            Device* device,
            const PipelineStateDesc& desc
        );

        // D3D12�I�u�W�F�N�g�̎擾
        ID3D12PipelineState* getPipelineState() const { return m_pipelineState.Get(); }
        ID3D12RootSignature* getRootSignature() const { return m_rootSignature.Get(); }

        // ��{���
        const PipelineStateDesc& getDesc() const { return m_desc; }

        // �L�����`�F�b�N
        bool isValid() const { return m_pipelineState != nullptr && m_rootSignature != nullptr; }

        // �f�o�b�O���̐ݒ�
        void setDebugName(const std::string& name);

    private:
        Device* m_device = nullptr;
        PipelineStateDesc m_desc;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12RootSignature> m_rootSignature;

        // ������
        [[nodiscard]] Utils::VoidResult initialize(Device* device, const PipelineStateDesc& desc);
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createPipelineState();
    };

    //=========================================================================
    // �V�F�[�_�[�}�l�[�W���[�N���X
    //=========================================================================
    class ShaderManager
    {
    public:
        ShaderManager() = default;
        ~ShaderManager() = default;

        // �R�s�[�E���[�u�֎~
        ShaderManager(const ShaderManager&) = delete;
        ShaderManager& operator=(const ShaderManager&) = delete;

        // ������
        [[nodiscard]] Utils::VoidResult initialize(Device* device);

        // �V�F�[�_�[�Ǘ�
        std::shared_ptr<Shader> loadShader(const ShaderCompileDesc& desc);
        std::shared_ptr<Shader> getShader(const std::string& name) const;
        bool hasShader(const std::string& name) const;
        void removeShader(const std::string& name);

        // �p�C�v���C���X�e�[�g�Ǘ�
        std::shared_ptr<PipelineState> createPipelineState(
            const std::string& name,
            const PipelineStateDesc& desc
        );
        std::shared_ptr<PipelineState> getPipelineState(const std::string& name) const;
        bool hasPipelineState(const std::string& name) const;
        void removePipelineState(const std::string& name);

        // �f�t�H���g�V�F�[�_�[�̎擾
        std::shared_ptr<PipelineState> getDefaultPBRPipeline() const { return m_defaultPBRPipeline; }
        std::shared_ptr<PipelineState> getDefaultUnlitPipeline() const { return m_defaultUnlitPipeline; }

        // ���v���
        size_t getShaderCount() const { return m_shaders.size(); }
        size_t getPipelineStateCount() const { return m_pipelineStates.size(); }

        // �L�����`�F�b�N
        bool isValid() const { return m_initialized && m_device != nullptr; }

    private:
        Device* m_device = nullptr;
        bool m_initialized = false;

        // �V�F�[�_�[�L���b�V��
        std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaders;

        // �p�C�v���C���X�e�[�g�L���b�V��
        std::unordered_map<std::string, std::shared_ptr<PipelineState>> m_pipelineStates;

        // �f�t�H���g�p�C�v���C��
        std::shared_ptr<PipelineState> m_defaultPBRPipeline;
        std::shared_ptr<PipelineState> m_defaultUnlitPipeline;

        // �������w���p�[
        [[nodiscard]] Utils::VoidResult createDefaultShaders();
        [[nodiscard]] Utils::VoidResult createDefaultPipelines();

        // ���[�e�B���e�B
        std::string generateShaderKey(const ShaderCompileDesc& desc) const;
    };

    //=========================================================================
    // �W�����̓��C�A�E�g��`
    //=========================================================================
    namespace StandardInputLayouts
    {
        // �ʒu�̂�
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> Position;

        // �ʒu + UV
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionUV;

        // �ʒu + �@�� + UV
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PositionNormalUV;

        // PBR�p�i�ʒu + �@�� + UV + �ڐ��j
        extern const std::vector<D3D12_INPUT_ELEMENT_DESC> PBRVertex;
    }

    //=========================================================================
    // ���[�e�B���e�B�֐�
    //=========================================================================

    // �t�@�C���̓ǂݍ���
    Utils::Result<std::string> readShaderFile(const std::string& filePath);

    // �C���N���[�h�t�@�C���̏���
    std::string processIncludes(const std::string& shaderCode, const std::string& baseDir);
}