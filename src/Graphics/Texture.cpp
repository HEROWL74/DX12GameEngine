//src/Graphics/Texture.cpp
#include "Texture.hpp"
#include <format>
#include <fstream>
#include <filesystem>
#include <algorithm>

// STB�摜���C�u�����i�w�b�_�[�I�����[���C�u�����j
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_resize2.h"

namespace Engine::Graphics
{
    //=========================================================================
    // Texture����
    //=========================================================================

    Utils::Result<std::shared_ptr<Texture>> Texture::createFromFile(
        Device* device,
        const std::string& filePath,
        bool generateMips,
        bool sRGB)
    {
        // �摜�f�[�^��ǂݍ���
        auto imageResult = TextureLoader::loadFromFile(filePath);
        if (!imageResult)
        {
            return std::unexpected(imageResult.error());
        }

        // �e�N�X�`���ݒ���쐬
        TextureDesc desc;
        desc.width = imageResult->width;
        desc.height = imageResult->height;
        desc.format = sRGB ? TextureFormat::R8G8B8A8_SRGB : imageResult->format;
        desc.generateMips = generateMips;
        desc.mipLevels = generateMips ? 0 : 1; // 0 = �����v�Z
        desc.usage = TextureUsage::ShaderResource;
        desc.debugName = std::filesystem::path(filePath).filename().string();

        return createFromMemory(device, *imageResult, desc);
    }

    Utils::Result<std::shared_ptr<Texture>> Texture::createFromMemory(
        Device* device,
        const ImageData& imageData,
        const TextureDesc& desc)
    {
        auto texture = std::make_shared<Texture>();

        auto initResult = texture->initialize(device, desc);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        auto uploadResult = texture->uploadData(imageData);
        if (!uploadResult)
        {
            return std::unexpected(uploadResult.error());
        }

        return texture;
    }

    Utils::Result<std::shared_ptr<Texture>> Texture::create(
        Device* device,
        const TextureDesc& desc)
    {
        auto texture = std::make_shared<Texture>();

        auto initResult = texture->initialize(device, desc);
        if (!initResult)
        {
            return std::unexpected(initResult.error());
        }

        return texture;
    }

    void Texture::setDebugName(const std::string& name)
    {
        if (m_resource)
        {
            std::wstring wname(name.begin(), name.end());
            m_resource->SetName(wname.c_str());
        }
        m_desc.debugName = name;
    }

    Utils::VoidResult Texture::initialize(Device* device, const TextureDesc& desc)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        m_desc = desc;

        // �~�b�v���x���̎����v�Z
        if (m_desc.mipLevels == 0)
        {
            m_desc.mipLevels = static_cast<uint32_t>(std::floor(std::log2(max(m_desc.width, m_desc.height)))) + 1;
        }

        auto resourceResult = createResource();
        if (!resourceResult)
        {
            return resourceResult;
        }

        auto viewsResult = createViews();
        if (!viewsResult)
        {
            return viewsResult;
        }

        if (!m_desc.debugName.empty())
        {
            setDebugName(m_desc.debugName);
        }

