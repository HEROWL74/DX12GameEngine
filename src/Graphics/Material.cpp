//src/Graphics/Material.cpp
#include "Material.hpp"
//#include "Texture.hpp" // ���̃X�e�b�v�ō쐬�\��
#include <format>

namespace Engine::Graphics
{
    //=========================================================================
    // Material����
    //=========================================================================

    Material::Material(const std::string& name)
        : m_name(name)
    {
        // �f�t�H���g�v���p�e�B�͍\���̂̏������Őݒ�ς�
    }

    Utils::VoidResult Material::initialize(Device* device)
    {
        if (m_initialized)
        {
            return {};
        }

        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        // �萔�o�b�t�@���쐬
        auto cbResult = createConstantBuffer();
        if (!cbResult)
        {
            return cbResult;
        }

        // �f�X�N���v�^���쐬
        auto descResult = createDescriptors();
        if (!descResult)
        {
            return descResult;
        }

        // �����f�[�^���X�V
        auto updateResult = updateConstantBuffer();
        if (!updateResult)
        {
            return updateResult;
        }

        m_initialized = true;
        Utils::log_info(std::format("Material '{}' initialized successfully", m_name));
        return {};
    }

    void Material::setTexture(TextureType type, std::shared_ptr<Texture> texture)
    {
        if (texture)
        {
            m_textures[type] = texture;
        }
        else
        {
            removeTexture(type);
        }
        m_isDirty = true;
    }

    std::shared_ptr<Texture> Material::getTexture(TextureType type) const
    {
        auto it = m_textures.find(type);
        return (it != m_textures.end()) ? it->second : nullptr;
    }

    bool Material::hasTexture(TextureType type) const
    {
        return m_textures.find(type) != m_textures.end();
    }

    void Material::removeTexture(TextureType type)
    {
        auto it = m_textures.find(type);
        if (it != m_textures.end())
        {
            m_textures.erase(it);
            m_isDirty = true;
        }
    }

    Utils::VoidResult Material::updateConstantBuffer()
    {
        if (!m_initialized || !m_constantBufferData)
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Material not initialized"));
        }

        // GPU�p�̒萔�o�b�t�@�f�[�^������
        MaterialConstantBuffer cbData{};

        // albedo��metallic
        cbData.albedo = Math::Vector4(m_properties.albedo.x, m_properties.albedo.y, m_properties.albedo.z, m_properties.metallic);

        // roughness, ao, emissiveStrength
        cbData.roughnessAoEmissiveStrength = Math::Vector4(m_properties.roughness, m_properties.ao, m_properties.emissiveStrength, 0.0f);

        // emissive��normalStrength
        cbData.emissive = Math::Vector4(m_properties.emissive.x, m_properties.emissive.y, m_properties.emissive.z, m_properties.normalStrength);

        // alpha�p�����[�^
        cbData.alphaParams = Math::Vector4(
            m_properties.alpha,
            m_properties.useAlphaText ? 1.0f : 0.0f,
            m_properties.alphaTestThreshold,
            m_properties.heightScale
        );

        // UV�g�����X�t�H�[��
        cbData.uvTransform = Math::Vector4(m_properties.uvScale.x, m_properties.uvScale.y, m_properties.uvOffset.x, m_properties.uvOffset.y);

        // �萔�o�b�t�@�Ƀf�[�^���R�s�[
        memcpy(m_constantBufferData, &cbData, sizeof(MaterialConstantBuffer));

