//src/Graphics/Texture.hpp
#pragma once

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "../Utils/Common.hpp"
#include "Device.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics 
{
	//====================================================
	//�e�N�X�`���t�H�[�}�b�g
	//====================================================
	enum class TextureFormat
	{
		UnKnown,   
		R8G8B8A8_UNORM,       //��ʓI��RGBA�t�H�[�}�b�g
		R8G8B8A8_SRGB,        //sRGB�F���
		R8G8B8_UNORM,         //RGB�i�A���t�@�Ȃ��j
		R8_UNORM,             //�O���[�X�P�[��
		R16G16B16A16_FLOAT,   //HDR�p
		BC1_UNORM,            //DXT1���k
		BC3_UNORM,            //DXT5���k
		BC7_UNORM,            //���i�����k
		D32_FLOAT,            //�[�x�o�b�t�@�p
		D24_UNORM_S8_UINT     //�[�x�X�e���V���p
	};

	//====================================================
	//�e�N�X�`���^�C�v�񋓌^
	//====================================================
	enum class TextureType
	{
		Texture2D,
		TextureCube,
		Texture2DArray,
		Texture3D
	};

	//====================================================
	//�e�N�X�`���g�p�@�t���O
	//====================================================
	enum class TextureUsage : uint32_t
	{
		None = 0,
		ShaderResource = 1 << 0,  //�V�F�[�_�[���\�[�X
		RenderTarget = 1 << 1,    //�����_�[�^�[�Q�b�g
		DepthStencil = 1 << 2,    //�[�x�X�e���V��
		UnorderedAccess = 1 << 3, //�R���s���[�g�V�F�[�_�[�ł̓ǂݏ���
		CopySource = 1 << 4,      //�R�s�[���Ƃ��Ďg�p
		CopyDestination = 1 << 5  //�R�s�[��Ƃ��Ďg�p
	};

	//�r�b�g���Z�q�̃I�[�o�[���[�h
	constexpr TextureUsage operator|(TextureUsage a, TextureUsage b)
	{
		return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	constexpr TextureUsage operator&(TextureUsage a, TextureUsage b)
	{
		return static_cast<TextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	//=====================================================
	//�e�N�X�`���쐬�ݒ�
	//=====================================================
	struct TextureDesc
	{
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t mipLevels = 1;
		uint32_t arraySize = 1;
		TextureFormat format = TextureFormat::R8G8B8A8_UNORM;
		TextureType type = TextureType::Texture2D;
		TextureUsage usage = TextureUsage::ShaderResource;
		bool generateMips = false;
		std::string debugName;
	};

	//======================================================
	//�摜�f�[�^�\����
	//======================================================
	struct ImageData
	{
		std::vector<uint8_t> pixels;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t channels = 0;
		TextureFormat format = TextureFormat::UnKnown;
		uint32_t rowPitch = 0;  //1���̃o�C�g��
		uint32_t slicePitch = 0; //1�X���C�X�̃o�C�g��
	};

	//===================================================
	//�e�N�X�`���N���X
	//==================================================


	class Texture
	{
	public:
		Texture() = default;
		~Texture() = default;

		//�R�s�[�E���[�u
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;
		Texture(Texture&&) = default;
		Texture& operator=(Texture&&) = default;
		  
		//�t�@�C������̍쐬
		[[nodiscard]] static Utils::Result<std::shared_ptr<Texture>> createFromFile(
			Device* device,
			const std::string& filePath,
			bool generateMips = true,
			bool sRGB = false
		);

		//����������̍쐬
		[[nodiscard]] static Utils::Result<std::shared_ptr<Texture>> createFromMemory(
			Device* device,
			const ImageData& imageData,
			const TextureDesc& desc
		);

		//��̃e�N�X�`���̍쐬
		[[nodiscard]] static Utils::Result<std::shared_ptr<Texture>> create(
			Device* device,
			const TextureDesc& desc
		);

		//��{���擾
		const TextureDesc& getDesc() const { return m_desc; }
		uint32_t getWidth() const { return m_desc.width; }
		uint32_t getHeight() const { return m_desc.height; }
		uint32_t getMipLevels() const { return m_desc.mipLevels; }
		TextureFormat getFormat() const { return m_desc.format; }

		//D3D12���\�[�X�̎擾
		ID3D12Resource* getResource() const { return m_resource.Get(); }
		D3D12_GPU_DESCRIPTOR_HANDLE getSRVHandle() const { return m_srvHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE getRTVHandle() const { return m_rtvHandle; }
		D3D12_GPU_DESCRIPTOR_HANDLE getDSVHandle() const { return m_dsvHandle; }

		//�L�����`�F�b�N
		bool isValid() const { return m_resource != nullptr; }

		//�f�o�b�O���̐ݒ�
		void setDebugName(const std::string& name);
	private:
		Device* m_device = nullptr;
		TextureDesc m_desc;
		ComPtr<ID3D12Resource> m_resource;

		//�f�B�X�N���v�^�n���h��
		D3D12_GPU_DESCRIPTOR_HANDLE m_srvHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_rtvHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_dsvHandle{};
	    
		//�������̕����i�e�N�X�`���̓��������j
		[[nodiscard]] Utils::VoidResult initialize(Device* device, const TextureDesc& desc);
		[[nodiscard]] Utils::VoidResult createResources();
		[[nodiscard]] Utils::VoidResult createViews();
		[[nodiscard]] Utils::VoidResult uploadData(const ImageData& imageData);
	};

	//===================================================
	//�e�N�X�`�����[�_�[�N���X
	//===================================================
	class TextureLoader
	{
	public:
		TextureLoader() = default;
		~TextureLoader() = default;

		//�R�s�[�E���[�u�֎~
		TextureLoader(const TextureLoader&) = delete;
		TextureLoader& operator=(const TextureLoader&) = delete;

		//�摜�t�@�C���̓ǂݍ���
		[[nodiscard]] static Utils::Result<ImageData> loadFromFile(const std::string& filePath);

		[[nodiscard]] static Utils::Result<ImageData> loadPNG(const std::string& filePath);
		[[nodiscard]] static Utils::Result<ImageData> loadJPEG(const std::string& filePath);
		[[nodiscard]] static Utils::Result<ImageData> loadDDS(const std::string& filePath);
		[[nodiscard]] static Utils::Result<ImageData> loadTGA(const std::string& filePath);

		//�~�j�}�b�v����
		[[nodiscard]] static Utils::Result<std::vector<ImageData>> generateMipmaps(const ImageData& baseImage);

		//�t�H�[�}�b�g�ϊ�
		[[nodiscard]] static Utils::Result<ImageData> convertFormat(
			const ImageData& sourceImage,
			TextureFormat targetFormat
		);

		//�t�@�C���`���̔���
		static std::string getFileExtention(const std::string& filePath);
		static bool isSupportedFormat(const std::string& extention);

	private:
		//�����w���p�[�֐�
		static uint32_t calculateRowPitch(uint32_t width, TextureFormat format);
		static uint32_t calculateSlicePitch(uint32_t rowPitch, uint32_t height);
		static DXGI_FORMAT textureFormatToDXGI(TextureFormat format);
		static TextureFormat dxgiToTextureFormat(DXGI_FORMAT format);
	};

	//========================================================================================
	//�e�N�X�`���}�l�[�W���[�N���X
	//================================================================
	class TextureManager
	{
	public:
		TextureManager() = default;
		~TextureManager() = default;

		//�R�s�[�E���[�u�֎~
		TextureManager(const TextureManager&) = delete;
		TextureManager& operator=(const TextureManager&) = delete;

		//������
		[[nodiscard]] Utils::VoidResult initialize(Device* device);

		//�e�N�X�`���̍쐬�E�擾
		[[nodiscard]] std::shared_ptr<Texture> loadTexture(
			const std::string& filePath,
			bool generateMips = true,
			bool sRGB = false
		);

		std::shared_ptr<Texture> getTexture(const std::string& name) const;
		bool hasTexture(const std::string& name) const;
		void removeTexture(const std::string& name);

		//�f�t�H���g�e�N�X�`��
		std::shared_ptr<Texture> getWhiteTexture() const { return m_whiteTexture; }
		std::shared_ptr<Texture> getBlackTexture() const { return m_blackTexture; }
		std::shared_ptr<Texture> getDefaultNormalTexture() const { return m_defaultNormalTexture; }

		//���v���
		size_t getTextureCount() const { return m_textures.size(); }
		size_t getTotalMemoryUsage() const;

		//�L���b�V���N���A
		void clearCache();

		//�L�����`�F�b�N
		bool isVaild() const { return m_initialized && m_device != nullptr; }

	private:
		Device* m_device;
		bool m_initialized = false;

		//�e�N�X�`���L���b�V��
		std::unordered_map < std::string, std::shared_ptr<Texture>> m_textures;

		//�f�t�H���g�e�N�X�`��
		std::shared_ptr<Texture> m_whiteTexture;
		std::shared_ptr<Texture> m_blackTexture;
		std::shared_ptr<Texture> m_defaultNormalTexture;

		//�������w���p�[
		[[nodiscard]] Utils::VoidResult createDefaultTextures();
		[[nodiscard]] Utils::VoidResult createSolidColorTexture(
			std::shared_ptr<Texture>& texture,
			uint32_t color,
			const std::string& name
		);
	};
}

