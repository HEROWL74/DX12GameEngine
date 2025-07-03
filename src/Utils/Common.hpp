#pragma once

#include <Windows.h>
#include <expected>
#include <string>
#include <format>
#include <source_location>

namespace Engine::Utils 
{
    // =============================================================================
    // エラー型定義
    // =============================================================================

    //エンジン内で発生するエラーの種類
    enum class ErrorType
    {
        WindowCreation, //ウィンドウ作成エラー
        DeviceCreation, //デバイス作成エラー
        SwapChainCreation,//スワップチェイン作成エラー
        ResourceCreation, // リソース作成エラー
        ShaderCompilation, // シェーダーコンパイルエラー
        FileI0, // ファイルIO エラー
        Unknown // 不明なエラー
    };

    struct Error
    {
        ErrorType type;
        std::string message;
        std::source_location location;
        HRESULT hr = S_OK; //WindowsAPI用のHRESULT

        //エラーメッセージを取得
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
   // Result型の定義（std::expectedの別名）
     // =============================================================================

    //成功時の値かエラーを表す型
    template <typename T>
    using Result = std::expected<T, Error>;

    //vpid型でも戻り値を返すようにする
    using VoidResult = Result<void>;

    // =============================================================================
    // エラー作成用のヘルパー関数
     // =============================================================================

    // エラーを作成するヘルパー関数
    [[nodiscard]] constexpr Error make_error(
        ErrorType type,
        std::string_view message,
        HRESULT hr,
        std::source_location location = std::source_location::current()) noexcept
    {
        return Error{ type, std::string(message),location, hr };
    }
}