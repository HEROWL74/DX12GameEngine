// Core/App.hpp�i�N���X��`�j
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
        //�R���X�g���N�^
        App() = default;

        //�f�X�g���N�^
        ~App() = default;

        //�V���O���g��
        //�R�s�[�ƃ��[�u�֎~
        App(const App&) = delete;
        App& operator=(const App&) = delete;
        App(App&&) = delete;
        App& operator=(App&&) = delete;

        //�A�v���P�[�V����������
        // hInstance �A�v���P�[�V�����C���X�^���X
        // nCmdShow �\�����@
        //��������void�A���s���́A�G���[��Ԃ�
        
        [[nodiscard]]Utils::VoidResult initialize(HINSTANCE hInstance, int nCmdShow) noexcept;

        
        [[nodiscard]] int run() noexcept;

    private:

        // =============================================================================
       // �����o�ϐ�
       // =============================================================================


        //�E�B���h�E�Ǘ�
        Window m_window;

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

        // =============================================================================
        // �v���C�x�[�g���\�b�h
        // =============================================================================

        //DirectX 12�̏�����
        // ��������void�A���s����Error
        [[nodiscard]] Utils::VoidResult initD3D();

        //�X�V����
        void update();

        //�`�揈��
        void render();

        //�t���[�������҂�
        void waitForPreviousFrame();

        /// @brief ���\�[�X�̔j��
        void cleanup();

        // =============================================================================
        // �C�x���g�n���h��
        // =============================================================================

        //�E�B���h�E���T�C�Y���̃R�[���o�b�N
        // width �V������
        // height �V��������
        void onWindowResize(int width, int height);

        /// @brief �E�B���h�E�N���[�Y���̃R�[���o�b�N
        void onWindowClose();
    };
}