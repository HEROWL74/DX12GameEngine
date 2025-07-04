// Core/App.hpp（クラス定義）
#pragma once

#include <Windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#include "Window.hpp"
#include "../Utils/Common.hpp"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Engine::Core
{
    class App
    {
    public:
        //コンストラクタ
        App() = default;

        //デストラクタ
        ~App() = default;

        //シングルトン
        //コピーとムーブ禁止
        App(const App&) = delete;
        App& operator=(const App&) = delete;
        App(App&&) = delete;
        App& operator=(App&&) = delete;

        //アプリケーション初期化
        // hInstance アプリケーションインスタンス
        // nCmdShow 表示方法
        //成功時はvoid、失敗時は、エラーを返す
        
        [[nodiscard]]Utils::VoidResult initialize(HINSTANCE hInstance, int nCmdShow) noexcept;

        
        [[nodiscard]] int run() noexcept;

    private:

        // =============================================================================
       // メンバ変数
       // =============================================================================


        //ウィンドウ管理
        Window m_window;

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

        // =============================================================================
        // プライベートメソッド
        // =============================================================================

        //DirectX 12の初期化
        // 成功時はvoid、失敗時はError
        [[nodiscard]] Utils::VoidResult initD3D();

        //更新処理
        void update();

        //描画処理
        void render();

        //フレーム完了待ち
        void waitForPreviousFrame();

        /// @brief リソースの破棄
        void cleanup();

        // =============================================================================
        // イベントハンドラ
        // =============================================================================

        //ウィンドウリサイズ時のコールバック
        // width 新しい幅
        // height 新しい高さ
        void onWindowResize(int width, int height);

        /// @brief ウィンドウクローズ時のコールバック
        void onWindowClose();
    };
}