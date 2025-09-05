//src/Graphics/Material.cpp
#include "Material.hpp"
//#include "Texture.hpp" // 谺｡縺ｮ繧ｹ繝・ャ繝励〒菴懈・莠亥ｮ・
#include <format>

namespace Engine::Graphics
{
    //=========================================================================
    // Material螳溯｣・
    //=========================================================================

    Material::Material(const std::string& name)
        : m_name(name)
    {
        // 繝・ヵ繧ｩ繝ｫ繝医・繝ｭ繝代ユ繧｣縺ｯ讒矩菴薙・蛻晄悄蛹悶〒險ｭ螳壽ｸ医∩
    }

    Utils::VoidResult Material::initialize(Device* device)
    {
        Utils::log_info(std::format("=== Material::initialize START for '{}' ===", m_name));
        Utils::log_info(std::format("Current m_initialized state: {}", m_initialized));

        if (m_initialized)
        {
            Utils::log_info(std::format("Material '{}' already initialized, returning", m_name));
            return {};
        }

        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;
        Utils::log_info(std::format("Device assigned to material '{}'", m_name));

        // 螳壽焚繝舌ャ繝輔ぃ繧剃ｽ懈・
        Utils::log_info(std::format("Calling createConstantBuffer for '{}'", m_name));
        auto cbResult = createConstantBuffer();
        if (!cbResult)
        {
            Utils::log_warning(std::format("createConstantBuffer failed for '{}': {}", m_name, cbResult.error().message));
            return cbResult;
        }
        Utils::log_info(std::format("createConstantBuffer succeeded for '{}'", m_name));

        // 繝・せ繧ｯ繝ｪ繝励ち繧剃ｽ懈・
        Utils::log_info(std::format("Calling createDescriptors for '{}'", m_name));
        auto descResult = createDescriptors();
        if (!descResult)
        {
            Utils::log_warning(std::format("createDescriptors failed for '{}': {}", m_name, descResult.error().message));
            return descResult;
        }
        Utils::log_info(std::format("createDescriptors succeeded for '{}'", m_name));

        // 蛻晄悄蛹悶ヵ繝ｩ繧ｰ繧定ｨｭ螳・
        Utils::log_info(std::format("Setting m_initialized = true for '{}'", m_name));
        m_initialized = true;
        Utils::log_info(std::format("m_initialized is now: {} for '{}'", m_initialized, m_name));

        Utils::log_info(std::format("=== Material::initialize COMPLETE for '{}' ===", m_name));
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
        Utils::log_info(std::format("=== updateConstantBuffer called for '{}' ===", m_name));
        Utils::log_info(std::format("m_initialized: {}", m_initialized));
        Utils::log_info(std::format("m_device: {}", m_device ? "not null" : "null"));
        Utils::log_info(std::format("m_constantBufferData: {}", m_constantBufferData ? "not null" : "null"));

        // 繧医ｊ隧ｳ邏ｰ縺ｪ蛻晄悄蛹悶メ繧ｧ繝・け
        if (!m_initialized) {
            Utils::log_warning(std::format("Material '{}' not initialized (m_initialized = {})", m_name, m_initialized));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' not initialized", m_name)));
        }

        if (!m_constantBufferData) {
            Utils::log_warning(std::format("Material '{}' constant buffer data is null", m_name));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' constant buffer data is null", m_name)));
        }

        if (!m_device) {
            Utils::log_warning(std::format("Material '{}' device is null", m_name));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' device is null", m_name)));
        }

        if (!m_device->isValid()) {
            Utils::log_warning(std::format("Material '{}' device is not valid", m_name));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Material '{}' device is not valid", m_name)));
        }

        Utils::log_info(std::format("All checks passed, updating constant buffer for '{}'", m_name));

        // GPU逕ｨ縺ｮ螳壽焚繝舌ャ繝輔ぃ繝・・繧ｿ繧呈ｺ門ｙ
        MaterialConstantBuffer cbData{};

        // albedo縺ｨmetallic
        cbData.albedo = Math::Vector4(m_properties.albedo.x, m_properties.albedo.y, m_properties.albedo.z, m_properties.metallic);

        // roughness, ao, emissiveStrength
        cbData.roughnessAoEmissiveStrength = Math::Vector4(m_properties.roughness, m_properties.ao, m_properties.emissiveStrength, 0.0f);

        // emissive縺ｨnormalStrength
        cbData.emissive = Math::Vector4(m_properties.emissive.x, m_properties.emissive.y, m_properties.emissive.z, m_properties.normalStrength);

        // alpha繝代Λ繝｡繝ｼ繧ｿ
        cbData.alphaParams = Math::Vector4(
            m_properties.alpha,
            m_properties.useAlphaTest ? 1.0f : 0.0f,
            m_properties.alphaTestThreshold,
            m_properties.heightScale
        );

        // UV繝医Λ繝ｳ繧ｹ繝輔か繝ｼ繝
        cbData.uvTransform = Math::Vector4(m_properties.uvScale.x, m_properties.uvScale.y, m_properties.uvOffset.x, m_properties.uvOffset.y);

        // 螳壽焚繝舌ャ繝輔ぃ縺ｫ繝・・繧ｿ繧偵さ繝斐・
        memcpy(m_constantBufferData, &cbData, sizeof(MaterialConstantBuffer));

        m_isDirty = false;
        Utils::log_info(std::format("updateConstantBuffer completed successfully for '{}'", m_name));
        return {};
    }

