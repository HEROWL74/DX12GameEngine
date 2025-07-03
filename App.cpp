// App.cpp
#include "src/Core/App.hpp"

namespace Core
{
    bool App::initialize(HINSTANCE hInstance, int nCmdShow) noexcept
    {
        // ウィンドウの初期化
        if (!initWindow(hInstance, nCmdShow)) {
            return false;
        }

        // DirectX 12の初期化
        if (!initD3D()) {
            return false;
        }

        return true;
    }

    int App::run() noexcept
    {
        MSG msg{};
        while (msg.message != WM_QUIT)
        {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                // メインループ：更新と描画を実行
                update();
                render();
            }
        }

        // 終了前にGPUの処理完了を待つ
        waitForPreviousFrame();

        // フェンスイベントのクリーンアップ
        if (m_fenceEvent) {
            CloseHandle(m_fenceEvent);
        }

        return static_cast<int>(msg.wParam);
    }

    bool App::initWindow(HINSTANCE hInstance, int nCmdShow) noexcept
    {
        constexpr wchar_t CLASS_NAME[] = L"DX12WindowClass";

        const WNDCLASS wc{
            .lpfnWndProc = DefWindowProc,
            .hInstance = hInstance,
            .lpszClassName = CLASS_NAME
        };
        RegisterClass(&wc);

        windowHandle = CreateWindowEx(0, CLASS_NAME, L"DX12 Engine",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
            width, height, nullptr, nullptr, hInstance, nullptr);

        if (!windowHandle) return false;

        ShowWindow(windowHandle, nCmdShow);
        return true;
    }

    bool App::initD3D()
    {
        // --- デバッグレイヤーの有効化（デバッグビルドdakeだけ） ---
#ifdef _DEBUG
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
#endif

        // --- DXGIファクトリの作成 ---
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)))) {
            return false;
        }

        // --- D3D12デバイスの作成 ---
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
            return false;
        }

        // --- コマンドキューの作成 ---
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  // 直接コマンドリスト用

        if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
            return false;
        }

        // --- スワップチェーンの作成 ---
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.BufferCount = 2;              // ダブルバッファリング
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 32bit RGBA
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;         // マルチサンプリング無し

        ComPtr<IDXGISwapChain1> swapChain1;
        if (FAILED(m_dxgiFactory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),   // GPU命令を流すキュー
            windowHandle,           // 対象のウィンドウ
            &swapChainDesc,         // スワップチェーン設定
            nullptr, nullptr,       // 全画面設定なし
            &swapChain1))) {
            return false;
        }

        // IDXGISwapChain3にキャスト
        if (FAILED(swapChain1.As(&m_swapChain))) {
            return false;
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // --- レンダーターゲットビュー用デスクリプタヒープの作成 ---
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;                         // バックバッファ2個分
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;      // レンダーターゲットビュー用
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;    // GPU側からは参照しない

        if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
            return false;
        }

        // デスクリプタのサイズを取得（GPU依存）
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // --- レンダーターゲットビューの作成 ---
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        // スワップチェーンの各バックバッファに対してレンダーターゲットビューを作成
        for (UINT i = 0; i < 2; i++)
        {
            if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])))) {
                return false;
            }

            // レンダーターゲットビューを作成
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);

            // 次のデスクリプタへ移動
            rtvHandle.ptr += m_rtvDescriptorSize;
        }

        // --- コマンドアロケータの作成 ---
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)))) {
            return false;
        }

        // --- コマンドリストの作成 ---
        if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)))) {
            return false;
        }

        // コマンドリストは作成時に記録状態になっているので、一度クローズする
        m_commandList->Close();

        // --- 同期用フェンスの作成 ---
        if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
            return false;
        }
        m_fenceValue = 1;

        // フェンスイベントの作成
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!m_fenceEvent) {
            return false;
        }

        return true;
    }

    void App::update()
    {
        // 現在は何もしない
        // 将来的にはゲームロジックやアニメーションの更新処理を記述
    }

    void App::render()
    {
        // --- コマンドリストの記録開始 ---
        // コマンドアロケータとコマンドリストをリセット
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        // --- リソースバリア：バックバッファをレンダーターゲット状態に変更 ---
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;        // 表示状態から
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;   // レンダーターゲット状態へ
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);

        // --- レンダーターゲットの設定 ---
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;  // 現在のフレームのRTVへ移動

        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // --- 画面クリア（青色で塗りつぶし） ---
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };  // R, G, B, A（濃い青）
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // --- リソースバリア：バックバッファを表示状態に戻す ---
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;  // レンダーターゲット状態から
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;         // 表示状態へ

        m_commandList->ResourceBarrier(1, &barrier);

        // --- コマンドリストの記録終了 ---
        m_commandList->Close();

        // --- コマンドリストの実行 ---
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // --- 画面に表示 ---
        m_swapChain->Present(1, 0);  // 1 = V-Sync有効, 0 = フラグなし

        // --- フレーム完了待ち ---
        waitForPreviousFrame();
    }

    void App::waitForPreviousFrame()
    {
        // フェンスにシグナルを送信
        const UINT64 fence = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fence);
        m_fenceValue++;

        // GPUがフェンス値に到達するまで待機
        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        // 次のフレームインデックスを取得
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }
}