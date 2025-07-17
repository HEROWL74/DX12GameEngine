// src/Core/App.cpp
#include "App.hpp"
#include <format>

namespace Engine::Core
{
    Utils::VoidResult App::initialize(HINSTANCE hInstance, int nCmdShow)
    {
        Utils::log_info("Initializing Game Engine...");

        // �E�B���h�E�̍쐬
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

        // �E�B���h�E�C�x���g�̃R�[���o�b�N��ݒ�
        m_window.setResizeCallback([this](int width, int height) {
            onWindowResize(width, height);
            });

        m_window.setCloseCallback([this]() {
            onWindowClose();
            });

        m_window.show(nCmdShow);

        // DirectX 12�̏�����
        auto d3dResult = initD3D();
        if (!d3dResult)
        {
            Utils::log_error(d3dResult.error());
            return std::unexpected(d3dResult.error());
        }

        // ���̓V�X�e���̏�����
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

        // ���C�����[�v
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

        // �f�o�C�X�̏�����
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

        // �e�R���|�[�l���g�����Ԃɏ�����
        auto queueResult = createCommandQueue();
        if (!queueResult) return queueResult;

        auto swapChainResult = createSwapChain();
        if (!swapChainResult) return swapChainResult;

        auto renderTargetResult = createRenderTargets();
        if (!renderTargetResult) return renderTargetResult;

        // �[�x�o�b�t�@�쐬
        auto depthStencilResult = createDepthStencilBuffer();
        if (!depthStencilResult) return depthStencilResult;

        auto commandResult = createCommandObjects();
        if (!commandResult) return commandResult;

        auto syncResult = createSyncObjects();
        if (!syncResult) return syncResult;

        auto sceneResult = m_scene.initialize(&m_device);
        if (!sceneResult) {
            Utils::log_error(sceneResult.error());
            return sceneResult;
        }

        // **�f�o�C�X���m���ɏ���������Ă��邩�`�F�b�N**
        if (!m_device.isValid()) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::DeviceCreation, "Device is not valid after initialization"));
        }


        //�O�p�`�I�u�W�F�N�g
        auto* triangleObject = m_scene.createGameObject("Triangle");
        triangleObject->getTransform()->setPosition(Math::Vector3(-2.0f, 0.0f, 0.0f));
        auto* triangleRender = triangleObject->addComponent<Graphics::RenderComponent>(Graphics::RenderableType::Triangle);
        auto triangleInitResult = triangleRender->initialize(&m_device);
        if (!triangleInitResult) {
            Utils::log_error(triangleInitResult.error());
            return triangleInitResult;
        }


        // �����̃I�u�W�F�N�g
        auto* cubeObject = m_scene.createGameObject("Cube");
        cubeObject->getTransform()->setPosition(Math::Vector3(2.0f, 0.0f, 0.0f));
        auto* cubeRender = cubeObject->addComponent<Graphics::RenderComponent>(Graphics::RenderableType::Cube);
        auto cubeInitResult = cubeRender->initialize(&m_device);
        if (!cubeInitResult) {
            Utils::log_error(cubeInitResult.error());
            return cubeInitResult;
        }

        // �����̗����̂��쐬�i�f���p�j
        for (int i = 0; i < 3; ++i)
        {
            auto* extraCube = m_scene.createGameObject("Cube" + std::to_string(i + 2));
            extraCube->getTransform()->setPosition(Math::Vector3(0.0f, 2.0f * (i + 1), -3.0f));
            extraCube->getTransform()->setScale(Math::Vector3(0.5f, 0.5f, 0.5f));
            auto* extraRender = extraCube->addComponent<Graphics::RenderComponent>(Graphics::RenderableType::Cube);
            auto extraInitResult = extraRender->initialize(&m_device);
            if (!extraInitResult) {
                Utils::log_warning("Failed to initialize extra cube renderer");
            }
        }

        // �J�����̏�����
        const auto [clientWidth, clientHeight] = m_window.getClientSize();
        m_camera.setPerspective(45.0f, static_cast<float>(clientWidth) / clientHeight, 0.1f, 100.0f);
        m_camera.setPosition({ 0.0f, 0.0f, 8.0f });
        m_camera.lookAt({ 0.0f, 0.0f, 0.0f });

        // �J�����R���g���[���[�̏�����
        m_cameraController = std::make_unique<Graphics::FPSCameraController>(&m_camera);
        m_cameraController->setMovementSpeed(5.0f);
        m_cameraController->setMouseSensitivity(0.1f);



        //�V�[�����J�n
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

        // �}�E�X�𑊑΃��[�h�ɐݒ�iFPS�X�^�C���j
        #ifdef _DEBUG
        inputManager->setRelativeMouseMode(true);
        #else
        inputManager->setRelativeMouseMode(true);
        #endif

        inputManager->setMouseSensitivity(0.1f);  // ���x��������

        // �ȉ��͊����̃R�[�h�̂܂�
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
        // �����_�[�^�[�Q�b�g�r���[�p�f�X�N���v�^�q�[�v�̍쐬
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create RTV descriptor heap");

        // �����_�[�^�[�Q�b�g�r���[�̍쐬
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
        // �R�}���h�A���P�[�^�[�̍쐬
        CHECK_HR(m_device.getDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)),
            Utils::ErrorType::ResourceCreation, "Failed to create command allocator");

        // �R�}���h���X�g�̍쐬
        CHECK_HR(m_device.getDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)),
            Utils::ErrorType::ResourceCreation, "Failed to create command list");

        // �R�}���h���X�g�͍쐬���ɋL�^��ԂɂȂ��Ă���̂ŁA��x�N���[�Y����
        m_commandList->Close();

        return {};
    }

    //�[�x�X�e���V���o�b�t�@�̍쐬
    Utils::VoidResult App::createDepthStencilBuffer()
    {
        const auto [clientWidth, clientHeight] = m_window.getClientSize();

        //�[�x�X�e���V���o�b�t�@�p�̃q�[�v�쐬
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
        dsvHeapDesc.NumDescriptors = 1;                          //���̃q�[�v�ɉ���DSV�����邩
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;       //���̃q�[�v�͉��p�H
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;     //���̃q�[�v�ɓ��ʂȎg���������邩�H

        CHECK_HR(m_device.getDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_dsvHeap)),
            Utils::ErrorType::ResourceCreation, "Failed to create DSV descriptor heap");

        //�[�x�X�e���V���o�b�t�@���쐬
        D3D12_CLEAR_VALUE depthOptimizedClearValue{};
        depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT; //�o�b�t�@�̃t�H�[�}�b�g�w��E32bit�̕��������_�`���̐[�x�o�b�t�@
        depthOptimizedClearValue.DepthStencil.Depth = 1.0f;      //Z�o�b�t�@���ǂ�Ȓl�ŃN���A�i�������j���邩�H�v
        depthOptimizedClearValue.DepthStencil.Stencil = 0;       //�X�e���V���l�̏����l�i�}�X�N�`��ȂǂŎg���ԍ��j

        D3D12_HEAP_PROPERTIES heapProps{};                         
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;                   //GPU�̂��߂̃��������g�������Ƃ��Ɏw��
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;//���ʂȖړI���Ȃ�����UNKNOWN�ɐݒ�
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN; //���ʂȖړI���Ȃ�����UNKNOWN�ɐݒ�
        heapProps.CreationNodeMask = 1;                             //�ǂ�GPU�m�[�h�ł��̃��\�[�X����邩 
        heapProps.VisibleNodeMask = 1;                              //�ǂ̃m�[�h����A�N�Z�X�\�ɂ��邩

        D3D12_RESOURCE_DESC depthStencilDesc{};
        depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; //���\�[�X�̎�ނ� 2D �e�N�X�`��
        depthStencilDesc.Alignment = 0;                                  //�A���C�����g��0��OK�i�����ōœK�Ȓl���ݒ肳���j
        depthStencilDesc.Width = clientWidth;                            //�[�x�o�b�t�@�̉𑜓x�i�����E�����j
        depthStencilDesc.Height = clientHeight;                          //�[�x�o�b�t�@�̉𑜓x�i���E�������j
        depthStencilDesc.DepthOrArraySize = 1;                           //1���̃e�N�X�`���iDepth=1�j
        depthStencilDesc.MipLevels = 1;                                  //�~�b�v�}�b�v�͎g��Ȃ��iZ�o�b�t�@�ɂ͕s�v�j
        depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;                 //Z�o�b�t�@�̌`���i32bit ���������_�̐[�x�j
        depthStencilDesc.SampleDesc.Count = 1;                           //�}���`�T���v�����O�iMSAA�j�̐ݒ�
        depthStencilDesc.SampleDesc.Quality = 0;                         //�}���`�T���v�����O�iMSAA�j�̐ݒ�
        depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;          //�������C�A�E�g��OK
        depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;//���̃��\�[�X�� �u�[�x�o�b�t�@�Ƃ��Ďg���܂��v �Ɩ���

        CHECK_HR(m_device.getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &depthStencilDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            IID_PPV_ARGS(&m_depthStencilBuffer)),
            Utils::ErrorType::ResourceCreation, "Failed to create depth stencil buffer");

        //�[�x�X�e���V���r���[�ݒ�
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
        // �����p�t�F���X�̍쐬
        CHECK_HR(m_device.getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)),
            Utils::ErrorType::ResourceCreation, "Failed to create fence");

        m_fenceValue = 1;

        // �t�F���X�C�x���g�̍쐬
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        CHECK_CONDITION(m_fenceEvent != nullptr, Utils::ErrorType::ResourceCreation,
            "Failed to create fence event");

        return {};
    }

    void App::update()
    {
        // �f���^�^�C���̌v�Z
        updateDeltaTime();

        // ���͏���
        processInput();

        //�V�[���̍X�V
        m_scene.update(m_deltaTime);
        m_scene.lateUpdate(m_deltaTime);

        // **�f���p�A�j���[�V���� - GameObject���g�p**
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

        // �ǉ��̗����̂��A�j���[�V����
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
        // �R�}���h���X�g�̋L�^�J�n
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        // ���\�[�X�o���A: �o�b�N�o�b�t�@�������_�[�^�[�Q�b�g��ԂɕύX
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);

        // �����_�[�^�[�Q�b�g�r���[�Ɛ[�x�X�e���V���r���[�̃n���h���擾
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        const UINT rtvDescriptorSize = m_device.getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        rtvHandle.ptr += m_frameIndex * rtvDescriptorSize;

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();

        // �����_�[�^�[�Q�b�g�Ɛ[�x�X�e���V����ݒ�
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

        // ��ʃN���A�i�Z���F�œh��Ԃ��j
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // �[�x�o�b�t�@���N���A
        m_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

        // �r���[�|�[�g�ƃV�U�[��`��ݒ�
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

        

        m_scene.render(m_commandList.Get(), m_camera, m_frameIndex);

        // ���\�[�X�o���A: �o�b�N�o�b�t�@��\����Ԃɖ߂�
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        m_commandList->ResourceBarrier(1, &barrier);

        // �R�}���h���X�g�̋L�^�I��
        m_commandList->Close();

        // �R�}���h���X�g�̎��s
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // ��ʂɕ\��
        m_swapChain->Present(1, 0);

        // �t���[�������҂�
        waitForPreviousFrame();
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

        // �t���[�����[�g�̌v�Z
        m_frameCount++;
        m_frameTimeAccumulator += m_deltaTime;
        if (m_frameTimeAccumulator >= 1.0f)
        {
            m_currentFPS = m_frameCount / m_frameTimeAccumulator;
            m_frameCount = 0;
            m_frameTimeAccumulator = 0.0f;

            // FPS���E�B���h�E�^�C�g���ɕ\��
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

        // WASD�L�[�ŃJ�����ړ�
        bool forward = inputManager->isKeyDown(Input::KeyCode::W);
        bool backward = inputManager->isKeyDown(Input::KeyCode::S);
        bool left = inputManager->isKeyDown(Input::KeyCode::A);
        bool right = inputManager->isKeyDown(Input::KeyCode::D);
        bool up = inputManager->isKeyDown(Input::KeyCode::Space);
        bool down = inputManager->isKeyDown(Input::KeyCode::LeftShift);

        // �J�����R���g���[���[�Ɉړ�����n��
        m_cameraController->processKeyboard(forward, backward, left, right, up, down, m_deltaTime);

        // �}�E�X�ɂ�鎋�_�ύX
        if (inputManager->getMouseState().isRelativeMode)
        {
            int deltaX = inputManager->getMouseDeltaX();
            int deltaY = inputManager->getMouseDeltaY();
            m_cameraController->processMouseMovement(static_cast<float>(deltaX), static_cast<float>(deltaY));
        }
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
        // GPU�̏���������҂�
        waitForPreviousFrame();

        // �t�F���X�C�x���g�̃N���[���A�b�v
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

        // �J�����̃A�X�y�N�g����X�V
        if (height > 0)
        {
            m_camera.updateAspect(static_cast<float>(width) / height);
        }

        // TODO: �X���b�v�`�F�[���̃��T�C�Y����������
        // ���݂͉������Ȃ�
    }

    void App::onWindowClose()
    {
        Utils::log_info("Window close requested.");
        PostQuitMessage(0);
    }

    void App::onKeyPressed(Input::KeyCode key)
    {
        // �f�o�b�O�p�F�L�[���͂̃��O�o��
        if (key == Input::KeyCode::Escape)
        {
            Utils::log_info("Escape key pressed - requesting exit");
            PostQuitMessage(0);
        }
        else if (key == Input::KeyCode::F1)
        {
            // F1�L�[�Ń}�E�X���΃��[�h�̐؂�ւ�
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
        // ���݂͉������Ȃ�
    }

    void App::onMouseMove(int x, int y, int deltaX, int deltaY)
    {
        // �}�E�X�ړ��̏�����processInput()�ōs��
    }

    void App::onMouseButtonPressed(Input::MouseButton button, int x, int y)
    {
        if (button == Input::MouseButton::Left)
        {
            Utils::log_info(std::format("Left mouse button pressed at ({}, {})", x, y));
        }
    }
}