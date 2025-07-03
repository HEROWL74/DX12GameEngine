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
}