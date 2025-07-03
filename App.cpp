// App.cpp
#include "src/Core/App.hpp"

namespace Core
{
    bool App::initialize(HINSTANCE hInstance, int nCmdShow) noexcept
    {
        // �E�B���h�E�̏�����
        if (!initWindow(hInstance, nCmdShow)) {
            return false;
        }

        // DirectX 12�̏�����
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
                // ���C�����[�v�F�X�V�ƕ`������s
                update();
                render();
            }
        }

        // �I���O��GPU�̏���������҂�
        waitForPreviousFrame();

        // �t�F���X�C�x���g�̃N���[���A�b�v
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
        // --- �f�o�b�O���C���[�̗L�����i�f�o�b�O�r���hdake�����j ---
#ifdef _DEBUG
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
        }
#endif

        // --- DXGI�t�@�N�g���̍쐬 ---
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory)))) {
            return false;
        }

        // --- D3D12�f�o�C�X�̍쐬 ---
        if (FAILED(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)))) {
            return false;
        }

        // --- �R�}���h�L���[�̍쐬 ---
        D3D12_COMMAND_QUEUE_DESC queueDesc{};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  // ���ڃR�}���h���X�g�p

        if (FAILED(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)))) {
            return false;
        }

        // --- �X���b�v�`�F�[���̍쐬 ---
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc{};
        swapChainDesc.BufferCount = 2;              // �_�u���o�b�t�@�����O
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;  // 32bit RGBA
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;         // �}���`�T���v�����O����

        ComPtr<IDXGISwapChain1> swapChain1;
        if (FAILED(m_dxgiFactory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),   // GPU���߂𗬂��L���[
            windowHandle,           // �Ώۂ̃E�B���h�E
            &swapChainDesc,         // �X���b�v�`�F�[���ݒ�
            nullptr, nullptr,       // �S��ʐݒ�Ȃ�
            &swapChain1))) {
            return false;
        }

        // IDXGISwapChain3�ɃL���X�g
        if (FAILED(swapChain1.As(&m_swapChain))) {
            return false;
        }

        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

        // --- �����_�[�^�[�Q�b�g�r���[�p�f�X�N���v�^�q�[�v�̍쐬 ---
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
        rtvHeapDesc.NumDescriptors = 2;                         // �o�b�N�o�b�t�@2��
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;      // �����_�[�^�[�Q�b�g�r���[�p
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;    // GPU������͎Q�Ƃ��Ȃ�

        if (FAILED(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)))) {
            return false;
        }

        // �f�X�N���v�^�̃T�C�Y���擾�iGPU�ˑ��j
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // --- �����_�[�^�[�Q�b�g�r���[�̍쐬 ---
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

        // �X���b�v�`�F�[���̊e�o�b�N�o�b�t�@�ɑ΂��ă����_�[�^�[�Q�b�g�r���[���쐬
        for (UINT i = 0; i < 2; i++)
        {
            if (FAILED(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])))) {
                return false;
            }

            // �����_�[�^�[�Q�b�g�r���[���쐬
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);

            // ���̃f�X�N���v�^�ֈړ�
            rtvHandle.ptr += m_rtvDescriptorSize;
        }

        // --- �R�}���h�A���P�[�^�̍쐬 ---
        if (FAILED(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)))) {
            return false;
        }

        // --- �R�}���h���X�g�̍쐬 ---
        if (FAILED(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList)))) {
            return false;
        }

        // �R�}���h���X�g�͍쐬���ɋL�^��ԂɂȂ��Ă���̂ŁA��x�N���[�Y����
        m_commandList->Close();

        // --- �����p�t�F���X�̍쐬 ---
        if (FAILED(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)))) {
            return false;
        }
        m_fenceValue = 1;

        // �t�F���X�C�x���g�̍쐬
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        if (!m_fenceEvent) {
            return false;
        }

        return true;
    }

    void App::update()
    {
        // ���݂͉������Ȃ�
        // �����I�ɂ̓Q�[�����W�b�N��A�j���[�V�����̍X�V�������L�q
    }

    void App::render()
    {
        // --- �R�}���h���X�g�̋L�^�J�n ---
        // �R�}���h�A���P�[�^�ƃR�}���h���X�g�����Z�b�g
        m_commandAllocator->Reset();
        m_commandList->Reset(m_commandAllocator.Get(), nullptr);

        // --- ���\�[�X�o���A�F�o�b�N�o�b�t�@�������_�[�^�[�Q�b�g��ԂɕύX ---
        D3D12_RESOURCE_BARRIER barrier{};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = m_renderTargets[m_frameIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;        // �\����Ԃ���
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;   // �����_�[�^�[�Q�b�g��Ԃ�
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);

        // --- �����_�[�^�[�Q�b�g�̐ݒ� ---
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += m_frameIndex * m_rtvDescriptorSize;  // ���݂̃t���[����RTV�ֈړ�

        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // --- ��ʃN���A�i�F�œh��Ԃ��j ---
        const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };  // R, G, B, A�i�Z���j
        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

        // --- ���\�[�X�o���A�F�o�b�N�o�b�t�@��\����Ԃɖ߂� ---
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;  // �����_�[�^�[�Q�b�g��Ԃ���
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;         // �\����Ԃ�

        m_commandList->ResourceBarrier(1, &barrier);

        // --- �R�}���h���X�g�̋L�^�I�� ---
        m_commandList->Close();

        // --- �R�}���h���X�g�̎��s ---
        ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
        m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // --- ��ʂɕ\�� ---
        m_swapChain->Present(1, 0);  // 1 = V-Sync�L��, 0 = �t���O�Ȃ�

        // --- �t���[�������҂� ---
        waitForPreviousFrame();
    }

    void App::waitForPreviousFrame()
    {
        // �t�F���X�ɃV�O�i���𑗐M
        const UINT64 fence = m_fenceValue;
        m_commandQueue->Signal(m_fence.Get(), fence);
        m_fenceValue++;

        // GPU���t�F���X�l�ɓ��B����܂őҋ@
        if (m_fence->GetCompletedValue() < fence)
        {
            m_fence->SetEventOnCompletion(fence, m_fenceEvent);
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        // ���̃t���[���C���f�b�N�X���擾
        m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    }
}