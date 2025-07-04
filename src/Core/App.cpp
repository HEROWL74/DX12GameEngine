// App.cpp
#include "App.hpp"

namespace Engine::Core
{
    Utils::VoidResult App::initialize(HINSTANCE hInstance, int nCmdShow) noexcept
    {
        Utils::log_info("Initializing Game Engine...");

        // ---ウィンドウの設定---//
        WindowSettings windowSettings{
            .title = L"DX12 Game Engine",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .fullScreen = false
        };

        //ウィンドウを作成
        auto windowResult = m_window.create(hInstance, windowSettings);
        if (!windowResult)
        {
            Utils::log_error(windowResult.error());
            return std::unexpected(windowResult.error());
        }

        //ウィンドウイベントのコールバックを設定
        m_window.setResizeCallback([this](int width, int height) {
            onWindowResize(width, height);
            });

        m_window.setCloseCallback([this]() {
            onWindowClose();
        });

        //ウィンドウを表示
        m_window.show(nCmdShow);
            
        //DirectX12の初期化
        auto d3dResult = initD3D();
        if (!d3dResult)
        {
            Utils::log_error(d3dResult.error());
            return std::unexpected(d3dResult.error());
        }

        Utils::log_info("Game Engine initialization completed successfully!");
        return {};
    }

    int App::run() noexcept
    {
        Utils::log_info("Starting main loop...");

        //メインループ
        while (m_window.processMessages())
        {
            update();
            render();
        }

        //クリーンアップ
        cleanup();

        Utils::log_info("Application terminated successfully");
        return 0;
    }

   

    Utils::VoidResult App::initD3D()
    {
        Utils::log_info("Initializing DirectX 12...");

        //デバッグレイヤーの有効化　（デバッグビルドだけ）
#ifdef _DEBUG
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            Utils::log_info("D3D12 Debug Layer enabled");
        }
#endif

        //DXGIファクトリの作成
        CHECK_HR(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)),
            Utils::ErrorType::DeviceCreation, "Failed to create DXGI factory");

        //D3D12デバイスの作成
        CHECK_HR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)),
            Utils::ErrorType::DeviceCreation, "Failed to create D3D12 Device");

        //コマンドキューの作成
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        CHECK_HR(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)),
            Utils::ErrorType::DeviceCreation, "Failed to create command queue");

        //スワップチェインの作成
        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.BufferCount = 2;
        swapChainDesc.Width = clientWidth;
        swapChainDesc.Height = clientHeight;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain1;
        CHECK_HR(m_dxgiFactory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_window.getHandle(),
            &swapChainDesc,
            nullptr, nullptr,
            &swapChain1),
            Utils::ErrorType::SwapChainCreation, "Failed to create swap chain");

        CHECK_HR(swapChain1.As(&m_swapChain),
            Utils::ErrorType::SwapChainCreation, "Failed to cast swap chain to IDXGISwapChain3");

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        //レンダーターゲットビュー用ディスクリプタヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create RTV descriptor heap");

        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        //レンダーターゲットビューの作成
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        for (UINT i = 0; i < 2; i++) {
            CHECK_HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])),
                Utils::ErrorType::ResourceCreation,
                std::format("Failed to get swap chain buffer {}", i));

            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += m_rtvDescriptorSize;
        }

        //コマンドアロケータの作成
        CHECK_HR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)),
            Utils::ErrorType::ResourceCreation, "Failed to create command allocator");

        //コマンドリストの作成
        CHECK_HR(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)),
            Utils::ErrorType::ResourceCreation, "Failed to create fence");

        //コマンドリストは、作成時に記録状態になっているため、一回クローズする。

        m_commandList->Close();

        CHECK_HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
            Utils::ErrorType::ResourceCreation, "Failed to create fence event");

        m_fenceValue = 1;

        //フェンスイベントの作成
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK_CONDITION(m_fenceEvent != nullptr, Utils::ErrorType::ResourceCreation,
            "Failed to create fence event");

        Utils::log_info("DirectX 12 initialization completed successfully!");
        return {};
    }

    void App::update()
    {
        // 現在は何もしない
        // TODO 将来的にはゲームロジックやアニメーションの更新処理を記述
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

    void App::cleanup()
    {
        //GPU処理完了を待つ
        waitForPreviousFrame();

        //フェンスイベントのクリーンアップ
        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        Utils::log_info("DirectX 12 resources cleaned up");
    }

    void App::onWindowResize(int width, int height)
    {
        Utils::log_info(std::format("Window resized: {}x{}", width, height));
        // TODO 将来的にはスワップチェーンのリサイズ処理を実装予定
        // 現在は何もしない
    }

    void App::onWindowClose()
    {
        Utils::log_info("Window close requested");
    }
}