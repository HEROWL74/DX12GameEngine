// Core/App.hpp（クラス定義）
#pragma once

#include <Windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Core
{
    class App
    {
    public:
        [[nodiscard]]
        bool initialize(HINSTANCE hInstance, int nCmdShow) noexcept;

        [[nodiscard]]
        int run() noexcept;

    private:
        // ウィンドウ関係
        HWND windowHandle = nullptr;
        static constexpr int width = 1280;
        static constexpr int height = 720;

        // 初期化関数
        [[nodiscard]]
        bool initWindow(HINSTANCE hInstance, int nCmdShow) noexcept;

        [[nodiscard]]
        bool initD3D();

        // レンダリング関数
        void update();
        void render();

        // DirectX 12関係のメンバ変数
        ComPtr<ID3D12Device> m_device;              // D3D12デバイス（GPU制御の中核）
        ComPtr<ID3D12CommandQueue> m_commandQueue;  // コマンドキュー（GPU命令の送信先）
        ComPtr<IDXGIFactory4> m_dxgiFactory;        // DXGI ファクトリ（スワップチェーン作成用）
        ComPtr<IDXGISwapChain3> m_swapChain;        // スワップチェーン（画面表示用バッファ管理）
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;     // レンダーターゲットビュー用デスクリプタヒープ
        ComPtr<ID3D12Resource> m_renderTargets[2];  // レンダーターゲット（描画先バッファ）
        ComPtr<ID3D12CommandAllocator> m_commandAllocator; // コマンドアロケータ（コマンドリスト用メモリ管理）
        ComPtr<ID3D12GraphicsCommandList> m_commandList;   // コマンドリスト（GPU命令の記録）

        // 同期用
        ComPtr<ID3D12Fence> m_fence;    // フェンス（CPU-GPU同期用）
        UINT64 m_fenceValue = 0;        // フェンス値
        HANDLE m_fenceEvent = nullptr;  // フェンスイベント

        // 描画関係
        UINT m_rtvDescriptorSize = 0;   // レンダーターゲットビューのデスクリプタサイズ
        UINT m_frameIndex = 0;          // 現在のフレームインデックス

        // 同期待機用のヘルパー関数
        void waitForPreviousFrame();
    };
}