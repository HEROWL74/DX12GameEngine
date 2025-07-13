// src/Graphics/CubeRenderer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Device.hpp"
#include "Shader.hpp"
#include "ConstantBuffer.hpp"
#include "Camera.hpp"
#include "VertexTypes.hpp"
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // �����̕`���p�̃����_���[
    class CubeRenderer
    {
    public:
        CubeRenderer() = default;
        ~CubeRenderer() = default;

        // �R�s�[�E���[�u�֎~
        CubeRenderer(const CubeRenderer&) = delete;
        CubeRenderer& operator=(const CubeRenderer&) = delete;
        CubeRenderer(CubeRenderer&&) = delete;
        CubeRenderer& operator=(CubeRenderer&&) = delete;

        // ������
        [[nodiscard]] Utils::VoidResult initialize(Device* device);

        // �����̂�`��
        void render(ID3D12GraphicsCommandList* commandList, const Camera& camera, UINT frameIndex);

        // 3D��Ԃł̈ʒu�E��]�E�X�P�[����ݒ�
        void setPosition(const Math::Vector3& position) { m_position = position; updateWorldMatrix(); }
        void setRotation(const Math::Vector3& rotation) { m_rotation = rotation; updateWorldMatrix(); }
        void setScale(const Math::Vector3& scale) { m_scale = scale; updateWorldMatrix(); }

        // �Q�b�^�[
        const Math::Vector3& getPosition() const { return m_position; }
        const Math::Vector3& getRotation() const { return m_rotation; }
        const Math::Vector3& getScale() const { return m_scale; }

        // �L�����`�F�b�N
        [[nodiscard]] bool isValid() const noexcept {
            return m_rootSignature != nullptr &&
                m_pipelineState != nullptr &&
                m_vertexBuffer != nullptr &&
                m_indexBuffer != nullptr &&
                m_constantBufferManager.isValid();
        }

    private:
        Device* m_device = nullptr;
        ShaderManager m_shaderManager;
        ConstantBufferManager m_constantBufferManager;

        // 3D�ϊ��p�����[�^
        Math::Vector3 m_position = Math::Vector3::zero();
        Math::Vector3 m_rotation = Math::Vector3::zero();
        Math::Vector3 m_scale = Math::Vector3::one();
        Math::Matrix4 m_worldMatrix;

        // �`�惊�\�[�X
        ComPtr<ID3D12RootSignature> m_rootSignature;
        ComPtr<ID3D12PipelineState> m_pipelineState;
        ComPtr<ID3D12Resource> m_vertexBuffer;
        ComPtr<ID3D12Resource> m_indexBuffer;
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
        D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

        // �����̂̒��_�f�[�^�i24���_ - �e�ʂ�4���_�j
        std::array<Vertex, 24> m_cubeVertices;

        // �����̂̃C���f�b�N�X�f�[�^�i36�C���f�b�N�X - 12�O�p�` * 3���_�j
        std::array<uint16_t, 36> m_cubeIndices;

        // �������p���\�b�h
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createShaders();
        [[nodiscard]] Utils::VoidResult createPipelineState();
        [[nodiscard]] Utils::VoidResult createVertexBuffer();
        [[nodiscard]] Utils::VoidResult createIndexBuffer();

        // �����̂̒��_�f�[�^��ݒ�
        void setupCubeVertices();

        // ���[���h�s����X�V
        void updateWorldMatrix();
    };
}