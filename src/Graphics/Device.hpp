// src/Graphics/Device.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <string>
#include "../Utils/Common.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
    // =============================================================================
    // �A�_�v�^�[���\����
    // =============================================================================

    //GPU�A�_�v�^�[�̏��
    struct AdapterInfo
    {
        std::wstring description;       // �A�_�v�^�[��
        size_t dedicatedVideoMemory;    // ��p�r�f�I������ (�o�C�g)
        size_t dedicatedSystemMemory;   // ��p�V�X�e�������� (�o�C�g)
        size_t sharedSystemMemory;      // ���L�V�X�e�������� (�o�C�g)
        bool isHardware;                // �n�[�h�E�F�A�A�_�v�^�[��
        UINT vendorId;                  // �x���_�[ID
        UINT deviceId;                  // �f�o�C�XID

        /// @brief ���������𕶎���Ŏ擾
        [[nodiscard]] std::string getMemoryInfoString() const noexcept;
    };

    // =============================================================================
    // �f�o�C�X�ݒ�\����
    // =============================================================================

    //�f�o�C�X�쐬���̐ݒ�
    struct DeviceSettings
    {
        bool enableDebugLayer = true;           // �f�o�b�O���C���[��L���ɂ��邩�i�f�o�b�O�r���h�̂݁j
        bool enableGpuValidation = false;      // GPU���؂�L���ɂ��邩�i�d���j
        D3D_FEATURE_LEVEL minFeatureLevel = D3D_FEATURE_LEVEL_11_0;  // �ŏ��@�\���x��
        bool preferHighPerformanceAdapter = true;  // �����\�A�_�v�^�[��D�悷�邩
    };

    // =============================================================================
    // Device�N���X
    // =============================================================================

    //DirectX 12�f�o�C�X�̍쐬�ƊǗ����s���N���X
    class Device
    {
    public:
        //�R���X�g���N�^
        Device() = default;

        //�f�X�g���N�^
        ~Device() = default;

        // �R�s�[�֎~
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        // ���[�u����
        Device(Device&&) noexcept = default;
        Device& operator=(Device&&) noexcept = default;

        //�f�o�C�X��������
        [[nodiscard]] Utils::VoidResult initialize(const DeviceSettings& settings = {});

        //���p�\�ȃA�_�v�^�[���
        [[nodiscard]] std::vector<AdapterInfo> enumerateAdapters() const;

        //D3D12�f�o�C�X���擾
        [[nodiscard]] ID3D12Device* getDevice() const noexcept { return m_device.Get(); }

        //DXGI�t�@�N�g�����擾
        [[nodiscard]] IDXGIFactory4* getDXGIFactory() const noexcept { return m_dxgiFactory.Get(); }

        //�f�o�C�X���L�����`�F�b�N
        [[nodiscard]] bool isValid() const noexcept { return m_device != nullptr; }

        //���݂̃A�_�v�^�[�����擾
        [[nodiscard]] const AdapterInfo& getCurrentAdapterInfo() const noexcept { return m_currentAdapterInfo; }

        //�T�|�[�g����Ă���@�\���x�����擾
        [[nodiscard]] D3D_FEATURE_LEVEL getFeatureLevel() const noexcept { return m_featureLevel; }

        //�f�o�b�O���C���[���L�����`�F�b�N
        [[nodiscard]] bool isDebugLayerEnabled() const noexcept { return m_debugLayerEnabled; }

        //�f�X�N���v�^�̃C���N�������g�T�C�Y���擾
        [[nodiscard]] UINT getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE heapType) const noexcept;

        //�@�\�T�|�[�g�`�F�b�N
        [[nodiscard]] bool checkFeatureSupport(D3D12_FEATURE feature, void* pFeatureSupportData, UINT featureSupportDataSize) const noexcept;

    private:
        // =============================================================================
        // �����o�ϐ�
        // =============================================================================

        ComPtr<ID3D12Device> m_device;              // D3D12�f�o�C�X
        ComPtr<IDXGIFactory4> m_dxgiFactory;        // DXGI�t�@�N�g��
        ComPtr<IDXGIAdapter1> m_adapter;            // �I�����ꂽ�A�_�v�^�[

        AdapterInfo m_currentAdapterInfo{};         // ���݂̃A�_�v�^�[���
        D3D_FEATURE_LEVEL m_featureLevel = D3D_FEATURE_LEVEL_11_0;  // �T�|�[�g�@�\���x��
        bool m_debugLayerEnabled = false;           // �f�o�b�O���C���[���L����

        // �f�X�N���v�^�T�C�Y�̃L���b�V��
        UINT m_rtvDescriptorSize = 0;
        UINT m_dsvDescriptorSize = 0;
        UINT m_cbvSrvUavDescriptorSize = 0;
        UINT m_samplerDescriptorSize = 0;

        // =============================================================================
        // �v���C�x�[�g���\�b�h
        // =============================================================================

        //�f�o�b�O���C���[��������
        [[nodiscard]] Utils::VoidResult initializeDebugLayer(const DeviceSettings& settings);

        /// DXGI�t�@�N�g�����쐬
        [[nodiscard]] Utils::VoidResult createDXGIFactory();

        //�œK�ȃA�_�v�^�[��I��
        [[nodiscard]] Utils::VoidResult selectBestAdapter(const DeviceSettings& settings);

        //D3D12�f�o�C�X���쐬
        [[nodiscard]] Utils::VoidResult createDevice(const DeviceSettings& settings);

        //�f�X�N���v�^�T�C�Y���L���b�V��
        void cacheDescriptorSizes();

        //�A�_�v�^�[�����擾
        [[nodiscard]] AdapterInfo getAdapterInfo(IDXGIAdapter1* adapter) const;

        //�A�_�v�^�[��D3D12�Ή����`�F�b�N
        [[nodiscard]] bool isAdapterCompatible(IDXGIAdapter1* adapter, D3D_FEATURE_LEVEL minFeatureLevel) const;
    };
}