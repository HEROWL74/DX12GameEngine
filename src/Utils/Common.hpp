#pragma once

#include <Windows.h>
#include <expected>
#include <string>
#include <format>
#include <source_location>

namespace Engine::Utils 
{
    // =============================================================================
    // �G���[�^��`
    // =============================================================================

    //�G���W�����Ŕ�������G���[�̎��
    enum class ErrorType
    {
        WindowCreation, //�E�B���h�E�쐬�G���[
        DeviceCreation, //�f�o�C�X�쐬�G���[
        SwapChainCreation,//�X���b�v�`�F�C���쐬�G���[
        ResourceCreation, // ���\�[�X�쐬�G���[
        ShaderCompilation, // �V�F�[�_�[�R���p�C���G���[
        FileI0, // �t�@�C��IO �G���[
        Unknown // �s���ȃG���[
    };
}