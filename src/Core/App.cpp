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

        // 入力システムの初期化
        auto inputResult = initializeInput();
        if (!inputResult)
        {
            Utils::log_error(inputResult.error());
            return std::unexpected(inputResult.error());
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

        // デバイス初期化
        Graphics::DeviceSettings deviceSettings{
            .enableDebugLayer = true,
            .enableGpuValidation = false,
            .minFeatureLevel = D3D_FEATURE_LEVEL_11_0,
            .preferHighPerformanceAdapter = true
        };

        auto deviceResult = m_device.initialize(deviceSettings);
        if (!deviceResult) return deviceResult;

        // DirectXリソース基本初期化
        auto queueResult = createCommandQueue();
        if (!queueResult) return queueResult;

        auto swapChainResult = createSwapChain();
        if (!swapChainResult) return swapChainResult;

        auto renderTargetResult = createRenderTargets();
        if (!renderTargetResult) return renderTargetResult;

        auto depthStencilResult = createDepthStencilBuffer();
        if (!depthStencilResult) return depthStencilResult;

        auto commandResult = createCommandObjects();
        if (!commandResult) return commandResult;

        auto syncResult = createSyncObjects();
        if (!syncResult) return syncResult;

        // ImGui初期化
        auto imguiResult = m_imguiManager.initialize(&m_device, m_window.getHandle(), m_commandQueue.Get());
        if (!imguiResult) return imguiResult;

        m_window.setImGuiManager(&m_imguiManager);

        // ShaderManager初期化
        Utils::log_info("Initializing ShaderManager FIRST...");
        m_shaderManager = std::make_unique<Graphics::ShaderManager>();

        auto shaderManagerResult = m_shaderManager->initialize(&m_device);
        if (!shaderManagerResult) {
            Utils::log_error(shaderManagerResult.error());
            return shaderManagerResult;
        }

        if (!m_shaderManager || !m_shaderManager.get()) {
            Utils::log_warning("ShaderManager pointer is invalid after initialization");
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null after initialization"));
        }

        Utils::log_info("ShaderManager initialization completed successfully");

        // TextureManager初期化
        auto textureManagerResult = m_textureManager.initialize(&m_device);
        if (!textureManagerResult) return textureManagerResult;

        // MaterialManager初期化
        auto materialManagerResult = m_materialManager.initialize(&m_device);
        if (!materialManagerResult) return materialManagerResult;

        // Scene初期化
        auto sceneResult = m_scene.initialize(&m_device);
        if (!sceneResult) return sceneResult;

        // ShaderManagerポインタ取得
        if (!m_shaderManager || !m_shaderManager.get()) {
            Utils::log_warning("ShaderManager became null before GameObject creation");
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager is null before GameObject creation"));
        }

        Graphics::ShaderManager* shaderMgrPtr = m_shaderManager.get();
        if (!shaderMgrPtr) {
            Utils::log_warning("Failed to get valid ShaderManager pointer");
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ShaderManager pointer is null"));
        }

        // Triangle作成
        auto* triangleObject = m_scene.createGameObject("Triangle");
        triangleObject->getTransform()->setPosition(Math::Vector3(-2.0f, 0.0f, 0.0f));
        auto* triangleRender = triangleObject->addComponent<Graphics::RenderComponent>(Graphics::RenderableType::Triangle);

        auto triangleMaterial = m_materialManager.createMaterial("Triangle_Material");
        if (triangleMaterial) {
            Graphics::MaterialProperties triangleProps;
            triangleProps.albedo = Math::Vector3(1.0f, 0.0f, 0.0f);
            triangleProps.metallic = 0.0f;
            triangleProps.roughness = 0.5f;
            triangleMaterial->setProperties(triangleProps);
            triangleRender->setMaterial(triangleMaterial);
        }

        triangleRender->setMaterialManager(&m_materialManager);
        auto triangleInitResult = triangleRender->initialize(&m_device, shaderMgrPtr);
        if (!triangleInitResult) {
            Utils::log_error(triangleInitResult.error());
            return triangleInitResult;
        }

        // Cube作成
        auto* cubeObject = m_scene.createGameObject("Cube");
        cubeObject->getTransform()->setPosition(Math::Vector3(2.0f, 0.0f, 0.0f));
        auto* cubeRender = cubeObject->addComponent<Graphics::RenderComponent>(Graphics::RenderableType::Cube);

        auto cubeMaterial = m_materialManager.createMaterial("Cube_Material");
        if (cubeMaterial) {
            Graphics::MaterialProperties cubeProps;
            cubeProps.albedo = Math::Vector3(0.0f, 1.0f, 0.0f);
            cubeProps.metallic = 0.0f;
            cubeProps.roughness = 0.3f;
            cubeMaterial->setProperties(cubeProps);
            cubeRender->setMaterial(cubeMaterial);
        }

        cubeRender->setMaterialManager(&m_materialManager);
        auto cubeInitResult = cubeRender->initialize(&m_device, shaderMgrPtr);
        if (!cubeInitResult) {
            Utils::log_error(cubeInitResult.error());
            return cubeInitResult;
        }

        // 追加キューブ作成
        for (int i = 0; i < 3; ++i)
        {
            auto* extraCube = m_scene.createGameObject("Cube" + std::to_string(i + 2));
            extraCube->getTransform()->setPosition(Math::Vector3(0.0f, 2.0f * (i + 1), -3.0f));
            extraCube->getTransform()->setScale(Math::Vector3(0.5f, 0.5f, 0.5f));
            auto* extraRender = extraCube->addComponent<Graphics::RenderComponent>(Graphics::RenderableType::Cube);

            auto extraMaterial = m_materialManager.createMaterial("ExtraCube" + std::to_string(i) + "_Material");
            if (extraMaterial) {
                Graphics::MaterialProperties extraProps;
                switch (i) {
                case 0: extraProps.albedo = Math::Vector3(0.0f, 0.0f, 1.0f); break;
                case 1: extraProps.albedo = Math::Vector3(1.0f, 1.0f, 0.0f); break;
                case 2: extraProps.albedo = Math::Vector3(1.0f, 0.0f, 1.0f); break;
                }
                extraProps.metallic = 0.1f;
                extraProps.roughness = 0.4f;
                extraMaterial->setProperties(extraProps);
                extraRender->setMaterial(extraMaterial);
            }

            extraRender->setMaterialManager(&m_materialManager);
            extraRender->initialize(&m_device, shaderMgrPtr);
        }

        // ProjectWindow作成
        m_projectWindow = std::make_unique<UI::ProjectWindow>();
        m_projectWindow->setTextureManager(&m_textureManager);
        m_projectWindow->setMaterialManager(&m_materialManager);
        m_projectWindow->setProjectPath("assets");

        // カメラ初期化
        const auto [clientWidth, clientHeight] = m_window.getClientSize();
        m_camera.setPerspective(45.0f, static_cast<float>(clientWidth) / clientHeight, 0.1f, 100.0f);
        m_camera.setPosition({ 0.0f, 0.0f, 8.0f });
        m_camera.lookAt({ 0.0f, 0.0f, 0.0f });

        m_cameraController = std::make_unique<Graphics::FPSCameraController>(&m_camera);
        m_cameraController->setMovementSpeed(5.0f);
        m_cameraController->setMouseSensitivity(0.1f);

        // UIウィンドウ作成
        m_debugWindow = std::make_unique<UI::DebugWindow>();
        m_hierarchyWindow = std::make_unique<UI::SceneHierarchyWindow>();
        m_inspectorWindow = std::make_unique<UI::InspectorWindow>();

        // UIウィンドウ設定
        m_hierarchyWindow->setScene(&m_scene);
        m_hierarchyWindow->setSelectionChangedCallback([this](Core::GameObject* selectedObject) {
            m_inspectorWindow->setSelectedObject(selectedObject);
            });

        m_scriptManager = std::make_unique<Scripting::ScriptManager>();
        m_scriptManager->initialize();
        m_scriptManager->loadScript("assets/scripts/test.lua");

        //コンテキストメニューのコールバック設定
        m_hierarchyWindow->setCreateObjectCallback([this](UI::PrimitiveType type, const std::string& name) -> Core::GameObject* {
            return createPrimitiveObject(type, name);
            });

        m_hierarchyWindow->setDeleteObjectCallback([this](Core::GameObject* object) {
            deleteGameObject(object);
            });

        // Duplicateコールバックを追加
        m_hierarchyWindow->setDuplicateObjectCallback([this](Core::GameObject* object) -> Core::GameObject* {
            return duplicateGameObject(object);
            });

        // Renameコールバックを追加
        m_hierarchyWindow->setRenameObjectCallback([this](Core::GameObject* object, const std::string& newName) {
            renameGameObject(object, newName);
            });

        m_inspectorWindow->setMaterialManager(&m_materialManager);
        m_inspectorWindow->setTextureManager(&m_textureManager);

        m_scene.start();

        Utils::log_info("DirectX 12 initialization completed successfully!");
        return {};
    }

    Utils::VoidResult App::initializeInput()
    {
        Utils::log_info("Initializing input system...");

        auto* inputManager = m_window.getInputManager();
        if (!inputManager)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "InputManager not available"));
        }

        // マウスを相対モードに設定（FPSスタイル）
        #ifdef _DEBUG
        inputManager->setRelativeMouseMode(false);
        #else
        inputManager->setRelativeMouseMode(true);
        #endif

        inputManager->setMouseSensitivity(0.1f);  // 感度を下げる

        // 以下は既存のコードのまま
        inputManager->setKeyPressedCallback([this](Input::KeyCode key) {
            onKeyPressed(key);
            });

        inputManager->setKeyReleasedCallback([this](Input::KeyCode key) {
            onKeyReleased(key);
            });

        inputManager->setMouseMoveCallback([this](int x, int y, int deltaX, int deltaY) {
            onMouseMove(x, y, deltaX, deltaY);
            });

        inputManager->setMouseButtonPressedCallback([this](Input::MouseButton button, int x, int y) {
            onMouseButtonPressed(button, x, y);
            });

        Utils::log_info("Input system initialized successfully!");
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
        // コマンドアロケーターの作成
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

    //深度ステンシルバッファの作成
    Utils::VoidResult App::createDepthStencilBuffer()
    {
        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        //深度ステンシルバッファ用のヒープ作成
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;                          //このヒープに何個のDSVを入れるか
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;       //このヒープは何用？
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;     //このヒープに特別な使い方をするか？

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create DSV descriptor heap");

        //深度ステンシルバッファを作成
        D3D12_CLEAR_VALUE depthOptimizedClearValue{};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT; //バッファのフォーマット指定・32bitの浮動小数点形式の深度バッファ
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;      //Zバッファをどんな値でクリア（初期化）するか？」
        depthOptimizedClearValue.DepthStencil.Stencil = 0;       //ステンシル値の初期値（マスク描画などで使う番号）

        D3D12_HEAP_PROPERTIES heapProps{};                         
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;                   //GPUのためのメモリを使いたいときに指定
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;//特別な目的がない限りUNKNOWNに設定
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; //特別な目的がない限りUNKNOWNに設定
        heapProps.CreationNodeMask = 1;                             //どのGPUノードでこのリソースを作るか 
        heapProps.VisibleNodeMask = 1;                              //どのノードからアクセス可能にするか

        D3D12_RESOURCE_DESC depthStencilDesc{};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //リソースの種類は 2D テクスチャ
        depthStencilDesc.Alignment = 0;                                  //アライメントは0でOK（自動で最適な値が設定される）
        depthStencilDesc.Width = clientWidth;                            //深度バッファの解像度（幅←・高さ）
        depthStencilDesc.Height = clientHeight;                          //深度バッファの解像度（幅・高さ←）
        depthStencilDesc.DepthOrArraySize = 1;                           //1枚のテクスチャ（Depth=1）
        depthStencilDesc.MipLevels = 1;                                  //ミップマップは使わない（Zバッファには不要）
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;                 //Zバッファの形式（32bit 浮動小数点の深度）
        depthStencilDesc.SampleDesc.Count = 1;                           //マルチサンプリング（MSAA）の設定
        depthStencilDesc.SampleDesc.Quality = 0;                         //マルチサンプリング（MSAA）の設定
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;          //自動レイアウトでOK
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//このリソースを 「深度バッファとして使います」 と明示

        CHECK_HR(m_device.getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencilBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create depth stencil buffer");

        //深度ステンシルビュー設定
        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
        dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
        dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        dsvDesc.Texture2D.MipSlice = 0;

        m_device.getDevice()->CreateDepthStencilView(
            m_depthStencilBuffer.Get(),
            &dsvDesc,
            m_dsvHeap->GetCPUDescriptorHandleForHeapStart()
        );

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
        // デルタタイムの計算
        updateDeltaTime();

        // 入力処理
        processInput();

        //シーンの更新
        m_scene.update(m_deltaTime);
        m_scene.lateUpdate(m_deltaTime);

        //ImGui情報を更新
        m_debugWindow->setFPS(m_currentFPS);
        m_debugWindow->setFrameTime(m_deltaTime);
        m_debugWindow->setObjectCount(m_scene.getGameObjects().size());

        // デモ用アニメーション
        auto* triangleObject = m_scene.findGameObject("Triangle");
        if (triangleObject)
        {
            static float triangleRotation = 0.0f;
            triangleRotation += 30.0f * m_deltaTime;
            triangleObject->getTransform()->setRotation(Math::Vector3(0.0f, triangleRotation, 0.0f));
        }

        auto* cubeObject = m_scene.findGameObject("Cube");
        if (cubeObject)
        {
            static float cubeRotation = 0.0f;
            cubeRotation += 45.0f * m_deltaTime;
            cubeObject->getTransform()->setRotation(Math::Vector3(cubeRotation, cubeRotation * 0.7f, 0.0f));
        }

        // 追加の立方体をアニメーション
        for (int i = 0; i < 3; ++i)
        {
            auto* extraCube = m_scene.findGameObject("Cube" + std::to_string(i + 2));
            if (extraCube)
            {
                static float extraRotation = 0.0f;
                extraRotation += (60.0f + i * 20.0f) * m_deltaTime;
                extraCube->getTransform()->setRotation(Math::Vector3(0.0f, extraRotation, 0.0f));
            }
        }
    }
    void App::render()
    {
        Utils::log_info("App::render() called");

        // リサイズ状態確認
        bool isResizing = false;
        {
            std::lock_guard<std::mutex> lock(m_resizeMutex);
            isResizing = m_isResizing;
        }

        if (isResizing)
        {
            Utils::log_info("Currently resizing, checking ImGui state");
        }

        // ImGuiの初期化状態確認
        if (!m_imguiManager.isInitialized())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "ImGuiManager not initialized"));
            return;
        }

        // ImGuiコンテキスト確認
        ImGuiContext* context = m_imguiManager.getContext();
        if (!context)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "ImGui context is null"));
            return;
        }

        Utils::log_info("Starting ImGui newFrame");
        try
        {
            m_imguiManager.newFrame();
            Utils::log_info("ImGui newFrame completed");
        }
        catch (...)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Exception in ImGui newFrame"));
            return;
        }

        Utils::log_info("Drawing ImGui windows");
        try
        {
            m_debugWindow->draw();
            m_hierarchyWindow->draw();
            m_inspectorWindow->draw();
            m_projectWindow->draw();
            Utils::log_info("ImGui windows drawn successfully");
        }
        catch (...)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Exception in ImGui window drawing"));
            return;
        }

        // リサイズ中の処理
        {
            std::lock_guard<std::mutex> lock(m_resizeMutex);
            if (m_isResizing)
            {
                Utils::log_info("Resizing: calling ImGui::Render and returning");
                try
                {
                    ImGui::Render();
                    Utils::log_info("ImGui::Render completed during resize");
                }
                catch (...)
                {
                    Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Exception in ImGui::Render during resize"));
                }
                return;
            }
        }

        // 3Dレンダリング部分（既存コード）
        Utils::log_info("Starting 3D rendering");

        if (!m_renderTargets[m_frameIndex] || !m_commandList || !m_swapChain)
        {
            Utils::log_warning("Render resources not ready, skipping frame");
            return;
        }

        // コマンドリストの記録開始
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        // リソースバリア: バックバッファをレンダーターゲット状態に変更
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);

        // レンダーターゲットビューと深度ステンシルビューのハンドル取得
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize = m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        rtvHandle.ptr += m_frameIndex * rtvDescriptorSize;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

        // レンダーターゲットと深度ステンシル設定
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // 画面クリア（深青色で塗りつぶし）
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

        if (m_renderTargets[m_frameIndex])
        {
            m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
        }

        if (m_depthStencilBuffer)
        {
            m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        }

        // ビューポートとシザー矩形を動的に設定（リサイズ対応）
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

        // シーンのレンダリング
        Utils::log_info("Rendering scene");
        m_scene.render(m_commandList.Get(), m_camera, m_frameIndex);

        // ImGui描画
        Utils::log_info("Rendering ImGui");
        try
        {
            m_imguiManager.render(m_commandList.Get());
            Utils::log_info("ImGui rendering completed");
        }
        catch (...)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Exception in ImGui render"));
        }

        // リソースバリア: バックバッファを表示状態に戻す
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        m_commandList->ResourceBarrier(1, &barrier);

        // コマンドリストの記録終了
        m_commandList->Close();

        // コマンドリストの実行
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // 画面に表示
        HRESULT hr = m_swapChain->Present(1, 0);
        if (FAILED(hr))
        {
            Utils::log_warning("Present failed, possibly due to resize");
            return;
        }

        // フレーム待機
        waitForPreviousFrame();

        Utils::log_info("App::render() completed");
    }

    void App::updateDeltaTime()
    {
        auto currentTime = std::chrono::high_resolution_clock::now();
        if (m_lastFrameTime.time_since_epoch().count() == 0)
        {
            m_lastFrameTime = currentTime;
        }

        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(currentTime - m_lastFrameTime);
        m_deltaTime = duration.count() / 1000000.0f;
        m_lastFrameTime = currentTime;

        // フレームレートの計算
        m_frameCount++;
        m_frameTimeAccumulator += m_deltaTime;
        if (m_frameTimeAccumulator >= 1.0f)
        {
            m_currentFPS = m_frameCount / m_frameTimeAccumulator;
            m_frameCount = 0;
            m_frameTimeAccumulator = 0.0f;

            // FPSをウィンドウタイトルに表示
            std::wstring title = std::format(L"DX12 Game Engine - FPS: {:.1f}", m_currentFPS);
            m_window.setTitle(title);
        }
    }

    void App::processInput()
    {
        auto* inputManager = m_window.getInputManager();
        if (!inputManager || !m_cameraController)
        {
            return;
        }

        // ImGuiが入力をキャプチャしている場合は、ゲーム入力を処理しない
        ImGuiIO& io = ImGui::GetIO();

        // キーボード入力がImGuiにキャプチャされている場合はスキップ
        if (!io.WantCaptureKeyboard)
        {
            // WASDキーでカメラ移動
            bool forward = inputManager->isKeyDown(Input::KeyCode::W);
            bool backward = inputManager->isKeyDown(Input::KeyCode::S);
            bool left = inputManager->isKeyDown(Input::KeyCode::A);
            bool right = inputManager->isKeyDown(Input::KeyCode::D);
            bool up = inputManager->isKeyDown(Input::KeyCode::Space);
            bool down = inputManager->isKeyDown(Input::KeyCode::LeftShift);

            // カメラコントローラーに移動情報を渡す
            m_cameraController->processKeyboard(forward, backward, left, right, up, down, m_deltaTime);
        }

        // マウス入力がImGuiにキャプチャされている場合はスキップ
        if (!io.WantCaptureMouse)
        {
            // マウスによる視点変更
            if (inputManager->getMouseState().isRelativeMode)
            {
                int deltaX = inputManager->getMouseDeltaX();
                int deltaY = inputManager->getMouseDeltaY();
                m_cameraController->processMouseMovement(static_cast<float>(deltaX), static_cast<float>(deltaY));
            }
        }
    }

    void App::waitForPreviousFrame()
    {
        // 必要なオブジェクトが初期化されているかチェック
        if (!m_commandQueue || !m_fence || !m_fenceEvent)
        {
            Utils::log_warning("DirectX objects not initialized in waitForPreviousFrame");
            return;
        }

        const UINT64 fence = m_fenceValue;
        HRESULT hr = m_commandQueue->Signal(m_fence.Get(), fence);
        if (FAILED(hr))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "Failed to signal fence", hr));
            return;
        }

        m_fenceValue++;

        if (m_fence->GetCompletedValue() < fence)
        {
            hr = m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            if (FAILED(hr))
            {
                Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                    "Failed to set fence event", hr));
                return;
            }
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
        Utils::log_info(std::format("App::onWindowResize called: {}x{}", width, height));

        if (width <= 0 || height <= 0)
        {
            Utils::log_warning(std::format("Invalid resize dimensions: {}x{}", width, height));
            return;
        }

        // DirectX 12が初期化されていない場合
        if (!m_commandQueue || !m_swapChain || !m_fence)
        {
            Utils::log_info("DirectX 12 not initialized yet");
            if (height > 0)
            {
                m_camera.updateAspect(static_cast<float>(width) / height);
            }
            return;
        }

        Utils::log_info("Starting safe DirectX resize process...");

        try
        {
            // 1. ImGuiの安全なシャットダウン
            if (m_imguiManager.isInitialized())
            {
                Utils::log_info("Safely shutting down ImGui for resize");
                m_imguiManager.shutdown();
            }

            // 2. 完全なGPU同期
            Utils::log_info("Complete GPU synchronization");
            waitForPreviousFrame();

            // 追加同期
            const UINT64 flushFence = m_fenceValue;
            m_commandQueue->Signal(m_fence.Get(), flushFence);
            m_fenceValue++;

            if (m_fence->GetCompletedValue() < flushFence)
            {
                m_fence->SetEventOnCompletion(flushFence, m_fenceEvent);
                WaitForSingleObject(m_fenceEvent, INFINITE);
            }

            // 3. リソースクリア
            Utils::log_info("Clearing DirectX resources");
            for (UINT i = 0; i < 2; i++)
            {
                if (m_renderTargets[i])
                {
                    m_renderTargets[i].Reset();
                }
            }

            if (m_depthStencilBuffer)
            {
                m_depthStencilBuffer.Reset();
            }

            // 4. スワップチェインリサイズ
            Utils::log_info("Resizing swap chain");
            HRESULT hr = m_swapChain->ResizeBuffers(2, width, height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);

            if (FAILED(hr))
            {
                Utils::log_error(Utils::make_error(Utils::ErrorType::SwapChainCreation,
                    std::format("Failed to resize swap chain: 0x{:08x}", static_cast<unsigned>(hr)), hr));
                return;
            }

            // 5. リソース再作成
            auto renderTargetResult = createRenderTargets();
            if (!renderTargetResult)
            {
                Utils::log_error(renderTargetResult.error());
                return;
            }

            auto depthStencilResult = createDepthStencilBuffer();
            if (!depthStencilResult)
            {
                Utils::log_error(depthStencilResult.error());
                return;
            }

            // 6. フレームインデックス更新
            m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

            // 7. カメラ更新
            m_camera.updateAspect(static_cast<float>(width) / height);

            // 8. ImGuiの安全な再初期化
            Utils::log_info("Safely reinitializing ImGui after resize");
            auto imguiResult = m_imguiManager.initialize(&m_device, m_window.getHandle(), m_commandQueue.Get());
            if (!imguiResult)
            {
                Utils::log_error(imguiResult.error());
                Utils::log_warning("ImGui reinitialization failed, continuing without ImGui");
            }
            else
            {
                m_window.setImGuiManager(&m_imguiManager);
                Utils::log_info("ImGui reinitialized successfully");
            }

            Utils::log_info("DirectX resize completed successfully");
        }
        catch (const std::exception& e)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Exception during resize: {}", e.what())));
        }
        catch (...)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Unknown exception during resize"));
        }
    }


    void App::onWindowClose()
    {
        Utils::log_info("Window close requested.");
        PostQuitMessage(0);
    }

    void App::onKeyPressed(Input::KeyCode key)
    {
        // ImGuiが入力をキャプチャしている場合はスキップ
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureKeyboard)
        {
            return;
        }

        // デバッグ用：キー入力のログ出力
        if (key == Input::KeyCode::Escape)
        {
            Utils::log_info("Escape key pressed - requesting exit");
            PostQuitMessage(0);
        }
        else if (key == Input::KeyCode::F1)
        {
            // F1キーでマウス相対モードの切り替え
            auto* inputManager = m_window.getInputManager();
            if (inputManager)
            {
                bool currentMode = inputManager->getMouseState().isRelativeMode;
                inputManager->setRelativeMouseMode(!currentMode);
                Utils::log_info(std::format("Mouse relative mode: {}", !currentMode ? "ON" : "OFF"));
            }
        }
    }

    void App::onKeyReleased(Input::KeyCode key)
    {
        // 現在は何もしない
    }

    [[maybe_unused]]
    void App::onMouseMove(int x, int y, int deltaX, int deltaY)
    {
        // マウス移動の処理はprocessInput()で行う
    }

    void App::onMouseButtonPressed(Input::MouseButton button, int x, int y)
    {
        if (button == Input::MouseButton::Left)
        {
            Utils::log_info(std::format("Left mouse button pressed at ({}, {})", x, y));
        }
    }

    Core::GameObject* App::createPrimitiveObject(UI::PrimitiveType type, const std::string& name)
    {
        // 新しいGameObjectを作成
        auto* newObject = m_scene.createGameObject(name);
        if (!newObject) return nullptr;

        // デフォルト位置設定
        Math::Vector3 cameraPos = m_camera.getPosition();
        Math::Vector3 cameraForward = m_camera.getForward();
        newObject->getTransform()->setPosition(cameraPos + cameraForward * 3.0f);

        // RenderComponentを追加（コンポーネントは生ポインタで作成）
        Graphics::RenderableType renderType = primitiveToRenderableType(type);
        auto* renderComponent = newObject->addComponent<Graphics::RenderComponent>(renderType);

        if (!renderComponent)
        {
            m_scene.destroyGameObject(newObject);
            return nullptr;
        }

        // マテリアルを作成
        auto material = m_materialManager.createMaterial(name + "_Material");
        if (material)
        {
            Graphics::MaterialProperties props;

            // タイプ別に色を設定
            switch (type)
            {
            case UI::PrimitiveType::Cube:
                props.albedo = Math::Vector3(0.8f, 0.8f, 0.8f);
                break;
            case UI::PrimitiveType::Sphere:
                props.albedo = Math::Vector3(1.0f, 0.5f, 0.5f);
                break;
            case UI::PrimitiveType::Plane:
                props.albedo = Math::Vector3(0.5f, 1.0f, 0.5f);
                break;
            case UI::PrimitiveType::Cylinder:
                props.albedo = Math::Vector3(0.5f, 0.5f, 1.0f);
                break;
            }

            props.metallic = 0.0f;
            props.roughness = 0.5f;
            material->setProperties(props);
            renderComponent->setMaterial(material);
        }

        // MaterialManagerを設定
        renderComponent->setMaterialManager(&m_materialManager);

        // ShaderManagerで初期化
        if (m_shaderManager)
        {
            auto initResult = renderComponent->initialize(&m_device, m_shaderManager.get());
            if (!initResult)
            {
                Utils::log_error(initResult.error());
                m_scene.destroyGameObject(newObject);
                return nullptr;
            }
        }

        Utils::log_info(std::format("Created new object: {}", name));
        return newObject;
    }

    void App::deleteGameObject(Core::GameObject* object)
    {
        if (!object)
        {
            Utils::log_warning("Attempted to delete null object");
            return;
        }

        std::string objectName = object->getName();
        Utils::log_info(std::format("Starting deletion of object: {}", objectName));

        // まずUIの参照をすべてクリア
        if (m_inspectorWindow)
        {
            if (m_inspectorWindow->getSelectedObject() == object)
            {
                m_inspectorWindow->setSelectedObject(nullptr);
            }
        }

        if (m_hierarchyWindow)
        {
            if (m_hierarchyWindow->getSelectedObject() == object)
            {
                m_hierarchyWindow->setSelectedObject(nullptr);
            }
        }

        // ImGuiのコンテキストをクリア（重要）
        //ImGui::SetWindowFocus(nullptr);

        // オブジェクトを削除
        m_scene.destroyGameObject(object);

        // 削除後にnullptrを設定
        object = nullptr;

        Utils::log_info(std::format("Successfully deleted object: {}", objectName));
    }

    Core::GameObject* App::duplicateGameObject(Core::GameObject* original)
    {
        if (!original) return nullptr;

        // オリジナルの情報を取得
        auto* originalRender = original->getComponent<Graphics::RenderComponent>();
        if (!originalRender) return nullptr;

        // 新しい名前を生成
        std::string newName = generateUniqueName(original->getName() + "_Copy");

        // プリミティブタイプを使用して新規作成
        UI::PrimitiveType primitiveType = renderableToPrimitiveType(originalRender->getRenderableType());
        auto* newObject = createPrimitiveObject(primitiveType, newName);

        if (!newObject) return nullptr;

        // Transformをコピー
        auto* originalTransform = original->getTransform();
        auto* newTransform = newObject->getTransform();
        if (originalTransform && newTransform)
        {
            newTransform->setPosition(originalTransform->getPosition() + Math::Vector3(1.0f, 0.0f, 0.0f));
            newTransform->setRotation(originalTransform->getRotation());
            newTransform->setScale(originalTransform->getScale());
        }

        return newObject;
    }


    std::string App::generateUniqueName(const std::string& baseName)
    {
        std::string candidateName = baseName;
        int counter = 1;

        // 同じ名前が存在する限りカウンターを増やす
        while (m_scene.findGameObject(candidateName) != nullptr)
        {
            candidateName = baseName + "_" + std::to_string(counter);
            counter++;

            // 無限ループ防止
            if (counter > 1000)
            {
                candidateName = baseName + "_" + std::to_string(std::time(nullptr));
                break;
            }
        }

        return candidateName;
    }

    // リネーム機能（簡易版）
    void App::renameGameObject(Core::GameObject* object, const std::string& newName)
    {
        if (!object) return;

        std::string oldName = object->getName();
        object->setName(newName);

        Utils::log_info(std::format("Renamed object: {} -> {}", oldName, newName));
    }

    Graphics::RenderableType App::primitiveToRenderableType(UI::PrimitiveType type)
    {
        switch (type)
        {
        case UI::PrimitiveType::Cube:
            return Graphics::RenderableType::Cube;
        case UI::PrimitiveType::Sphere:
            // 現在はCubeのみ実装されているため、将来的に追加
            return Graphics::RenderableType::Cube;
        case UI::PrimitiveType::Plane:
            return Graphics::RenderableType::Triangle; // 仮実装
        case UI::PrimitiveType::Cylinder:
            return Graphics::RenderableType::Cube; // 仮実装
        default:
            return Graphics::RenderableType::Cube;
        }
    }

    UI::PrimitiveType App::renderableToPrimitiveType(Graphics::RenderableType renderType)
    {
        switch (renderType)
        {
        case Graphics::RenderableType::Cube:
            return UI::PrimitiveType::Cube;
        case Graphics::RenderableType::Triangle:
            return UI::PrimitiveType::Plane; // 仮実装
        default:
            return UI::PrimitiveType::Cube;
        }
    }
}

