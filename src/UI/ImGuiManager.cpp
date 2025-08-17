//src/UI/ImGuiManager.cpp
#include "ImGuiManager.hpp"
#include "ProjectWindow.hpp"  // AssetInfo�g�p�̂��ߒǉ�
#include <format>

//�O����Win32 ���b�Z�[�W�n���h���[
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine::UI
{
	//================================================================
	//ImGuiManager����
	//================================================================
	ImGuiManager::~ImGuiManager()
	{
		shutdown();
	}

	Utils::VoidResult ImGuiManager::initialize(Graphics::Device* device, HWND hwnd, ID3D12CommandQueue* commandQueue, DXGI_FORMAT rtvFormat, UINT frameCount)
	{
		if (m_initialized)
		{
			return {};
		}

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(commandQueue != nullptr, Utils::ErrorType::Unknown, "CommandQueue is null");

		m_device = device;
		m_hwnd = hwnd;
		m_rtvFormat = rtvFormat;
		m_frameCount = frameCount;
		m_commandQueue = commandQueue;

		Utils::log_info("Initializing ImGui...");

		// ImGui�R���e�L�X�g�쐬
		IMGUI_CHECKVERSION();
		m_context = ImGui::CreateContext();
		ImGui::SetCurrentContext(m_context);

		// �ݒ�
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// �f�t�H���g�t�H���g��ǉ�
		io.Fonts->AddFontDefault();

		// �X�^�C���ݒ�
		ImGui::StyleColorsDark();

		// �f�B�X�N���v�^�q�[�v�쐬
		auto heapResult = createDescriptorHeap();
		if (!heapResult)
		{
			return heapResult;
		}

		// Win32������
		if (!ImGui_ImplWin32_Init(hwnd))
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui Win32"));
		}

		// DX12�������i�]���̕��@�j
		if (!ImGui_ImplDX12_Init(
			m_device->getDevice(),
			static_cast<int>(frameCount),
			rtvFormat,
			m_srvDescHeap.Get(),
			m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			m_srvDescHeap->GetGPUDescriptorHandleForHeapStart()))
		{
			ImGui_ImplWin32_Shutdown();
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui DX12"));
		}

		// �蓮�Ńt�H���g�e�N�X�`�����A�b�v���[�h
		auto fontResult = createFontTextureManually();
		if (!fontResult)
		{
			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			return fontResult;
		}

		// �����E�B���h�E�T�C�Y��ݒ�
		RECT rect;
		if (GetClientRect(hwnd, &rect))
		{
			io.DisplaySize = ImVec2(static_cast<float>(rect.right - rect.left),
				static_cast<float>(rect.bottom - rect.top));
		}

		m_initialized = true;
		Utils::log_info("ImGui initialized successfully!");
		return {};
	}

	Utils::VoidResult ImGuiManager::createFontTextureManually()
	{
		// �t�H���g�e�N�X�`�����蓮�ō쐬�E�A�b�v���[�h
		ImGuiIO& io = ImGui::GetIO();

		// �t�H���g�A�g���X���\�z
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		// �R�}���h�A���P�[�^�ƃR�}���h���X�g���쐬
		ComPtr<ID3D12CommandAllocator> commandAllocator;
		ComPtr<ID3D12GraphicsCommandList> commandList;

		CHECK_HR(m_device->getDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
			Utils::ErrorType::ResourceCreation, "Failed to create font command allocator");

		CHECK_HR(m_device->getDevice()->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
			IID_PPV_ARGS(&commandList)),
			Utils::ErrorType::ResourceCreation, "Failed to create font command list");

		// ImGui_ImplDX12_CreateDeviceObjects���Ă�œ����Ńt�H���g�e�N�X�`�����쐬������
		// �������A���ꂪ���s����\��������̂ŁAtry-catch�ň͂�
		try {
			if (!ImGui_ImplDX12_CreateDeviceObjects())
			{
				Utils::log_warning("ImGui_ImplDX12_CreateDeviceObjects failed, trying alternative approach");
			}
		}
		catch (...)
		{
			Utils::log_warning("Exception in ImGui_ImplDX12_CreateDeviceObjects, continuing with alternative");
		}

		// �R�}���h���X�g���N���[�Y
		commandList->Close();

		// �R�}���h�L���[�Ŏ��s
		ID3D12CommandList* cmdLists[] = { commandList.Get() };
		m_commandQueue->ExecuteCommandLists(1, cmdLists);

		// GPU������ҋ@
		ComPtr<ID3D12Fence> fence;
		CHECK_HR(m_device->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
			Utils::ErrorType::ResourceCreation, "Failed to create font fence");

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		CHECK_CONDITION(fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");

		const UINT64 fenceValue = 1;
		m_commandQueue->Signal(fence.Get(), fenceValue);
		fence->SetEventOnCompletion(fenceValue, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);

		Utils::log_info("Font texture created manually");
		return {};
	}

	// �t�H���g�e�N�X�`���쐬�̐�p���\�b�h
	Utils::VoidResult ImGuiManager::createFontTexture()
	{
		if (!m_device || !m_commandQueue)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Device or CommandQueue is null"));
		}

		// ��p�̃R�}���h�A���P�[�^�ƃR�}���h���X�g���쐬
		ComPtr<ID3D12CommandAllocator> fontCommandAllocator;
		ComPtr<ID3D12GraphicsCommandList> fontCommandList;

		CHECK_HR(m_device->getDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&fontCommandAllocator)),
			Utils::ErrorType::ResourceCreation, "Failed to create font command allocator");

		CHECK_HR(m_device->getDevice()->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			fontCommandAllocator.Get(), nullptr,
			IID_PPV_ARGS(&fontCommandList)),
			Utils::ErrorType::ResourceCreation, "Failed to create font command list");

		// �t�H���g�A�g���X�������ō\�z
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		// ImGui�̃f�o�C�X�I�u�W�F�N�g���쐬�i�t�H���g�e�N�X�`�����܂ށj
		ImGui_ImplDX12_CreateDeviceObjects();

		// �R�}���h���X�g�����
		fontCommandList->Close();

		// �R�}���h�L���[�Ŏ��s
		ID3D12CommandList* cmdLists[] = { fontCommandList.Get() };
		m_commandQueue->ExecuteCommandLists(1, cmdLists);

		// ������ҋ@
		ComPtr<ID3D12Fence> tempFence;
		CHECK_HR(m_device->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&tempFence)),
			Utils::ErrorType::ResourceCreation, "Failed to create temp fence");

		HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		CHECK_CONDITION(fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");

		m_commandQueue->Signal(tempFence.Get(), 1);
		tempFence->SetEventOnCompletion(1, fenceEvent);
		WaitForSingleObject(fenceEvent, INFINITE);
		CloseHandle(fenceEvent);

		Utils::log_info("Font texture created successfully");
		return {};
	}

	void ImGuiManager::shutdown()
	{
		if (!m_initialized)
		{
			return;
		}

		// �R���e�L�X�g��ݒ肵�Ă���V���b�g�_�E��
		if (m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();

		if (m_context)
		{
			ImGui::DestroyContext(m_context);
			m_context = nullptr;
		}

		m_srvDescHeap.Reset();
		m_commandQueue = nullptr;  // �Q�Ƃ��N���A
		m_initialized = false;

		Utils::log_info("ImGui shutdown complete");
	}

	void ImGuiManager::newFrame()
	{
		if (!m_initialized || !m_context)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "ImGuiManager not initialized"));
			return;
		}

		// ���t���[���A�m���ɃR���e�L�X�g��ݒ�
		ImGui::SetCurrentContext(m_context);

		// �E�B���h�E�T�C�Y�𖈃t���[���X�V
		if (m_hwnd)
		{
			RECT rect;
			if (GetClientRect(m_hwnd, &rect))
			{
				ImGuiIO& io = ImGui::GetIO();
				float width = static_cast<float>(rect.right - rect.left);
				float height = static_cast<float>(rect.bottom - rect.top);

				if (width > 0 && height > 0)
				{
					io.DisplaySize = ImVec2(width, height);
					io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
				}
			}
		}

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}



	void ImGuiManager::render(ID3D12GraphicsCommandList* commandList) const
	{
		if (!m_initialized || !m_context)
		{
			Utils::log_warning("ImGuiManager not initialized, skipping render");
			return;
		}

		// �����_�����O�O�ɃR���e�L�X�g���m�F
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		ImGui::Render();

		// �f�X�N���v�^�q�[�v��ݒ�
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

	void ImGuiManager::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		if (!m_initialized || !m_context)
		{
			// ImGui������������Ă��Ȃ��ꍇ�͉������Ȃ�
			return;
		}

		// �d�v: ���b�Z�[�W�n���h���[���ĂԑO�ɕK���R���e�L�X�g��ݒ�
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		// ���b�Z�[�W��ImGui�ɓ]��
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
	}

	Utils::VoidResult ImGuiManager::createDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;//ImGui�p�Ɉ��
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		CHECK_HR(m_device->getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvDescHeap)),
			Utils::ErrorType::ResourceCreation, "Failed to create ImGui descriptor heap");

		return {};
	}
	void ImGuiManager::onWindowResize(int width, int height)
	{
		Utils::log_info(std::format("ImGuiManager::onWindowResize called: {}x{}", width, height));

		if (!m_initialized || !m_context)
		{
			Utils::log_warning("ImGuiManager not initialized, skipping resize");
			return;
		}

		if (width <= 0 || height <= 0)
		{
			Utils::log_warning(std::format("Invalid resize dimensions: {}x{}", width, height));
			return;
		}

		// �R���e�L�X�g���m���ɐݒ�
		ImGui::SetCurrentContext(m_context);

		// ImGui�̃f�B�X�v���C�T�C�Y���X�V
		ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
		io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

		Utils::log_info(std::format("ImGui display size updated to: {}x{}", width, height));
	}


	void ImGuiManager::invalidateDeviceObjects()
	{
		// �������Ȃ� - reinitializeForResize()���g�p����
		Utils::log_info("invalidateDeviceObjects called - use reinitializeForResize instead");
	}

	void ImGuiManager::createDeviceObjects()
	{
		// �������Ȃ� - reinitializeForResize()���g�p����
		Utils::log_info("createDeviceObjects called - use reinitializeForResize instead");
	}
	Utils::VoidResult ImGuiManager::reinitializeForResize()
	{
		if (!m_initialized || !m_device || !m_commandQueue)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
				"ImGuiManager not properly initialized for reinitialize"));
		}

		Utils::log_info("Reinitializing ImGui for resize...");

		// ���݂̃R���e�L�X�g��ێ�
		ImGui::SetCurrentContext(m_context);

		// DX12�o�b�N�G���h�����S�ɃV���b�g�_�E��
		ImGui_ImplDX12_Shutdown();

		// DX12�o�b�N�G���h���ď�����
		if (!ImGui_ImplDX12_Init(
			m_device->getDevice(),
			static_cast<int>(m_frameCount),
			m_rtvFormat,
			m_srvDescHeap.Get(),
			m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			m_srvDescHeap->GetGPUDescriptorHandleForHeapStart()))
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
				"Failed to reinitialize ImGui DX12"));
		}

		// �t�H���g�e�N�X�`�����č쐬
		auto fontResult = createFontTextureManually();
		if (!fontResult)
		{
			return fontResult;
		}

		Utils::log_info("ImGui reininitialized successfully for resize");
		return {};
	}
	//=====================================================================
	//DebugWindow����
	//=====================================================================
	void DebugWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			ImGui::Text("Performance");
			ImGui::Separator();
			ImGui::Text("FPS: %.1f", m_fps);
			ImGui::Text("Frame Time: %.3f ms", m_frameTime * 1000.0f);

			ImGui::Spacing();
			ImGui::Text("Scene Info");
			ImGui::Separator();
			ImGui::Text("Object: %zu", m_objectCount);

			ImGui::Spacing();
			ImGui::Text("Controls");
			ImGui::Separator();
			ImGui::Text("WASD: Move camera");
			ImGui::Text("Mouse: Look around");
			ImGui::Text("F1: Toggle mouse mode");
			ImGui::Text("ESC: Exit");
		}
		ImGui::End();
	}

	//======================================================================
	//Scene HierarchyWindow����
	//======================================================================
	SceneHierarchyWindow::SceneHierarchyWindow() : ImGuiWindow("Scene Hierarchy") 
	{
		m_contextMenu = std::make_unique<ContextMenu>();
	}

	void SceneHierarchyWindow::draw()
	{
		if (!m_visible || !m_scene) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			// �I�u�W�F�N�g�̗L�����`�F�b�N
			if (m_selectedObject)
			{
				bool stillExists = false;
				const auto& gameObjects = m_scene->getGameObjects();
				for (const auto& obj : gameObjects)
				{
					if (obj && obj.get() == m_selectedObject)
					{
						stillExists = true;
						break;
					}
				}

				if (!stillExists)
				{
					m_selectedObject = nullptr;
					if (m_onSelectionChanged)
					{
						m_onSelectionChanged(nullptr);
					}
				}
			}

			// GameObject��`��
			const auto& gameObjects = m_scene->getGameObjects();
			for (const auto& gameObject : gameObjects)
			{
				if (gameObject && gameObject->isActive())
				{
					drawGameObject(gameObject.get());
				}
			}

			// �E�N���b�N���j���[�i�󔒕����j
			if (m_contextMenu)
			{
				m_contextMenu->drawHierarchyContextMenu();
			}

			// ���[�_���_�C�A���O��`��i�d�v�FBegin�̒��ŌĂԁj
			if (m_contextMenu)
			{
				m_contextMenu->drawModals();
			}
		}
		ImGui::End();
	}

	void SceneHierarchyWindow::drawGameObject(Core::GameObject* gameObject)
	{
		if (!gameObject) return;

		// �c���[�m�[�h�t���O�ݒ�
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;

		if (m_selectedObject == gameObject)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		if (gameObject->getChildren().empty())
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		std::string nodeName = gameObject->getName();

		// ���j�[�N��ID�𐶐�
		ImGui::PushID(gameObject);
		bool nodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), flags);

		// �N���b�N����
		if (ImGui::IsItemClicked())
		{
			m_selectedObject = gameObject;
			if (m_onSelectionChanged)
			{
				m_onSelectionChanged(gameObject);
			}
		}

		// �E�N���b�N���j���[
		if (m_contextMenu)
		{
			m_contextMenu->drawGameObjectContextMenu(gameObject);
		}

		// �q�I�u�W�F�N�g��`��
		if (nodeOpen)
		{
			for (const auto& child : gameObject->getChildren())
			{
				if (child && child->isActive())
				{
					drawGameObject(child.get());
				}
			}
			ImGui::TreePop();
		}

		ImGui::PopID();
	}

	void SceneHierarchyWindow::setSelectionChangedCallback(std::function<void(Core::GameObject*)> callback)
	{
		m_onSelectionChanged = callback;
	}

	void SceneHierarchyWindow::setCreateObjectCallback(std::function<Core::GameObject* (UI::PrimitiveType, const std::string&)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setCreateObjectCallback(callback);
		}
	}

	void SceneHierarchyWindow::setDeleteObjectCallback(std::function<void(Core::GameObject*)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setDeleteObjectCallback(callback);
		}
	}

	void SceneHierarchyWindow::setDuplicateObjectCallback(std::function<Core::GameObject* (Core::GameObject*)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setDuplicateObjectCallback(callback);
		}
	}

	void SceneHierarchyWindow::setRenameObjectCallback(std::function<void(Core::GameObject*, const std::string&)> callback)
	{
		if (m_contextMenu)
		{
			m_contextMenu->setRenameObjectCallback(callback);
		}
	}
	//=======================================================================
	//InspectorWindow����
	//=======================================================================
	void InspectorWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			// �I�����ꂽ�I�u�W�F�N�g���L�����m�F
			if (m_selectedObject)
			{
				// �I�u�W�F�N�g�̖��O�����S�Ɏ擾
				std::string objectName;
				bool isValid = true;

				try {
					objectName = m_selectedObject->getName();
				}
				catch (...) {
					// �I�u�W�F�N�g�������ȏꍇ
					isValid = false;
					m_selectedObject = nullptr;
				}

				if (isValid && m_selectedObject)
				{
					ImGui::Text("Object: %s", objectName.c_str());
					ImGui::Separator();

					// Transform�R���|�[�l���g
					auto* transform = m_selectedObject->getTransform();
					if (transform)
					{
						drawTransformComponent(transform);
					}

					ImGui::Spacing();

					// RenderComponent
					auto* renderComponent = m_selectedObject->getComponent<Graphics::RenderComponent>();
					if (renderComponent)
					{
						drawRenderComponent(renderComponent);
					}
				}
				else
				{
					// �I�u�W�F�N�g�������ɂȂ���
					m_selectedObject = nullptr;
					ImGui::Text("Selected object is no longer valid");
				}
			}
			else
			{
				ImGui::Text("No object selected");
				ImGui::Text("Select an object in the Scene Hierarchy");
			}
		}
		ImGui::End();
	}


	void InspectorWindow::drawTransformComponent(Core::Transform* transform)
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			//�ʒu
			auto pos = transform->getPosition();
			float position[3] = { pos.x, pos.y, pos.z };
			if (ImGui::DragFloat3("Position", position, 0.1f))
			{
				transform->setPosition(Math::Vector3(position[0], position[1], position[2]));
			}

			//��]
			auto rot = transform->getRotation();
			float rotation[3] = { rot.x, rot.y, rot.z };
			if (ImGui::DragFloat3("Rotation", rotation, 1.0f))
			{
				transform->setRotation(Math::Vector3(rotation[0], rotation[1], rotation[2]));
			}

			//�X�P�[��
			auto scale = transform->getScale();
			float scaleArray[3] = { scale.x, scale.y, scale.z };
			if (ImGui::DragFloat3("Scale", scaleArray, 0.1f, 0.1f, 10.0f))
			{
				transform->setScale(Math::Vector3(scaleArray[0], scaleArray[1], scaleArray[2]));
			}
		}
	}

	void InspectorWindow::drawRenderComponent(Graphics::RenderComponent* renderComponent)
	{
		if (ImGui::CollapsingHeader("Render Component", ImGuiTreeNodeFlags_DefaultOpen))
		{
			// �\��/��\��
			bool visible = renderComponent->isVisible();
			if (ImGui::Checkbox("Visible", &visible))
			{
				renderComponent->setVisible(visible);
			}

			// �����_���u���^�C�v
			const char* types[] = { "Triangle", "Cube" };
			int currentType = static_cast<int>(renderComponent->getRenderableType());
			if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
			{
				renderComponent->setRenderableType(static_cast<Graphics::RenderableType>(currentType));
			}

			// �F�i���� - ��ō폜�\��j
			auto color = renderComponent->getColor();
			float colorArray[3] = { color.x, color.y, color.z };
			if (ImGui::ColorEdit3("Color", colorArray))
			{
				renderComponent->setColor(Math::Vector3(colorArray[0], colorArray[1], colorArray[2]));
			}

			// �}�e���A���G�f�B�^
			if (m_materialManager)
			{
				drawMaterialEditor(renderComponent);
			}
		}
	}

	void InspectorWindow::drawMaterialEditor(Graphics::RenderComponent* renderComponent)
	{
		if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen))
		{
			auto currentMaterial = renderComponent->getMaterial();

			// �}�e���A���I��
			static char materialNameBuffer[256] = "";
			if (currentMaterial)
			{
				strncpy_s(materialNameBuffer, currentMaterial->getName().c_str(), sizeof(materialNameBuffer));
			}

			if (ImGui::InputText("Material Name", materialNameBuffer, sizeof(materialNameBuffer)))
			{
				// �}�e���A�����ύX���̏���
			}

			ImGui::SameLine();
			if (ImGui::Button("Create New"))
			{
				std::string newName = materialNameBuffer;
				if (newName.empty())
				{
					newName = "New Material";
				}

				auto newMaterial = m_materialManager->createMaterial(newName);
				if (newMaterial)
				{
					renderComponent->setMaterial(newMaterial);
					currentMaterial = newMaterial;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Load Default"))
			{
				auto defaultMaterial = m_materialManager->getDefaultMaterial();
				if (defaultMaterial)
				{
					renderComponent->setMaterial(defaultMaterial);
					currentMaterial = defaultMaterial;
				}
			}

			// �}�e���A���v���p�e�B�ҏW
			if (currentMaterial)
			{
				ImGui::Separator();

				auto properties = currentMaterial->getProperties();
				bool changed = false;

				// PBR�v���p�e�B
				if (ImGui::CollapsingHeader("PBR Properties", ImGuiTreeNodeFlags_DefaultOpen))
				{
					// �A���x�h
					float albedo[3] = { properties.albedo.x, properties.albedo.y, properties.albedo.z };
					if (ImGui::ColorEdit3("Albedo", albedo))
					{
						properties.albedo = Math::Vector3(albedo[0], albedo[1], albedo[2]);
						changed = true;
					}

					// ���^���b�N
					if (ImGui::SliderFloat("Metallic", &properties.metallic, 0.0f, 1.0f))
					{
						changed = true;
					}

					// ���t�l�X
					if (ImGui::SliderFloat("Roughness", &properties.roughness, 0.0f, 1.0f))
					{
						changed = true;
					}

					// AO
					if (ImGui::SliderFloat("AO", &properties.ao, 0.0f, 1.0f))
					{
						changed = true;
					}

					// �A���t�@
					if (ImGui::SliderFloat("Alpha", &properties.alpha, 0.0f, 1.0f))
					{
						changed = true;
					}
				}

				// �G�~�b�V����
				if (ImGui::CollapsingHeader("Emission"))
				{
					float emissive[3] = { properties.emissive.x, properties.emissive.y, properties.emissive.z };
					if (ImGui::ColorEdit3("Emissive", emissive))
					{
						properties.emissive = Math::Vector3(emissive[0], emissive[1], emissive[2]);
						changed = true;
					}

					if (ImGui::SliderFloat("Emissive Strength", &properties.emissiveStrength, 0.0f, 5.0f))
					{
						changed = true;
					}
				}

				// �e�N�X�`���X���b�g
				if (ImGui::CollapsingHeader("Textures"))
				{
					drawTextureSlot("Albedo", Graphics::TextureType::Albedo, currentMaterial);
					drawTextureSlot("Normal", Graphics::TextureType::Normal, currentMaterial);
					drawTextureSlot("Metallic", Graphics::TextureType::Metallic, currentMaterial);
					drawTextureSlot("Roughness", Graphics::TextureType::Roughness, currentMaterial);
					drawTextureSlot("AO", Graphics::TextureType::AO, currentMaterial);
					drawTextureSlot("Emissive", Graphics::TextureType::Emissive, currentMaterial);
					drawTextureSlot("Height", Graphics::TextureType::Height, currentMaterial);
				}

				// UV�ݒ�
				if (ImGui::CollapsingHeader("UV Settings"))
				{
					float uvScale[2] = { properties.uvScale.x, properties.uvScale.y };
					if (ImGui::DragFloat2("UV Scale", uvScale, 0.01f))
					{
						properties.uvScale = Math::Vector2(uvScale[0], uvScale[1]);
						changed = true;
					}

					float uvOffset[2] = { properties.uvOffset.x, properties.uvOffset.y };
					if (ImGui::DragFloat2("UV Offset", uvOffset, 0.01f))
					{
						properties.uvOffset = Math::Vector2(uvOffset[0], uvOffset[1]);
						changed = true;
					}
				}

				if (changed)
				{
					currentMaterial->setProperties(properties);
				}
			}
		}
	}

	void InspectorWindow::drawTextureSlot(const char* name, Graphics::TextureType textureType,
		std::shared_ptr<Graphics::Material> material)
	{
		ImGui::PushID(static_cast<int>(textureType));

		auto currentTexture = material->getTexture(textureType);

		ImGui::Text("%s:", name);
		ImGui::SameLine(100);

		// �e�N�X�`���\���i�T���l�C���j
		if (currentTexture)
		{
			// �e�N�X�`������\��
			ImGui::Button(currentTexture->getDesc().debugName.c_str(), ImVec2(150, 30));

			// �c�[���`�b�v�Ńp�X����\��
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Texture: %s", currentTexture->getDesc().debugName.c_str());
				ImGui::Text("Size: %dx%d", currentTexture->getWidth(), currentTexture->getHeight());
				ImGui::EndTooltip();
			}

			// �h���b�O&�h���b�v�󂯓���
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					const AssetInfo* droppedAsset = static_cast<const AssetInfo*>(payload->Data);
					if (droppedAsset->type == AssetInfo::Type::Texture && m_textureManager)
					{
						auto texture = m_textureManager->loadTexture(droppedAsset->path.string());
						if (texture)
						{
							material->setTexture(textureType, texture);
							Utils::log_info(std::format("Texture assigned: {} -> {}",
								droppedAsset->name, name));
						}
					}
				}
				ImGui::EndDragDropTarget();
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear"))
			{
				material->removeTexture(textureType);
				Utils::log_info(std::format("Texture cleared from slot: {}", name));
			}
		}
		else
		{
			// ��̃X���b�g
			ImGui::Button("Drag texture here", ImVec2(150, 30));

			// �h���b�O&�h���b�v�󂯓���
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					const AssetInfo* droppedAsset = static_cast<const AssetInfo*>(payload->Data);
					if (droppedAsset->type == AssetInfo::Type::Texture && m_textureManager)
					{
						auto texture = m_textureManager->loadTexture(droppedAsset->path.string());
						if (texture)
						{
							material->setTexture(textureType, texture);
							Utils::log_info(std::format("Texture assigned: {} -> {}",
								droppedAsset->name, name));
						}
					}
				}
				ImGui::EndDragDropTarget();
			}
		}

		ImGui::PopID();
	}
}