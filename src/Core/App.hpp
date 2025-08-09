// src/Core/App.hpp
#pragma once

#include <Windows.h>
#include <memory>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <chrono>
#include <mutex>
#include "Window.hpp"
#include "../Graphics/Device.hpp"
#include "../Graphics/Camera.hpp"
#include "../Input/InputManager.hpp"
#include "../Utils/Common.hpp"
#include "../Graphics/RenderComponent.hpp"
#include "../UI/ImGuiManager.hpp"
#include "../Graphics/MaterialSerialization.hpp"
#include "../Graphics/ShaderManager.hpp"
#include "../UI/ProjectWindow.hpp"
#include "../UI/ContextMenu.hpp"

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

        // �V���O���g���I�Ȏg�p��z�肵�Ă���̂ŃR�s�[�E���[�u�֎~
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
        Graphics::CubeRenderer m_cubeRenderer;
        std::vector<Graphics::CubeRenderer> m_cubes; //�����̗����̗p
        Graphics::Camera m_camera;
        std::unique_ptr<Graphics::FPSCameraController> m_cameraController;
        Graphics::Scene m_scene;

        // �X���b�v�`�F�[���֌W
        ComPtr<ID3D12CommandQueue> m_commandQueue;      // GPU���߂̑��M��
        ComPtr<IDXGISwapChain3> m_swapChain;            // ��ʕ\���p�o�b�t�@�Ǘ�
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;         // �����_�[�^�[�Q�b�g�r���[�p�f�X�N���v�^�q�[�v
        ComPtr<ID3D12Resource> m_renderTargets[2];      // �`���o�b�t�@�i�_�u���o�b�t�@�����O�j

        //�[�x�o�b�t�@�֌W
        ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
        ComPtr<ID3D12Resource> m_depthStencilBuffer;

        // �R�}���h�֌W
        ComPtr<ID3D12CommandAllocator> m_commandAllocator;  // �R�}���h���X�g�p�������Ǘ�
        ComPtr<ID3D12GraphicsCommandList> m_commandList;    // GPU���߂̋L�^

        // �����p
        ComPtr<ID3D12Fence> m_fence;        // CPU-GPU�����p
        UINT64 m_fenceValue = 0;            // �t�F���X�l
        HANDLE m_fenceEvent = nullptr;      // �t�F���X�C�x���g

        // �`��֌W
        UINT m_frameIndex = 0;              // ���݂̃t���[���C���f�b�N�X

        bool m_isResizing = false;
        std::mutex m_resizeMutex;
        // ���ԊǗ�
        std::chrono::high_resolution_clock::time_point m_lastFrameTime{};
        float m_deltaTime = 0.0f;
        float m_currentFPS = 0.0f;
        int m_frameCount = 0;
        float m_frameTimeAccumulator = 0.0f;

        //ImGui�֌W�̃X�}�[�g�|�C���^
        UI::ImGuiManager m_imguiManager;
        std::unique_ptr<UI::DebugWindow> m_debugWindow;
        std::unique_ptr<UI::SceneHierarchyWindow> m_hierarchyWindow;
        std::unique_ptr<UI::InspectorWindow> m_inspectorWindow;
        std::unique_ptr<UI::ProjectWindow> m_projectWindow;

        //�}�e���A���֌W
        Graphics::MaterialManager m_materialManager;
        Graphics::TextureManager m_textureManager;
        std::unique_ptr<Graphics::ShaderManager> m_shaderManager;
        
        //�R���e�L�X�g���j���[�֌W
        Core::GameObject* createPrimitiveObject(UI::PrimitiveType type, const std::string& name);
        void deleteGameObject(Core::GameObject* object);
        Core::GameObject* duplicateGameObject(Core::GameObject* original);
        void renameGameObject(Core::GameObject* object, const std::string& newName);
        Graphics::RenderableType primitiveToRenderableType(UI::PrimitiveType type);
        UI::PrimitiveType renderableToPrimitiveType(Graphics::RenderableType renderType);
        std::string generateUniqueName(const std::string& baseName);
        // unique_ptr�ɕύX
     

        // ����������
        [[nodiscard]] Utils::VoidResult initD3D();
        [[nodiscard]] Utils::VoidResult initializeInput();
        [[nodiscard]] Utils::VoidResult createCommandQueue();
        [[nodiscard]] Utils::VoidResult createSwapChain();
        [[nodiscard]] Utils::VoidResult createRenderTargets();
        [[nodiscard]] Utils::VoidResult createDepthStencilBuffer();
        [[nodiscard]] Utils::VoidResult createCommandObjects();
        [[nodiscard]] Utils::VoidResult createSyncObjects();

        // �X�V�E�`�揈��
        void update();
        void render();

        // ���ԊǗ�
        void updateDeltaTime();
        void processInput();

        // �t���[�������҂�
        void waitForPreviousFrame();

        // ���\�[�X�̔j��
        void cleanup();

        // �C�x���g�n���h��
        void onWindowResize(int width, int height);
        void onWindowClose();

        // ���̓C�x���g�n���h��
        void onKeyPressed(Input::KeyCode key);
        void onKeyReleased(Input::KeyCode key);
        void onMouseMove(int x, int y, int deltaX, int deltaY);
        void onMouseButtonPressed(Input::MouseButton button, int x, int y);
    };
}