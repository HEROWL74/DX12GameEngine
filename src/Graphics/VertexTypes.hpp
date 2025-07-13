#pragma once
// src/Graphics/VertexTypes.hpp
#pragma once

namespace Engine::Graphics
{
    // ���_�f�[�^�\����
    struct Vertex
    {
        float position[3];  // x, y, z
        float color[3];     // r, g, b
    };

    // �����I�ɂ̓e�N�X�`�����W�t���̒��_���ǉ��\��
    struct VertexPosTexture
    {
        float position[3];  // x, y, z
        float texCoord[2];  // u, v
    };

    // �@���t���̒��_�i���C�e�B���O�p�j
    struct VertexPosNormal
    {
        float position[3];  // x, y, z
        float normal[3];    // nx, ny, nz
        float color[3];     // r, g, b
    };
}