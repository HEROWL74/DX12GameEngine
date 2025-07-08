// src/Core/App.hpp
#pragma once

#include <Windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>

#include "Window.hpp"
#include "../Graphics/Device.hpp"
#include "../Utils/Common.hpp"
#include "../Graphics/TriangleRenderer.hpp"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Engine::Core
{
    class App
    {
    public:
        App() = default;
        ~App() = default;

        // シングルトン的な使用するためコピー・ムーブ禁止
        App(const App&) = delete;
        App& operator=(const App&) = delete;
        App(App&&) = delete;
        App& operator=(App&&) = delete;

        // アプリケーションの初期化
        [[nodiscard]] Utils::VoidResult initialize(HINSTANCE hInstance, int nCmdShow);

        // メインループを実行
        [[nodiscard]] int run();

    private:
        // ウィンドウとデバイス管理
        Window m_window;
        Graphics::Device m_device;
        Graphics::TriangleRenderer m_triangleRenderer;

        // スワップチェーン関係
        ComPtr<ID3D12CommandQueue> m_commandQueue;      // GPU命令の送信先
        ComPtr<IDXGISwapChain3> m_swapChain;            // 画面表示用バッファ管理
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;         // レンダーターゲットビュー用デスクリプタヒープ
        ComPtr<ID3D12Resource> m_renderTargets[2];      // 描画先バッファ（ダブルバッファリング）

        // コマンド関係
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;  // コマンドリスト用メモリ管理
        ComPtr<ID3D12GraphicsCommandList> m_commandList;    // GPU命令の記録

        // 同期用
        ComPtr<ID3D12Fence> m_fence;        // CPU-GPU同期用
        UINT64 m_fenceValue = 0;            // フェンス値
        HANDLE m_fenceEvent = nullptr;      // フェンスイベント

        // 描画関係
        UINT m_frameIndex = 0;              // 現在のフレームインデックス

        // 初期化処理
        [[nodiscard]] Utils::VoidResult initD3D();
        [[nodiscard]] Utils::VoidResult createCommandQueue();
        [[nodiscard]] Utils::VoidResult createSwapChain();
        [[nodiscard]] Utils::VoidResult createRenderTargets();
        [[nodiscard]] Utils::VoidResult createCommandObjects();
        [[nodiscard]] Utils::VoidResult createSyncObjects();

        // 更新・描画処理
        void update();
        void render();

        // フレーム完了待ち
        void waitForPreviousFrame();

        // リソースの破棄
        void cleanup();

        // イベントハンドラ
        void onWindowResize(int width, int height);
        void onWindowClose();
    };
}