    void Material::bind(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex) const
    {
        if (!m_initialized)
        {
            Utils::log_warning(std::format("Attempting to bind uninitialized material '{}'", m_name));
            return;
        }

        if (!m_constantBuffer)
        {
            Utils::log_warning(std::format("No constant buffer available for material '{}'", m_name));
            return;
        }

        // 螳壽焚繝舌ャ繝輔ぃ繝薙Η繝ｼ繧堤峩謗･繝舌う繝ｳ繝会ｼ医す繝ｳ繝励Ν縺ｪ譁ｹ豕包ｼ・
        commandList->SetGraphicsRootConstantBufferView(
            rootParameterIndex,
            m_constantBuffer->GetGPUVirtualAddress()
        );

        // 繝・ぅ繧ｹ繧ｯ繝ｪ繝励ち繝偵・繝・
        /*
        if (m_cbvDescriptorHeap)
        {
            ID3D12DescriptorHeap* heaps[] = { m_cbvDescriptorHeap.Get() };
            commandList->SetDescriptorHeaps(1, heaps);
            commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, m_cbvGpuHandle);
        }
        */
    }
    Utils::VoidResult Material::createConstantBuffer()
    {
        // 繧医ｊ隧ｳ邏ｰ縺ｪ繝・ヰ繧､繧ｹ繝√ぉ繝・け
        if (!m_device) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Device is null for material '{}'", m_name)));
        }