        m_isDirty = false;
        return {};
    }

    void Material::bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const
    {
        if (!m_initialized)
        {
            return;
        }

        // �萔�o�b�t�@���o�C���h
        commandList->SetGraphicsRootConstantBufferView(rootParameterIndex, m_constantBuffer->GetGPUVirtualAddress());

        // �e�N�X�`�����o�C���h�i���̃X�e�b�v�Ŏ����j
        // ���݂̓e�N�X�`���V�X�e�����܂��Ȃ��̂ŃX�L�b�v
    }

    Utils::VoidResult Material::createConstantBuffer()
    {
        const UINT constantBufferSize = (sizeof(MaterialConstantBuffer) + 255) & ~255; // 256�o�C�g�A���C�����g

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD; // CPU����A�N�Z�X�\
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Alignment = 0;
        resourceDesc.Width = constantBufferSize;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.SampleDesc.Quality = 0;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        CHECK_HR(m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer)),
            Utils::ErrorType::ResourceCreation,
            std::format("Failed to create constant buffer for material '{}'", m_name));

        // �i���I�Ƀ}�b�v
        D3D12_RANGE readRange{ 0, 0 }; // CPU�ł͓ǂݎ��Ȃ�
        CHECK_HR(m_constantBuffer->Map(0, &readRange, &m_constantBufferData),
            Utils::ErrorType::ResourceCreation,
            std::format("Failed to map constant buffer for material '{}'", m_name));

        return {};
    }

    Utils::VoidResult Material::createDescriptors()
    {
        // ���݂̓V���v���Ɏ���
        // ���̃X�e�b�v�Ńe�N�X�`���V�X�e���ƘA�g���Ċ��S�Ɏ���
        return {};
    }

    Utils::VoidResult Material::saveToFile(const std::string& filePath) const
    {
        // ��Ŏ����iJSON�`���ŕۑ��\��j
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
    }

    Utils::VoidResult Material::loadFromFile(const std::string& filePath)
    {
        // ��Ŏ����iJSON�`������ǂݍ��ݗ\��j
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
    }

    //=========================================================================
    // MaterialManager����
    //=========================================================================

    Utils::VoidResult MaterialManager::initialize(Device* device)
    {
        if (m_initialized)
        {
            return {};
        }

        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        // �f�t�H���g�}�e���A�����쐬
        auto defaultResult = createDefaultMaterial();
        if (!defaultResult)
        {
            return defaultResult;
        }

        m_initialized = true;
        Utils::log_info("MaterialManager initialized successfully");
        return {};
    }

    std::shared_ptr<Material> MaterialManager::createMaterial(const std::string& name)
    {
        if (!m_initialized)
        {
        
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "MaterialManager not initialized"));
            return nullptr;
        }

        // ���ɑ��݂���ꍇ�͊����̂��̂�Ԃ�
        if (hasMaterial(name))
        {
            Utils::log_warning(std::format("Material '{}' already exists", name));
            return getMaterial(name);
        }

        auto material = std::make_shared<Material>(name);
        auto initResult = material->initialize(m_device);
        if (!initResult)
        {
            Utils::log_error(initResult.error());
            return nullptr;
        }

        m_materials[name] = material;
        Utils::log_info(std::format("Material '{}' created successfully", name));
        return material;
    }

    std::shared_ptr<Material> MaterialManager::getMaterial(const std::string& name) const
    {
        auto it = m_materials.find(name);
        return (it != m_materials.end()) ? it->second : nullptr;
    }

    bool MaterialManager::hasMaterial(const std::string& name) const
    {
        return m_materials.find(name) != m_materials.end();
    }

    void MaterialManager::removeMaterial(const std::string& name)
    {
        auto it = m_materials.find(name);
        if (it != m_materials.end())
        {
            m_materials.erase(it);
            Utils::log_info(std::format("Material '{}' removed", name));
        }
    }

    void MaterialManager::updateAllMaterials()
    {
        for (auto& [name, material] : m_materials)
        {
            material->updateConstantBuffer();
        }
    }

    Utils::VoidResult MaterialManager::createDefaultMaterial()
    {
        m_defaultMaterial = std::make_shared<Material>("DefaultMaterial");
        auto initResult = m_defaultMaterial->initialize(m_device);
        if (!initResult)
        {
            return initResult;
        }

        // �f�t�H���g�}�e���A���̃v���p�e�B�ݒ�
        MaterialProperties defaultProps;
        defaultProps.albedo = Math::Vector3(0.8f, 0.8f, 0.8f); // �����O���[
        defaultProps.metallic = 0.0f;
        defaultProps.roughness = 0.5f;
        defaultProps.ao = 1.0f;

        m_defaultMaterial->setProperties(defaultProps);
        m_materials["DefaultMaterial"] = m_defaultMaterial;

        return {};
    }

    //=========================================================================
    // ���[�e�B���e�B�֐�����
    //=========================================================================

    std::string textureTypeToString(TextureType type)
    {
        switch (type)
        {
        case TextureType::Albedo:    return "Albedo";
        case TextureType::Normal:    return "Normal";
        case TextureType::Metallic:  return "Metallic";
        case TextureType::Roughness: return "Roughness";
        case TextureType::AO:        return "AO";
        case TextureType::Emissive:  return "Emissive";
        case TextureType::Height:    return "Height";
        default:                     return "Unknown";
        }
    }

    TextureType stringToTextureType(const std::string& str)
    {
        if (str == "Albedo")    return TextureType::Albedo;
        if (str == "Normal")    return TextureType::Normal;
        if (str == "Metallic")  return TextureType::Metallic;
        if (str == "Roughness") return TextureType::Roughness;
        if (str == "AO")        return TextureType::AO;
        if (str == "Emissive")  return TextureType::Emissive;
        if (str == "Height")    return TextureType::Height;

        Utils::log_warning(std::format("Unknown texture type: {}", str));
        return TextureType::Albedo; // �f�t�H���g
    }
}