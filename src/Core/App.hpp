// src/Core/App.hpp
#pragma once

#include <Windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <chrono>

#include "Window.hpp"
#include "../Graphics/Device.hpp"
#include "../Graphics/TriangleRenderer.hpp"
#include "../Graphics/Camera.hpp"
#include "../Input/InputManager.hpp"
#include "../Utils/Common.hpp"
#include "../Graphics/CubeRenderer.hpp"

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

        // シングルトン的な使用を想定しているのでコピー・ムーブ禁止
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
        Graphics::CubeRenderer m_cubeRenderer;
        std::vector<Graphics::CubeRenderer> m_cubes; //複数の立方体用
        Graphics::Camera m_camera;
        std::unique_ptr<Graphics::FPSCameraController> m_cameraController;

        // スワップチェーン関係
        ComPtr<ID3D12CommandQueue> m_commandQueue;      // GPU命令の送信先
        ComPtr<IDXGISwapChain3> m_swapChain;            // 画面表示用バッファ管理
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;         // レンダーターゲットビュー用デスクリプタヒープ
        ComPtr<ID3D12Resource> m_renderTargets[2];      // 描画先バッファ（ダブルバッファリング）

        //深度バッファ関係
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
        ComPtr<ID3D12Resource> m_depthStencilBuffer;

        // コマンド関係
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;  // コマンドリスト用メモリ管理
        ComPtr<ID3D12GraphicsCommandList> m_commandList;    // GPU命令の記録

        // 同期用
        ComPtr<ID3D12Fence> m_fence;        // CPU-GPU同期用
        UINT64 m_fenceValue = 0;            // フェンス値
        HANDLE m_fenceEvent = nullptr;      // フェンスイベント

        // 描画関係
        UINT m_frameIndex = 0;              // 現在のフレームインデックス

        // 時間管理
        std::chrono::high_resolution_clock::time_point m_lastFrameTime{};
        float m_deltaTime = 0.0f;
        float m_currentFPS = 0.0f;
        int m_frameCount = 0;
        float m_frameTimeAccumulator = 0.0f;

        // 初期化処理
        [[nodiscard]] Utils::VoidResult initD3D();
        [[nodiscard]] Utils::VoidResult initializeInput();
        [[nodiscard]] Utils::VoidResult createCommandQueue();
        [[nodiscard]] Utils::VoidResult createSwapChain();
        [[nodiscard]] Utils::VoidResult createRenderTargets();
        [[nodiscard]] Utils::VoidResult createDepthStencilBuffer(); //新規追加
        [[nodiscard]] Utils::VoidResult createCommandObjects();
        [[nodiscard]] Utils::VoidResult createSyncObjects();

        // 更新・描画処理
        void update();
        void render();

        // 時間管理
        void updateDeltaTime();
        void processInput();

        // フレーム完了待ち
        void waitForPreviousFrame();

        // リソースの破棄
        void cleanup();

        // イベントハンドラ
        void onWindowResize(int width, int height);
        void onWindowClose();

        // 入力イベントハンドラ
        void onKeyPressed(Input::KeyCode key);
        void onKeyReleased(Input::KeyCode key);
        void onMouseMove(int x, int y, int deltaX, int deltaY);
        void onMouseButtonPressed(Input::MouseButton button, int x, int y);
    };
}