        return {};
    }

    Utils::VoidResult Texture::createResource()
    {
        D3D12_RESOURCE_DESC resourceDesc{};

        switch (m_desc.dimension)
        {
        case TextureDimension::Texture2D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            break;
        case TextureDimension::Texture3D:
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            break;
        default:
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation, "Unsupported texture dimension"));
        }

        resourceDesc.Alignment = 0;
        resourceDesc.Width = m_desc.width;
        resourceDesc.Height = m_desc.height;
        resourceDesc.DepthOrArraySize = m_desc.dimension == TextureDimension::Texture3D ? m_desc.depth : m_desc.arraySize;
        resourceDesc.MipLevels = m_desc.mipLevels;
        resourceDesc.Format = textureFormatToDXGI(m_desc.format);
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resourceDesc.Flags = textureUsageToD3D12Flags(m_desc.usage);

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_CLEAR_VALUE* clearValue = nullptr;
        D3D12_CLEAR_VALUE optimizedClearValue{};

        // �����_�[�^�[�Q�b�g�܂��͐[�x�X�e���V���̏ꍇ�A�œK���N���A�l��ݒ�
        if ((m_desc.usage & TextureUsage::RenderTarget) != TextureUsage::None)
        {
            optimizedClearValue.Format = resourceDesc.Format;
            optimizedClearValue.Color[0] = 0.0f;
            optimizedClearValue.Color[1] = 0.0f;
            optimizedClearValue.Color[2] = 0.0f;
            optimizedClearValue.Color[3] = 1.0f;
            clearValue = &optimizedClearValue;
        }
        else if ((m_desc.usage & TextureUsage::DepthStencil) != TextureUsage::None)
        {
            optimizedClearValue.Format = resourceDesc.Format;
            optimizedClearValue.DepthStencil.Depth = 1.0f;
            optimizedClearValue.DepthStencil.Stencil = 0;
            clearValue = &optimizedClearValue;
        }

        CHECK_HR(m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            textureUsageToD3D12State(m_desc.usage),
            clearValue,
            IID_PPV_ARGS(&m_resource)),
            Utils::ErrorType::ResourceCreation,
            std::format("Failed to create texture resource: {}", m_desc.debugName));

        return {};
    }

    Utils::VoidResult Texture::createViews()
    {
        // ���݂͊�{�I�Ȏ����̂�
        // ���S�ȃf�X�N���v�^�Ǘ��͌�Ŏ���
        return {};
    }

    Utils::VoidResult Texture::uploadData(const ImageData& imageData)
    {
        // ���݂͊ȈՎ���
        // ���S�ȃA�b�v���[�h�@�\�͌�Ŏ���
        return {};
    }

    //=========================================================================
    // TextureLoader����
    //=========================================================================

    Utils::Result<ImageData> TextureLoader::loadFromFile(const std::string& filePath)
    {
        if (!std::filesystem::exists(filePath))
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Texture file not found: {}", filePath)));
        }

        std::string extension = getFileExtension(filePath);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

        if (extension == ".png")
        {
            return loadPNG(filePath);
        }
        else if (extension == ".jpg" || extension == ".jpeg")
        {
            return loadJPEG(filePath);
        }
        else if (extension == ".dds")
        {
            return loadDDS(filePath);
        }
        else if (extension == ".tga")
        {
            return loadTGA(filePath);
        }
        else
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Unsupported texture format: {}", extension)));
        }
    }

    Utils::Result<ImageData> TextureLoader::loadPNG(const std::string& filePath)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!data)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to load PNG: {}", stbi_failure_reason())));
        }

        ImageData imageData;
        imageData.width = static_cast<uint32_t>(width);
        imageData.height = static_cast<uint32_t>(height);
        imageData.channels = 4; // STBI_rgb_alpha�ŋ����I��4�`�����l��
        imageData.format = TextureFormat::R8G8B8A8_UNORM;

        size_t dataSize = width * height * 4;
        imageData.pixels.resize(dataSize);
        memcpy(imageData.pixels.data(), data, dataSize);

        imageData.rowPitch = calculateRowPitch(imageData.width, imageData.format);
        imageData.slicePitch = calculateSlicePitch(imageData.rowPitch, imageData.height);

        stbi_image_free(data);
        return imageData;
    }

    Utils::Result<ImageData> TextureLoader::loadJPEG(const std::string& filePath)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!data)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to load JPEG: {}", stbi_failure_reason())));
        }

        ImageData imageData;
        imageData.width = static_cast<uint32_t>(width);
        imageData.height = static_cast<uint32_t>(height);
        imageData.channels = 4;
        imageData.format = TextureFormat::R8G8B8A8_UNORM;

        size_t dataSize = width * height * 4;
        imageData.pixels.resize(dataSize);
        memcpy(imageData.pixels.data(), data, dataSize);

        imageData.rowPitch = calculateRowPitch(imageData.width, imageData.format);
        imageData.slicePitch = calculateSlicePitch(imageData.rowPitch, imageData.height);

        stbi_image_free(data);
        return imageData;
    }

    Utils::Result<ImageData> TextureLoader::loadDDS(const std::string& filePath)
    {
        // DDS�ǂݍ��݂͕��G�Ȃ̂ŁA���݂͖�����
        return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0, "DDS format not implemented yet"));
    }

    Utils::Result<ImageData> TextureLoader::loadTGA(const std::string& filePath)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(filePath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!data)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to load TGA: {}", stbi_failure_reason())));
        }

        ImageData imageData;
        imageData.width = static_cast<uint32_t>(width);
        imageData.height = static_cast<uint32_t>(height);
        imageData.channels = 4;
        imageData.format = TextureFormat::R8G8B8A8_UNORM;

        size_t dataSize = width * height * 4;
        imageData.pixels.resize(dataSize);
        memcpy(imageData.pixels.data(), data, dataSize);

        imageData.rowPitch = calculateRowPitch(imageData.width, imageData.format);
        imageData.slicePitch = calculateSlicePitch(imageData.rowPitch, imageData.height);

        stbi_image_free(data);
        return imageData;
    }

    std::string TextureLoader::getFileExtension(const std::string& filePath)
    {
        size_t dotPos = filePath.find_last_of('.');
        if (dotPos != std::string::npos)
        {
            return filePath.substr(dotPos);
        }
        return "";
    }

    bool TextureLoader::isSupportedFormat(const std::string& extension)
    {
        std::string ext = extension;
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        return ext == ".png" || ext == ".jpg" || ext == ".jpeg" ||
            ext == ".tga" || ext == ".dds";
    }

    uint32_t TextureLoader::calculateRowPitch(uint32_t width, TextureFormat format)
    {
        return width * getBytesPerPixel(format);
    }

    uint32_t TextureLoader::calculateSlicePitch(uint32_t rowPitch, uint32_t height)
    {
        return rowPitch * height;
    }

    //=========================================================================
    // TextureManager����
    //=========================================================================

    Utils::VoidResult TextureManager::initialize(Device* device)
    {
        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        auto defaultResult = createDefaultTextures();
        if (!defaultResult)
        {
            return defaultResult;
        }

        m_initialized = true;
        Utils::log_info("TextureManager initialized successfully");
        return {};
    }

    std::shared_ptr<Texture> TextureManager::loadTexture(
        const std::string& filePath,
        bool generateMips,
        bool sRGB)
    {
        if (!m_initialized)
        {
            Utils::log_warning("TextureManager not initialized");
            return nullptr;
        }

        // �L���b�V���`�F�b�N
        std::string key = filePath;
        if (hasTexture(key))
        {
            return getTexture(key);
        }

        // �e�N�X�`����ǂݍ���
        auto textureResult = Texture::createFromFile(m_device, filePath, generateMips, sRGB);
        if (!textureResult)
        {
            Utils::log_warning(std::format("Failed to load texture '{}': {}", filePath, textureResult.error().message));
            return getWhiteTexture(); // �t�H�[���o�b�N
        }

        m_textures[key] = *textureResult;
        Utils::log_info(std::format("Texture loaded: {}", filePath));
        return *textureResult;
    }

    std::shared_ptr<Texture> TextureManager::getTexture(const std::string& name) const
    {
        auto it = m_textures.find(name);
        return (it != m_textures.end()) ? it->second : nullptr;
    }

    bool TextureManager::hasTexture(const std::string& name) const
    {
        return m_textures.find(name) != m_textures.end();
    }

    void TextureManager::removeTexture(const std::string& name)
    {
        auto it = m_textures.find(name);
        if (it != m_textures.end())
        {
            m_textures.erase(it);
        }
    }

    size_t TextureManager::getTotalMemoryUsage() const
    {
        // �ȈՎ���
        return m_textures.size() * 1024 * 1024; // ���̒l
    }

    void TextureManager::clearCache()
    {
        // �f�t�H���g�e�N�X�`���ȊO���N���A
        auto whiteTexture = m_whiteTexture;
        auto blackTexture = m_blackTexture;
        auto defaultNormalTexture = m_defaultNormalTexture;

        m_textures.clear();

        if (whiteTexture) m_textures["__white__"] = whiteTexture;
        if (blackTexture) m_textures["__black__"] = blackTexture;
        if (defaultNormalTexture) m_textures["__default_normal__"] = defaultNormalTexture;
    }

    Utils::VoidResult TextureManager::createDefaultTextures()
    {
        // ���e�N�X�`���i1x1�j
        auto whiteResult = createSolidColorTexture(m_whiteTexture, 0xFFFFFFFF, "DefaultWhite");
        if (!whiteResult)
        {
            return whiteResult;
        }

        // ���e�N�X�`���i1x1�j
        auto blackResult = createSolidColorTexture(m_blackTexture, 0xFF000000, "DefaultBlack");
        if (!blackResult)
        {
            return blackResult;
        }

        // �f�t�H���g�@���}�b�v�i1x1�A�@����������j
        auto normalResult = createSolidColorTexture(m_defaultNormalTexture, 0xFFFF8080, "DefaultNormal");
        if (!normalResult)
        {
            return normalResult;
        }

        // �L���b�V���ɓo�^
        m_textures["__white__"] = m_whiteTexture;
        m_textures["__black__"] = m_blackTexture;
        m_textures["__default_normal__"] = m_defaultNormalTexture;

        return {};
    }

    Utils::VoidResult TextureManager::createSolidColorTexture(
        std::shared_ptr<Texture>& texture,
        uint32_t color,
        const std::string& name)
    {
        // 1x1�̃C���[�W�f�[�^���쐬
        ImageData imageData;
        imageData.width = 1;
        imageData.height = 1;
        imageData.channels = 4;
        imageData.format = TextureFormat::R8G8B8A8_UNORM;
        imageData.pixels.resize(4);

        // �J���[�f�[�^��ݒ�iRGBA�j
        imageData.pixels[0] = (color >> 16) & 0xFF; // R
        imageData.pixels[1] = (color >> 8) & 0xFF;  // G
        imageData.pixels[2] = color & 0xFF;         // B
        imageData.pixels[3] = (color >> 24) & 0xFF; // A

        imageData.rowPitch = 4;
        imageData.slicePitch = 4;

        TextureDesc desc;
        desc.width = 1;
        desc.height = 1;
        desc.format = TextureFormat::R8G8B8A8_UNORM;
        desc.usage = TextureUsage::ShaderResource;
        desc.debugName = name;

        auto textureResult = Texture::createFromMemory(m_device, imageData, desc);
        if (!textureResult)
        {
            return std::unexpected(textureResult.error());
        }

        texture = *textureResult;
        return {};
    }

    //=========================================================================
    // ���[�e�B���e�B�֐�����
    //=========================================================================

    uint32_t getBytesPerPixel(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::R8G8B8A8_UNORM:
        case TextureFormat::R8G8B8A8_SRGB:
            return 4;
        case TextureFormat::R8G8B8_UNORM:
            return 3;
        case TextureFormat::R8_UNORM:
            return 1;
        case TextureFormat::R16G16B16A16_FLOAT:
            return 8;
        case TextureFormat::D32_FLOAT:
            return 4;
        case TextureFormat::D24_UNORM_S8_UINT:
            return 4;
        case TextureFormat::BC1_UNORM:
            return 0; // ���k�t�H�[�}�b�g�͓��ʌv�Z
        case TextureFormat::BC3_UNORM:
        case TextureFormat::BC7_UNORM:
            return 0; // ���k�t�H�[�}�b�g�͓��ʌv�Z
        default:
            return 4; // �f�t�H���g
        }
    }

    bool isCompressedFormat(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::BC1_UNORM:
        case TextureFormat::BC3_UNORM:
        case TextureFormat::BC7_UNORM:
            return true;
        default:
            return false;
        }
    }

    bool isSRGBFormat(TextureFormat format)
    {
        return format == TextureFormat::R8G8B8A8_SRGB;
    }

    bool isDepthFormat(TextureFormat format)
    {
        return format == TextureFormat::D32_FLOAT || format == TextureFormat::D24_UNORM_S8_UINT;
    }

    DXGI_FORMAT textureFormatToDXGI(TextureFormat format)
    {
        switch (format)
        {
        case TextureFormat::R8G8B8A8_UNORM:  return DXGI_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R8G8B8A8_SRGB:   return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case TextureFormat::R8G8B8_UNORM:    return DXGI_FORMAT_R8G8B8A8_UNORM; // 3�`�����l����4�`�����l���Ƃ��Ĉ���
        case TextureFormat::R8_UNORM:        return DXGI_FORMAT_R8_UNORM;
        case TextureFormat::R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case TextureFormat::BC1_UNORM:       return DXGI_FORMAT_BC1_UNORM;
        case TextureFormat::BC3_UNORM:       return DXGI_FORMAT_BC3_UNORM;
        case TextureFormat::BC7_UNORM:       return DXGI_FORMAT_BC7_UNORM;
        case TextureFormat::D32_FLOAT:       return DXGI_FORMAT_D32_FLOAT;
        case TextureFormat::D24_UNORM_S8_UINT: return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:                             return DXGI_FORMAT_R8G8B8A8_UNORM;
        }
    }

    TextureFormat dxgiToTextureFormat(DXGI_FORMAT format)
    {
        switch (format)
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:         return TextureFormat::R8G8B8A8_UNORM;
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:    return TextureFormat::R8G8B8A8_SRGB;
        case DXGI_FORMAT_R8_UNORM:               return TextureFormat::R8_UNORM;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:     return TextureFormat::R16G16B16A16_FLOAT;
        case DXGI_FORMAT_BC1_UNORM:              return TextureFormat::BC1_UNORM;
        case DXGI_FORMAT_BC3_UNORM:              return TextureFormat::BC3_UNORM;
        case DXGI_FORMAT_BC7_UNORM:              return TextureFormat::BC7_UNORM;
        case DXGI_FORMAT_D32_FLOAT:              return TextureFormat::D32_FLOAT;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:      return TextureFormat::D24_UNORM_S8_UINT;
        default:                                 return TextureFormat::Unknown;
        }
    }

    D3D12_RESOURCE_FLAGS textureUsageToD3D12Flags(TextureUsage usage)
    {
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;

        if ((usage & TextureUsage::RenderTarget) != TextureUsage::None)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }

        if ((usage & TextureUsage::DepthStencil) != TextureUsage::None)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }

        if ((usage & TextureUsage::UnorderedAccess) != TextureUsage::None)
        {
            flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        return flags;
    }

    D3D12_RESOURCE_STATES textureUsageToD3D12State(TextureUsage usage)
    {
        // �ł���ʓI�ȏ�����Ԃ�Ԃ�
        if ((usage & TextureUsage::RenderTarget) != TextureUsage::None)
        {
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        if ((usage & TextureUsage::DepthStencil) != TextureUsage::None)
        {
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        }

        if ((usage & TextureUsage::CopyDestination) != TextureUsage::None)
        {
            return D3D12_RESOURCE_STATE_COPY_DEST;
        }

        // �f�t�H���g�̓V�F�[�_�[���\�[�X
        return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
}