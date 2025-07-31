// src/Graphics/TriangleRenderer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Device.hpp"
#include "ShaderManager.hpp"
#include "ConstantBuffer.hpp"
#include "Camera.hpp"
#include "VertexTypes.hpp"  // �ǉ�
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // �O�p�`�`���p�̃����_���[
    class TriangleRenderer
    {
    public:
        TriangleRenderer() = default;
        ~TriangleRenderer() = default;

        // �R�s�[�E���[�u�֎~
        TriangleRenderer(const TriangleRenderer&) = delete;
        TriangleRenderer& operator=(const TriangleRenderer&) = delete;
        TriangleRenderer(TriangleRenderer&&) = delete;
        TriangleRenderer& operator=(TriangleRenderer&&) = delete;

        // ������
        [[nodiscard]] Utils::VoidResult initialize(Device* device, ShaderManager* shaderManager);

        // �O�p�`��`��
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
        [[nodiscard]] bool isValid() const noexcept { return m_rootSignature != nullptr && m_constantBufferManager.isValid(); }

    private:
        Device* m_device = nullptr;
        //ShaderManager m_shaderManager;
        ShaderManager* m_shaderManager = nullptr;
        ConstantBufferManager m_constantBufferManager;

        // 3D�ϊ��p�����[�^
        Math::Vector3 m_position = Math::Vector3::zero();
        Math::Vector3 m_rotation = Math::Vector3::zero();
        Math::Vector3 m_scale = Math::Vector3::one();
        Math::Matrix4 m_worldMatrix;

        // �`�惊�\�[�X
        ComPtr<ID3D12RootSignature> m_rootSignature;        // ���[�g�V�O�l�`��
        ComPtr<ID3D12PipelineState> m_pipelineState;        // �p�C�v���C���X�e�[�g
        ComPtr<ID3D12Resource> m_vertexBuffer;              // ���_�o�b�t�@
        D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};      // ���_�o�b�t�@�r���[

        // �O�p�`�̒��_�f�[�^
        std::array<Vertex, 3> m_triangleVertices;

        // �������p���\�b�h
        [[nodiscard]] Utils::VoidResult createRootSignature();
        [[nodiscard]] Utils::VoidResult createShaders();
        [[nodiscard]] Utils::VoidResult createPipelineState();
        [[nodiscard]] Utils::VoidResult createVertexBuffer();

        // �O�p�`�̒��_�f�[�^��ݒ�
        void setupTriangleVertices();

        // ���[���h�s����X�V
        void updateWorldMatrix();
    };
}