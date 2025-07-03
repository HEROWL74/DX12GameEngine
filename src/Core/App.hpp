// Core/App.hpp�i�N���X��`�j
#pragma once

#include <Windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

namespace Core
{
    class App
    {
    public:
        [[nodiscard]]
        bool initialize(HINSTANCE hInstance, int nCmdShow) noexcept;

        [[nodiscard]]
        int run() noexcept;

    private:
        // �E�B���h�E�֌W
        HWND windowHandle = nullptr;
        static constexpr int width = 1280;
        static constexpr int height = 720;

        // �������֐�
        [[nodiscard]]
        bool initWindow(HINSTANCE hInstance, int nCmdShow) noexcept;

        [[nodiscard]]
        bool initD3D();

        // �����_�����O�֐�
        void update();
        void render();

        // DirectX 12�֌W�̃����o�ϐ�
        ComPtr<ID3D12Device> m_device;              // D3D12�f�o�C�X�iGPU����̒��j�j
        ComPtr<ID3D12CommandQueue> m_commandQueue;  // �R�}���h�L���[�iGPU���߂̑��M��j
        ComPtr<IDXGIFactory4> m_dxgiFactory;        // DXGI �t�@�N�g���i�X���b�v�`�F�[���쐬�p�j
        ComPtr<IDXGISwapChain3> m_swapChain;        // �X���b�v�`�F�[���i��ʕ\���p�o�b�t�@�Ǘ��j
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;     // �����_�[�^�[�Q�b�g�r���[�p�f�X�N���v�^�q�[�v
        ComPtr<ID3D12Resource> m_renderTargets[2];  // �����_�[�^�[�Q�b�g�i�`���o�b�t�@�j
        ComPtr<ID3D12CommandAllocator> m_commandAllocator; // �R�}���h�A���P�[�^�i�R�}���h���X�g�p�������Ǘ��j
        ComPtr<ID3D12GraphicsCommandList> m_commandList;   // �R�}���h���X�g�iGPU���߂̋L�^�j

        // �����p
        ComPtr<ID3D12Fence> m_fence;    // �t�F���X�iCPU-GPU�����p�j
        UINT64 m_fenceValue = 0;        // �t�F���X�l
        HANDLE m_fenceEvent = nullptr;  // �t�F���X�C�x���g

        // �`��֌W
        UINT m_rtvDescriptorSize = 0;   // �����_�[�^�[�Q�b�g�r���[�̃f�X�N���v�^�T�C�Y
        UINT m_frameIndex = 0;          // ���݂̃t���[���C���f�b�N�X

        // �����ҋ@�p�̃w���p�[�֐�
        void waitForPreviousFrame();
    };
}