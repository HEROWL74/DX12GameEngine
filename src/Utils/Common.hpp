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
     //@brief �G���[�쐬�p�̃w���p�[�֐�
     // =============================================================================

    // �G���[���쐬����w���p�[�֐�
    [[nodiscard]] constexpr Error make_error(
        ErrorType type,
        std::string_view message,
        std::source_location location = std::source_location::current()) noexcept
    {
        return Error{ type, std::string(message), location, S_OK };
    }



    // =============================================================================
   // HRESULT �`�F�b�N�p�}�N��
   // =============================================================================


    //RESULT���`�F�b�N���āA���s���ɃG���[��Ԃ��}�N��
#define CHECK_HR(hr, error_type, message) \
        do { \
            HRESULT _hr = (hr); \
            if (FAILED(_hr)) { \
                return std::unexpected(Engine::Utils::make_error(error_type, message, _hr)); \
            } \
        } while(0)

#undef CHECK_CONDITION

    /// �������`�F�b�N���āA���s���ɃG���[��Ԃ��}�N��
#define CHECK_CONDITION(condition, error_type, message) \
        do { \
            if (!(condition)) { \
                return std::unexpected(Engine::Utils::make_error(error_type, message)); \
            } \
        } while(0)

    //���O�o�͗p�̃w���p�[�֐�

    //�G���[���f�o�b�O�o�͂ɕ\��
    inline void log_error(const Error& error) noexcept
    {
        OutputDebugStringA(error.what().c_str());
    }

    //�����f�o�b�O�o�͂ɕ\��
    inline void log_info(std::string_view message) noexcept
    {
        OutputDebugStringA(std::format("[INFO] {}\n", message).c_str());
    }

    //�x�����f�o�b�O�ɕ\��
    inline void log_warning(std::string_view message) noexcept 
    {
        OutputDebugStringA(std::format("[WARNING] {}\n", message).c_str());
    }


}