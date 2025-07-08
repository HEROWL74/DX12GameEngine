// src/Graphics/TriangleRenderer.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <array>
#include "Device.hpp"
#include "Shader.hpp"
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // ���_�f�[�^�\����
    struct Vertex
    {
        float position[3];  // x, y, z
        float color[3];     // r, g, b
    };

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
        [[nodiscard]] Utils::VoidResult initialize(Device* device);

        // �O�p�`��`��
        void render(ID3D12GraphicsCommandList* commandList);

        // �L�����`�F�b�N
        [[nodiscard]] bool isValid() const noexcept { return m_rootSignature != nullptr; }

    private:
        Device* m_device = nullptr;
        ShaderManager m_shaderManager;

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
    };
}