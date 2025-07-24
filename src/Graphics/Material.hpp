//src/Graphics/Material.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"
#include "Device.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
	//�O���錾
	class Texture;

	//======================================================================
	//�e�N�X�`���^�C�v�񋓌^
	//======================================================================
	enum class TextureType
	{
		Albedo, //�x�[�X�J���[�i�g�U���ːF�j
		Normal, //�@���}�b�v
		Metallic, //���^���b�N
		Roughness,//���t�l�X
		AO,      //�A���r�G���g�I�N���[�W����
		Emissive, //����
		Height,   //�����}�b�v
		Count  //�e�N�X�`���̐��̌v�Z�p
	};

	//======================================================================
	//�}�e���A���v���p�e�B�\����
	//======================================================================
	struct MaterialProperties
	{
		//PBR�p�����[�^
		Math::Vector3 albedo = Math::Vector3(1.0f, 1.0f, 1.0f);  //�x�[�X�J���[
		float metallic = 0.0f;  //���^���b�N�l�i0.0 == �����, 1.0 == �����j
		float roughness = 0.5f; //���t�l�X�l�i0.0 == ���S�Ɋ��炩, 1.0 == ���S�ɑe���j
		float ao = 1.0f;  //�A���r�G���g�I�N���[�W����

		//����
		Math::Vector3 emissive = Math::Vector3(0.0f, 0.0f, 0.0f); //�����F
		float emissiveStrength = 1.0f;  //�������x

		//���̑��̃v���p�e�B
		float normalStrength = 1.0f;  //�@���}�b�v�̋��x
		float heightScale = 0.05f;    //�����}�b�v�̃X�P�[��

		//�A���t�@�֘A
		float alpha = 1.0f;    //�����x
		bool useAlphaText = false; //�A���t�@�e�X�g���g�p���邩���Ȃ���
		float alphaTestThreshold = 0.5f;  //�A���t�@�e�X�g��臒l�i�������j

		//�e�N�X�`���̃^�C�����O
		Math::Vector2 uvScale = Math::Vector2(1.0f, 1.0f);  //UV�X�P�[��
		Math::Vector2 uvOffset = Math::Vector2(0.0f, 0.0f); //UV�I�t�Z�b�g
	};

	//======================================================================
	//�}�e���A���N���X
	//======================================================================
	class Material
	{
	public:
		Material(const std::string& name = "DefaultMaterial");
		~Material() = default;

		// �R�s�[�E���[�u
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;
		Material(Material&&) = default;
		Material& operator=(Material&&) = default;

		// ������
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		// ��{���
		const std::string& getName() const { return m_name; }
		void setName(const std::string& name) { m_name = name; }

		// �}�e���A���v���p�e�B
		const MaterialProperties& getProperties() const { return m_properties; }
		MaterialProperties& getProperties() { return m_properties; }
		void setProperties(const MaterialProperties& properties) { m_properties = properties; m_isDirty = true; }

		// �ʃv���p�e�B�A�N�Z�X
		void setAlbedo(const Math::Vector3& albedo) { m_properties.albedo = albedo; m_isDirty = true; }
		void setMetallic(float metallic) { m_properties.metallic = metallic; m_isDirty = true; }
		void setRoughness(float roughness) { m_properties.roughness = roughness; m_isDirty = true; }
		void setEmissive(const Math::Vector3& emissive) { m_properties.emissive = emissive; m_isDirty = true; }

		// �e�N�X�`���Ǘ�
		void setTexture(TextureType type, std::shared_ptr<Texture> texture);
		std::shared_ptr<Texture> getTexture(TextureType type) const;
		bool hasTexture(TextureType type) const;
		void removeTexture(TextureType type);

		// GPU�萔�o�b�t�@�̍X�V
		[[nodiscard]] Utils::VoidResult updateConstantBuffer();
		ID3D12Resource* getConstantBuffer() const { return m_constantBuffer.Get(); }

		// �V�F�[�_�[���\�[�X�r���[�̎擾
		D3D12_GPU_DESCRIPTOR_HANDLE getSRVGpuHandle() const { return m_srvGpuHandle; }
		D3D12_CPU_DESCRIPTOR_HANDLE getSRVCpuHandle() const { return m_srvCpuHandle; }

		// �L�����`�F�b�N
		bool isValid() const { return m_initialized && m_device != nullptr; }

		// �}�e���A���̓K�p�i�����_�����O���ɌĂԁj
		void bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const;

		// �t�@�C�����o�́i��Ŏ����j
		[[nodiscard]] Utils::VoidResult saveToFile(const std::string& filePath) const;
		[[nodiscard]] Utils::VoidResult loadFromFile(const std::string& filePath);
	private:
		std::string m_name;
		MaterialProperties m_properties;
		Device* m_device = nullptr;
		bool m_initialized = false;
		bool m_isDirty = true;

		//�e�N�X�`���}�b�v
		std::unordered_map<TextureType, std::shared_ptr<Texture>> m_textures;

		//GPU���\�[�X
		ComPtr<ID3D12Resource> m_constantBuffer;
		void* m_constantBufferData = nullptr;

		//�f�B�X�N���v�^�n���h��
		D3D12_CPU_DESCRIPTOR_HANDLE m_srvCpuHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_srvGpuHandle{};

		//�������w���p�[
	    [[nodiscard]] Utils::VoidResult createConstantBuffer();
		[[nodiscard]] Utils::VoidResult createDescriptors();

		//GPU�萔�o�b�t�@�p�̍\����
		struct MaterialConstantBuffer
		{
			Math::Vector4 albedo; //xyz: �x�[�X�J���[ w: ���^���b�N
			Math::Vector4 roughnessAoEmissiveStrength; //x:���t�l�X y:�A���r�G���g�I�N���[�W���� z: �������x w: �p�f�B���O
			Math::Vector4 emissive;  //xyz: ���� w: �@�����x
			Math::Vector4 alphaParams;  // x�F�A���t�@�l�i�����x) // y�F�A���t�@�e�X�g���g�����ǂ����itrue / false�j// z�F�A���t�@�e�X�g�̂������l�i�J�b�g�I�t�l)// w�F�n�C�g�}�b�v�̍����X�P�[��
			Math::Vector4 uvTransform; //xy: UV�X�P�[�� zw: UV�I�t�Z�b�g
		};
	};

	//============================================================
	//�}�e���A���}�l�[�W�W���[�N���X
	//============================================================
	class MaterialManager
	{
	public:
		MaterialManager() = default;
		~MaterialManager() = default;

		//�R�s�[�E���[�u�֎~
		MaterialManager(const MaterialManager&) = delete;
		MaterialManager& operator=(const MaterialManager&) = delete;

		//������
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//�}�e���A���Ǘ�
		std::shared_ptr<Material> createMaterial(const std::string& name);
		std::shared_ptr<Material> getMaterial(const std::string& name) const;
		bool hasMaterial(const std::string& name) const;
		void removeMaterial(const std::string& name);

		//�f�t�H���g�}�e���A��
          std::shared_ptr<Material> getDefaultMaterial() const { return m_defaultMaterial; }

		  //�S�}�e���A���̎擾
		  const std::unordered_map<std::string, std::shared_ptr<Material>>& getAllMaterial() const { return m_materials; };

		  //���\�[�X�̍X�V
		  void updateAllMaterials();

		

		  //�L�����`�F�b�N
		  bool isValid() const { return m_initialized && m_device != nullptr; }

	private:
		Device* m_device = nullptr;
		bool m_initialized = false;

		std::unordered_map<std::string, std::shared_ptr<Material>> m_materials;
		std::shared_ptr<Material> m_defaultMaterial;

		[[nodiscard]] Utils::VoidResult createDefaultMaterial();
	};

	//============================================================
	//���[�e�B���e�B�֐�
	//============================================================
    
	//TextureType�𕶎���ɕϊ�
	std::string textureTypeToString(TextureType type);

	//�������TextureType�ɕϊ�
	TextureType stringToTextureType(const std::string& str);
}
