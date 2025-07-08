// src/Core/App.cpp
#include "App.hpp"
#include <format>

namespace Engine::Core
{
    Utils::VoidResult App::initialize(HINSTANCE hInstance, int nCmdShow)
    {
        Utils::log_info("Initializing Game Engine...");

        // ウィンドウの作成
        WindowSettings windowSettings{
            .title = L"DX12 Game Engine",
            .width = 1280,
            .height = 720,
            .resizable = true,
            .fullScreen = false
        };

        auto windowResult = m_window.create(hInstance, windowSettings);
        if (!windowResult)
        {
            Utils::log_error(windowResult.error());
            return std::unexpected(windowResult.error());
        }

        // ウィンドウイベントのコールバックを設定
        m_window.setResizeCallback([this](int width, int height) {
            onWindowResize(width, height);
            });

        m_window.setCloseCallback([this]() {
            onWindowClose();
            });

        m_window.show(nCmdShow);

        // DirectX 12の初期化
        auto d3dResult = initD3D();
        if (!d3dResult)
        {
            Utils::log_error(d3dResult.error());
            return std::unexpected(d3dResult.error());
        }

        Utils::log_info("Game Engine initialization completed successfully!");
        return {};
    }

    int App::run()
    {
        Utils::log_info("Starting main loop...");

        // メインループ
        while (m_window.processMessages())
        {
            update();
            render();
        }

        cleanup();

        Utils::log_info("Application terminated successfully.");
        return 0;
    }

    Utils::VoidResult App::initD3D()
    {
        Utils::log_info("Initializing DirectX 12...");

        // デバイスの初期化
        Graphics::DeviceSettings deviceSettings{
            .enableDebugLayer = true,
            .enableGpuValidation = false,
            .minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
            .preferHighPerformanceAdapter = true
        };

        auto deviceResult = m_device.initialize(deviceSettings);
        if (!deviceResult)
        {
            return deviceResult;
        }

        // 各コンポーネントを順番に初期化
        auto queueResult = createCommandQueue();
        if (!queueResult) return queueResult;

        auto swapChainResult = createSwapChain();
        if (!swapChainResult) return swapChainResult;

        auto renderTargetResult = createRenderTargets();
        if (!renderTargetResult) return renderTargetResult;

        auto commandResult = createCommandObjects();
        if (!commandResult) return commandResult;

        auto syncResult = createSyncObjects();
        if (!syncResult) return syncResult;

        // 三角形レンダラーの初期化
        auto triangleResult = m_triangleRenderer.initialize(&m_device);
        if (!triangleResult) return triangleResult;

        Utils::log_info("DirectX 12 initialization completed successfully!");
        return {};
    }

    Utils::VoidResult App::createCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

        CHECK_HR(m_device.getDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)),
            Utils::ErrorType::DeviceCreation, "Failed to create command queue");

        return {};
    }

    Utils::VoidResult App::createSwapChain()
    {
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
        CHECK_HR(m_device.getDXGIFactory()->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            m_window.getHandle(),
            &swapChainDesc,
            nullptr, nullptr,
            &swapChain1),
            Utils::ErrorType::SwapChainCreation, "Failed to create swap chain");

        CHECK_HR(swapChain1.As(&m_swapChain),
            Utils::ErrorType::SwapChainCreation, "Failed to cast swap chain to IDXGISwapChain3");

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        return {};
    }

    Utils::VoidResult App::createRenderTargets()
    {
        // レンダーターゲットビュー用デスクリプタヒープの作成
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create RTV descriptor heap");

        // レンダーターゲットビューの作成
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize = m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        for (UINT i = 0; i < 2; i++)
        {
            CHECK_HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])),
                Utils::ErrorType::ResourceCreation,
                std::format("Failed to get swap chain buffer {}", i));

            m_device.getDevice()->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += rtvDescriptorSize;
        }

        return {};
    }

    Utils::VoidResult App::createCommandObjects()
    {
        // コマンドアロケータの作成
        CHECK_HR(m_device.getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)),
            Utils::ErrorType::ResourceCreation, "Failed to create command allocator");

        // コマンドリストの作成
        CHECK_HR(m_device.getDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)),
            Utils::ErrorType::ResourceCreation, "Failed to create command list");

        // コマンドリストは作成時に記録状態になっているので、一度クローズする
        m_commandList->Close();

        return {};
    }

    Utils::VoidResult App::createSyncObjects()
    {
        // 同期用フェンスの作成
        CHECK_HR(m_device.getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
            Utils::ErrorType::ResourceCreation, "Failed to create fence");

        m_fenceValue = 1;

        // フェンスイベントの作成
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK_CONDITION(m_fenceEvent != nullptr, Utils::ErrorType::ResourceCreation,
            "Failed to create fence event");

        return {};
    }

    void App::update()
    {
        // 現在は何もしない
        // TODO: ゲームロジックやアニメーションの更新処理を記述
    }

    void App::render()
    {
        // コマンドリストの記録開始
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        // リソースバリア：バックバッファをレンダーターゲット状態に変更
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);

        // レンダーターゲットの設定
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize = m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        rtvHandle.ptr += m_frameIndex * rtvDescriptorSize;

        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // 画面クリア（濃い青色で塗りつぶし）
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // ビューポートとシザー矩形を設定
        const auto [clientWidth, clientHeight] = m_window.getClientSize();
        D3D12_VIEWPORT viewport{};
        viewport.TopLeftX = 0.0f;
        viewport.TopLeftY = 0.0f;
        viewport.Width = static_cast<float>(clientWidth);
        viewport.Height = static_cast<float>(clientHeight);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;

        D3D12_RECT scissorRect{};
        scissorRect.left = 0;
        scissorRect.top = 0;
        scissorRect.right = clientWidth;
        scissorRect.bottom = clientHeight;

        m_commandList->RSSetViewports(1, &viewport);
        m_commandList->RSSetScissorRects(1, &scissorRect);

        // 三角形を描画
        m_triangleRenderer.render(m_commandList.Get());

        // リソースバリア：バックバッファを表示状態に戻す
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        m_commandList->ResourceBarrier(1, &barrier);

        // コマンドリストの記録終了
        m_commandList->Close();

        // コマンドリストの実行
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // 画面に表示
        m_swapChain->Present(1, 0);

        // フレーム完了待ち
        waitForPreviousFrame();
    }

    void App::waitForPreviousFrame()
    {
        const UINT64 fence = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fence);
        m_fenceValue++;

        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }

    void App::cleanup()
    {
        // GPUの処理完了を待つ
        waitForPreviousFrame();

        // フェンスイベントのクリーンアップ
        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        Utils::log_info("DirectX 12 resources cleaned up.");
    }

    void App::onWindowResize(int width, int height)
    {
        Utils::log_info(std::format("Window resized: {}x{}", width, height));

        // TODO: スワップチェーンのリサイズ処理を実装
        // 現在は何もしない
    }

    void App::onWindowClose()
    {
        Utils::log_info("Window close requested.");
    }
}