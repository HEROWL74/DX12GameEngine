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

        // �V���O���g���I�Ȏg�p���邽�߃R�s�[�E���[�u�֎~
        App(const App&) = delete;
        App& operator=(const App&) = delete;
        App(App&&) = delete;
        App& operator=(App&&) = delete;

        // �A�v���P�[�V�����̏�����
        [[nodiscard]] Utils::VoidResult initialize(HINSTANCE hInstance, int nCmdShow);

        // ���C�����[�v�����s
        [[nodiscard]] int run();

    private:
        // �E�B���h�E�ƃf�o�C�X�Ǘ�
        Window m_window;
        Graphics::Device m_device;
        Graphics::TriangleRenderer m_triangleRenderer;

        // �X���b�v�`�F�[���֌W
        ComPtr<ID3D12CommandQueue> m_commandQueue;      // GPU���߂̑��M��
        ComPtr<IDXGISwapChain3> m_swapChain;            // ��ʕ\���p�o�b�t�@�Ǘ�
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;         // �����_�[�^�[�Q�b�g�r���[�p�f�X�N���v�^�q�[�v
        ComPtr<ID3D12Resource> m_renderTargets[2];      // �`���o�b�t�@�i�_�u���o�b�t�@�����O�j

        // �R�}���h�֌W
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;  // �R�}���h���X�g�p�������Ǘ�
        ComPtr<ID3D12GraphicsCommandList> m_commandList;    // GPU���߂̋L�^

        // �����p
        ComPtr<ID3D12Fence> m_fence;        // CPU-GPU�����p
        UINT64 m_fenceValue = 0;            // �t�F���X�l
        HANDLE m_fenceEvent = nullptr;      // �t�F���X�C�x���g

        // �`��֌W
        UINT m_frameIndex = 0;              // ���݂̃t���[���C���f�b�N�X

        // ����������
        [[nodiscard]] Utils::VoidResult initD3D();
        [[nodiscard]] Utils::VoidResult createCommandQueue();
        [[nodiscard]] Utils::VoidResult createSwapChain();
        [[nodiscard]] Utils::VoidResult createRenderTargets();
        [[nodiscard]] Utils::VoidResult createCommandObjects();
        [[nodiscard]] Utils::VoidResult createSyncObjects();

        // �X�V�E�`�揈��
        void update();
        void render();

        // �t���[�������҂�
        void waitForPreviousFrame();

        // ���\�[�X�̔j��
        void cleanup();

        // �C�x���g�n���h��
        void onWindowResize(int width, int height);
        void onWindowClose();
    };
}