        if (!m_device->isValid()) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("Device is not valid for material '{}'", m_name)));
        }

        if (!m_device->getDevice()) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                std::format("D3D12 device is null for material '{}'", m_name)));
        }

        const UINT constantBufferSize = (sizeof(MaterialConstantBuffer) + 255) & ~255;

        Utils::log_info(std::format("Creating constant buffer of size {} bytes for material '{}'",
            constantBufferSize, m_name));

        // 繝偵・繝励・繝ｭ繝代ユ繧｣縺ｮ險ｭ螳・
        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
        heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heapProps.CreationNodeMask = 1;
        heapProps.VisibleNodeMask = 1;

        // 繝ｪ繧ｽ繝ｼ繧ｹ險倩ｿｰ蟄・
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

        HRESULT hr = m_device->getDevice()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_constantBuffer));

        if (FAILED(hr)) {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                std::format("Failed to create constant buffer for material '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)), hr));
        }

        Utils::log_info(std::format("D3D12 constant buffer resource created for material '{}'", m_name));

        // 豌ｸ邯夂噪縺ｫ繝槭ャ繝・
        D3D12_RANGE readRange{ 0, 0 };
        hr = m_constantBuffer->Map(0, &readRange, &m_constantBufferData);

        if (FAILED(hr)) {
            m_constantBuffer.Reset();
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                std::format("Failed to map constant buffer for material '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)), hr));
        }

        Utils::log_info(std::format("Constant buffer mapped successfully for material '{}'", m_name));
        return {};
    }
    Utils::VoidResult Material::createDescriptors()
    {
        Utils::log_info(std::format("Creating descriptors for material '{}'", m_name));

        // CBV + SRV逕ｨ繝・ぅ繧ｹ繧ｯ繝ｪ繝励ち繝偵・繝励・菴懈・
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.NumDescriptors = 2; // CBV(1) + SRV(1)
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        srvHeapDesc.NodeMask = 0;

        HRESULT hr = m_device->getDevice()->CreateDescriptorHeap(
            &srvHeapDesc,
            IID_PPV_ARGS(&m_srvDescriptorHeap));

        if (FAILED(hr))
        {
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                std::format("Failed to create SRV descriptor heap for material '{}': HRESULT=0x{:X}",
                    m_name, static_cast<unsigned>(hr)), hr));
        }

        // CBV繧剃ｽ懈・
        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
        cbvDesc.BufferLocation = m_constantBuffer->GetGPUVirtualAddress();
        cbvDesc.SizeInBytes = (sizeof(MaterialConstantBuffer) + 255) & ~255; // 256繝舌う繝医い繝ｩ繧､繝ｳ

        D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = m_srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
        m_device->getDevice()->CreateConstantBufferView(&cbvDesc, cbvHandle);

        // SRV・医ユ繧ｯ繧ｹ繝√Ε・峨ｒ菴懈・
        UINT descriptorSize = m_device->getDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = cbvHandle;
        srvHandle.ptr += descriptorSize;

        // 繝・ヵ繧ｩ繝ｫ繝医〒縺ｯ逋ｽ繝・け繧ｹ繝√Ε縺ｮSRV繧剃ｽ懈・・亥ｾ後〒繝槭ユ繝ｪ繧｢繝ｫ繝槭ロ繝ｼ繧ｸ繝｣繝ｼ縺九ｉ蜿門ｾ暦ｼ・
        // 縺薙％縺ｧ縺ｯ莉ｮ螳溯｣・
        m_srvGpuHandle = m_srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
        m_srvGpuHandle.ptr += descriptorSize; // SRV縺ｮGPU繝上Φ繝峨Ν

        Utils::log_info(std::format("Descriptors created successfully for material '{}'", m_name));
        return {};
    }

    Utils::VoidResult Material::saveToFile(const std::string& filePath) const
    {
        // 蠕後〒螳溯｣・ｼ・SON蠖｢蠑上〒菫晏ｭ倅ｺ亥ｮ夲ｼ・
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
    }

    Utils::VoidResult Material::loadFromFile(const std::string& filePath)
    {
        // 蠕後〒螳溯｣・ｼ・SON蠖｢蠑上°繧芽ｪｭ縺ｿ霎ｼ縺ｿ莠亥ｮ夲ｼ・
        return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Not implemented yet"));
    }

    //=========================================================================
    // MaterialManager螳溯｣・
    //=========================================================================
    Utils::VoidResult MaterialManager::initialize(Device* device)
    {
        if (m_initialized) {
            Utils::log_warning("MaterialManager already initialized");
            return {};
        }

        CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
        CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

        m_device = device;

        // 繝・ヵ繧ｩ繝ｫ繝医・繝・Μ繧｢繝ｫ繧剃ｽ懈・
        auto defaultResult = createDefaultMaterial();
        if (!defaultResult) {
            // 螟ｱ謨励＠縺溷ｴ蜷医・繝・ヰ繧､繧ｹ繧偵Μ繧ｻ繝・ヨ
            m_device = nullptr;
            Utils::log_warning(std::format("Failed to create default material: {}", defaultResult.error().message));
            return defaultResult;
        }

        // 蜈ｨ縺ｦ謌仙粥縺励◆蝣ｴ蜷医・縺ｿ蛻晄悄蛹悶ヵ繝ｩ繧ｰ繧定ｨｭ螳・
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

        // 譌｢縺ｫ蟄伜惠縺吶ｋ蝣ｴ蜷医・譁ｰ縺励＞蜷榊燕繧堤函謌撰ｼ亥・譛峨ｒ驕ｿ縺代ｋ・・
        std::string uniqueName = name;
        int counter = 1;
        while (hasMaterial(uniqueName))
        {
            uniqueName = name + "_" + std::to_string(counter);
            counter++;
        }

        if (uniqueName != name)
        {
            Utils::log_info(std::format("Material '{}' already exists, creating '{}' instead", name, uniqueName));
        }

        auto material = std::make_shared<Material>(uniqueName);
        auto initResult = material->initialize(m_device);
        if (!initResult)
        {
            Utils::log_error(initResult.error());
            return nullptr;
        }

        m_materials[uniqueName] = material;
        Utils::log_info(std::format("Material '{}' created successfully", uniqueName));
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
        // 繝・ヵ繧ｩ繝ｫ繝医・繝・Μ繧｢繝ｫ繧剃ｽ懈・
        m_defaultMaterial = std::make_shared<Material>("DefaultMaterial");

        // Device 縺ｮ譛牙柑諤ｧ繧貞・遒ｺ隱・
        if (!m_device || !m_device->isValid()) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "Device is invalid when creating default material"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "Device is invalid"));
        }

        // Material 繧貞・譛溷喧
        auto initResult = m_defaultMaterial->initialize(m_device);
        if (!initResult) {
            Utils::log_warning(std::format("Failed to initialize default material: {}",
                initResult.error().message));
            m_defaultMaterial.reset();
            return initResult;
        }

        // 蛻晄悄蛹悶′謌仙粥縺励◆蠕後↓繝励Ο繝代ユ繧｣繧定ｨｭ螳・
        MaterialProperties defaultProps;
        defaultProps.albedo = Math::Vector3(0.8f, 0.8f, 0.8f);
        defaultProps.metallic = 0.0f;
        defaultProps.roughness = 0.5f;
        defaultProps.ao = 1.0f;

        // setProperties 繧剃ｽｿ逕ｨ縺励※險ｭ螳夲ｼ亥・驛ｨ縺ｧ updateConstantBuffer 繧ょ他縺ｰ繧後ｋ・・
        m_defaultMaterial->setProperties(defaultProps);

        // 繝槭ユ繝ｪ繧｢繝ｫ繝槭ャ繝励↓逋ｻ骭ｲ
        m_materials["DefaultMaterial"] = m_defaultMaterial;

        Utils::log_info("Default material created successfully");
        return {};
    }

    void Material::setProperties(const MaterialProperties& properties)
    {
        Utils::log_info(std::format("=== setProperties called for '{}' ===", m_name));
        Utils::log_info(std::format("Current m_initialized: {}", m_initialized));
        Utils::log_info(std::format("Device valid: {}", (m_device && m_device->isValid())));

        m_properties = properties;
        m_isDirty = true;

        // 蛻晄悄蛹匁ｸ医∩縺ｮ蝣ｴ蜷医・縺ｿupdateConstantBuffer()繧貞他縺ｶ
        if (m_initialized && m_device && m_device->isValid()) {
            Utils::log_info(std::format("Calling updateConstantBuffer for '{}'", m_name));
            auto result = updateConstantBuffer();
            if (!result) {
                Utils::log_warning(std::format("Failed to update constant buffer for material '{}': {}",
                    m_name, result.error().message));
            }
            else {
                Utils::log_info(std::format("updateConstantBuffer succeeded for '{}'", m_name));
            }
        }
        else {
            Utils::log_info(std::format("Skipping updateConstantBuffer for '{}' - not ready (init:{}, device:{})",
                m_name, m_initialized, (m_device && m_device->isValid())));
        }

        Utils::log_info(std::format("=== setProperties completed for '{}' ===", m_name));
    }


    //=========================================================================
    // 繝ｦ繝ｼ繝・ぅ繝ｪ繝・ぅ髢｢謨ｰ螳溯｣・
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
        return TextureType::Albedo; // 繝・ヵ繧ｩ繝ｫ繝・
    }
}
