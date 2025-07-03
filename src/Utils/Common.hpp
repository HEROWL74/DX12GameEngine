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

    struct Error
    {
        ErrorType type;
        std::string message;
        std::source_location location;
        HRESULT hr = S_OK; //WindowsAPI�p��HRESULT

        //�G���[���b�Z�[�W���擾
        [[nodiscard]] std::string what() const noexcept
        {
            std::string result = std::format(
                "Error: {} at {}:{} in function '{}'\n",
                message,
                location.file_name(),
                location.line(),
                location.function_name()
            );

            if (FAILED(hr))
            {
                std::format("HRESULT: 0x{:08x}\n", static_cast<unsigned>(hr));
            }

            return result;
        }
    };

    // =============================================================================
   // Result�^�̒�`�istd::expected�̕ʖ��j
     // =============================================================================

    //�������̒l���G���[��\���^
    template <typename T>
    using Result = std::expected<T, Error>;

    //vpid�^�ł��߂�l��Ԃ��悤�ɂ���
    using VoidResult = Result<void>;

    // =============================================================================
    // �G���[�쐬�p�̃w���p�[�֐�
     // =============================================================================

    // �G���[���쐬����w���p�[�֐�
    [[nodiscard]] constexpr Error make_error(
        ErrorType type,
        std::string_view message,
        HRESULT hr,
        std::source_location location = std::source_location::current()) noexcept
    {
        return Error{ type, std::string(message),location, hr };
    }
}