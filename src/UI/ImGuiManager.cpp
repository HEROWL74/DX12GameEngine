//src/UI/ImGuiManager.cpp
#include "ImGuiManager.hpp"
#include "ProjectWindow.hpp"  // AssetInfoを使う
#include <format>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Engine::UI
{
	//================================================================
	//ImGuiManager実装
	//================================================================
	ImGuiManager::~ImGuiManager()
	{
		shutdown();
	}

	Utils::VoidResult ImGuiManager::initialize(Graphics::Device* device, HWND hwnd, ID3D12CommandQueue* commandQueue, DXGI_FORMAT rtvFormat, UINT frameCount)
	{
		if (m_initialized)
		{
			Utils::log_warning("ImGuiManager already initialized");
			return {};
		}

		// 蜈･蜉帙ヱ繝ｩ繝｡繝ｼ繧ｿ縺ｮ蜴ｳ蟇・↑繝√ぉ繝・け
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(commandQueue != nullptr, Utils::ErrorType::Unknown, "CommandQueue is null");
		CHECK_CONDITION(hwnd != nullptr, Utils::ErrorType::Unknown, "HWND is null");

		// 繝｡繝ｳ繝舌・螟画焚繧貞・縺ｫ險ｭ螳・
		m_device = device;
		m_hwnd = hwnd;
		m_rtvFormat = rtvFormat;
		m_frameCount = frameCount;
		m_commandQueue = commandQueue;  // 竊・驥崎ｦ・ｼ壼ｿ・★nullptr繝√ぉ繝・け蠕後↓險ｭ螳・

		Utils::log_info("Initializing ImGui...");

		try
		{
			// ImGui繧ｳ繝ｳ繝・く繧ｹ繝井ｽ懈・
			IMGUI_CHECKVERSION();
			m_context = ImGui::CreateContext();
			if (!m_context)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to create ImGui context"));
			}

			ImGui::SetCurrentContext(m_context);

			// 險ｭ螳・
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			// 繝・ヵ繧ｩ繝ｫ繝医ヵ繧ｩ繝ｳ繝医ｒ霑ｽ蜉
			io.Fonts->AddFontDefault();

			// 繧ｹ繧ｿ繧､繝ｫ險ｭ螳・
			ImGui::StyleColorsDark();

			// 繝・ぅ繧ｹ繧ｯ繝ｪ繝励ち繝偵・繝嶺ｽ懈・
			auto heapResult = createDescriptorHeap();
			if (!heapResult)
			{
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return heapResult;
			}

			// Win32蛻晄悄蛹・
			if (!ImGui_ImplWin32_Init(hwnd))
			{
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui Win32"));
			}

			// DX12蛻晄悄蛹・- commandQueue縺梧怏蜉ｹ縺ｧ縺ゅｋ縺薙→繧貞・遒ｺ隱・
			if (!m_commandQueue)
			{
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before DX12 init"));
			}

			if (!ImGui_ImplDX12_Init(
				m_device->getDevice(),
				static_cast<int>(frameCount),
				rtvFormat,
				m_srvDescHeap.Get(),
				m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
				m_srvDescHeap->GetGPUDescriptorHandleForHeapStart()))
			{
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui DX12"));
			}

			// 繝輔か繝ｳ繝医ユ繧ｯ繧ｹ繝√Ε繧呈焔蜍穂ｽ懈・
			auto fontResult = createFontTextureManually();
			if (!fontResult)
			{
				ImGui_ImplDX12_Shutdown();
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return fontResult;
			}

			// 蛻晄悄繧ｦ繧｣繝ｳ繝峨え繧ｵ繧､繧ｺ繧定ｨｭ螳・
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
		catch (const std::exception& e)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
				std::format("Exception during ImGui initialization: {}", e.what())));
			shutdown(); // 驛ｨ蛻・噪縺ｫ蛻晄悄蛹悶＆繧後◆繝ｪ繧ｽ繝ｼ繧ｹ繧偵け繝ｪ繝ｼ繝ｳ繧｢繝・・
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ImGui initialization failed"));
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Unknown exception during ImGui initialization"));
			shutdown();
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ImGui initialization failed"));
		}
	}

	// 繝輔か繝ｳ繝医ユ繧ｯ繧ｹ繝√Ε菴懈・縺ｮ蟆ら畑繝｡繧ｽ繝・ラ
	Utils::VoidResult ImGuiManager::createFontTextureManually()
	{
		// commandQueue縺ｮ譛牙柑諤ｧ繧貞・遒ｺ隱・
		if (!m_commandQueue)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue is null in createFontTextureManually"));
		}

		if (!m_device || !m_device->getDevice())
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Device is null in createFontTextureManually"));
		}

		Utils::log_info("Creating font texture manually...");

		try
		{
			// 繝輔か繝ｳ繝医い繝医Λ繧ｹ繧呈ｧ狗ｯ・
			ImGuiIO& io = ImGui::GetIO();
			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

			// 繧ｳ繝槭Φ繝峨い繝ｭ繧ｱ繝ｼ繧ｿ縺ｨ繧ｳ繝槭Φ繝峨Μ繧ｹ繝医ｒ菴懈・
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			ComPtr<ID3D12GraphicsCommandList> commandList;

			CHECK_HR(m_device->getDevice()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
				Utils::ErrorType::ResourceCreation, "Failed to create font command allocator");

			CHECK_HR(m_device->getDevice()->CreateCommandList(
				0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
				IID_PPV_ARGS(&commandList)),
				Utils::ErrorType::ResourceCreation, "Failed to create font command list");

			// ImGui縺ｮ繝・ヰ繧､繧ｹ繧ｪ繝悶ず繧ｧ繧ｯ繝医ｒ菴懈・
			// 縺薙％縺ｧcommandQueue縺御ｽｿ逕ｨ縺輔ｌ繧句庄閭ｽ諤ｧ縺後≠繧九・縺ｧ縲∽ｺ句燕縺ｫ繝√ぉ繝・け
			if (!m_commandQueue)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before CreateDeviceObjects"));
			}

			if (!ImGui_ImplDX12_CreateDeviceObjects())
			{
				Utils::log_warning("ImGui_ImplDX12_CreateDeviceObjects failed, but continuing");
			}

			// 繧ｳ繝槭Φ繝峨Μ繧ｹ繝医ｒ繧ｯ繝ｭ繝ｼ繧ｺ
			commandList->Close();

			// commandQueue縺御ｾ晉┯縺ｨ縺励※譛牙柑縺ｧ縺ゅｋ縺薙→繧堤｢ｺ隱・
			if (!m_commandQueue)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before execution"));
			}

			// 繧ｳ繝槭Φ繝峨く繝･繝ｼ縺ｧ螳溯｡・
			ID3D12CommandList* cmdLists[] = { commandList.Get() };
			m_commandQueue->ExecuteCommandLists(1, cmdLists);

			// GPU螳御ｺ・ｒ蠕・ｩ・
			ComPtr<ID3D12Fence> fence;
			CHECK_HR(m_device->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
				Utils::ErrorType::ResourceCreation, "Failed to create font fence");

			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			CHECK_CONDITION(fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");

			const UINT64 fenceValue = 1;

			// 譛邨ゅメ繧ｧ繝・け
			if (!m_commandQueue)
			{
				CloseHandle(fenceEvent);
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before signal"));
			}

			m_commandQueue->Signal(fence.Get(), fenceValue);
			fence->SetEventOnCompletion(fenceValue, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);

			Utils::log_info("Font texture created manually successfully");
			return {};
		}
		catch (const std::exception& e)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
				std::format("Exception in createFontTextureManually: {}", e.what())));
		}
		catch (...)
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Unknown exception in createFontTextureManually"));
		}
	}

	void ImGuiManager::newFrame()
	{
		if (!m_initialized || !m_context)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "ImGuiManager not initialized in newFrame"));
			return;
		}

		// 豈弱ヵ繝ｬ繝ｼ繝縲∫｢ｺ螳溘↓繧ｳ繝ｳ繝・く繧ｹ繝医ｒ險ｭ螳・
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			Utils::log_info("Setting ImGui context in newFrame");
			ImGui::SetCurrentContext(m_context);
		}

		// 繧ｦ繧｣繝ｳ繝峨え繧ｵ繧､繧ｺ繧呈ｯ弱ヵ繝ｬ繝ｼ繝譖ｴ譁ｰ
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

		try
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();
		}
		catch (const std::exception& e)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
				std::format("Exception in ImGui newFrame: {}", e.what())));
			throw; // 蜀肴兜縺偵＠縺ｦ荳贋ｽ阪〒蜃ｦ逅・
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
				"Unknown exception in ImGui newFrame"));
			throw; // 蜀肴兜縺偵＠縺ｦ荳贋ｽ阪〒蜃ｦ逅・
		}
	}


	void ImGuiManager::shutdown()
	{
		if (!m_initialized)
		{
			return;
		}

		Utils::log_info("Shutting down ImGui...");

		// 繧ｳ繝ｳ繝・く繧ｹ繝医ｒ險ｭ螳壹＠縺ｦ縺九ｉ繧ｷ繝｣繝・ヨ繝繧ｦ繝ｳ
		if (m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		// DX12繝舌ャ繧ｯ繧ｨ繝ｳ繝峨ｒ繧ｷ繝｣繝・ヨ繝繧ｦ繝ｳ
		try
		{
			ImGui_ImplDX12_Shutdown();
		}
		catch (...)
		{
			Utils::log_warning("Exception during ImGui_ImplDX12_Shutdown");
		}

		// Win32繝舌ャ繧ｯ繧ｨ繝ｳ繝峨ｒ繧ｷ繝｣繝・ヨ繝繧ｦ繝ｳ
		try
		{
			ImGui_ImplWin32_Shutdown();
		}
		catch (...)
		{
			Utils::log_warning("Exception during ImGui_ImplWin32_Shutdown");
		}

		// 繧ｳ繝ｳ繝・く繧ｹ繝医ｒ遐ｴ譽・
		if (m_context)
		{
			ImGui::DestroyContext(m_context);
			m_context = nullptr;
		}

		// 繝ｪ繧ｽ繝ｼ繧ｹ繧偵け繝ｪ繧｢
		m_srvDescHeap.Reset();
		m_commandQueue = nullptr;  // 蜿ら・繧偵け繝ｪ繧｢
		m_device = nullptr;
		m_hwnd = nullptr;
		m_initialized = false;

		Utils::log_info("ImGui shutdown completed");
	}



	void ImGuiManager::render(ID3D12GraphicsCommandList* commandList) const
	{
		if (!m_initialized || !m_context)
		{
			Utils::log_warning("ImGuiManager not initialized, skipping render");
			return;
		}

		// 繝ｬ繝ｳ繝繝ｪ繝ｳ繧ｰ蜑阪↓繧ｳ繝ｳ繝・く繧ｹ繝医ｒ遒ｺ隱・
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		ImGui::Render();

		// 繝・せ繧ｯ繝ｪ繝励ち繝偵・繝励ｒ險ｭ螳・
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

	void ImGuiManager::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		if (!m_initialized || !m_context)
		{
			// ImGui縺悟・譛溷喧縺輔ｌ縺ｦ縺・↑縺・ｴ蜷医・菴輔ｂ縺励↑縺・
			return;
		}

		// 驥崎ｦ・ 繝｡繝・そ繝ｼ繧ｸ繝上Φ繝峨Λ繝ｼ繧貞他縺ｶ蜑阪↓蠢・★繧ｳ繝ｳ繝・く繧ｹ繝医ｒ險ｭ螳・
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		// 繝｡繝・そ繝ｼ繧ｸ繧棚mGui縺ｫ霆｢騾・
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
	}

	Utils::VoidResult ImGuiManager::createDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;//ImGui逕ｨ縺ｫ荳縺､
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		CHECK_HR(m_device->getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srvDescHeap)),
			Utils::ErrorType::ResourceCreation, "Failed to create ImGui descriptor heap");

		return {};
	}
	void ImGuiManager::onWindowResize(int width, int height)
	{
		Utils::log_info(std::format("ImGuiManager::onWindowResize: {}x{}", width, height));

		if (!m_initialized || !m_context)
		{
			Utils::log_warning("ImGuiManager not properly initialized");
			return;
		}

		if (width <= 0 || height <= 0)
		{
			Utils::log_warning(std::format("Invalid ImGui resize dimensions: {}x{}", width, height));
			return;
		}

		// 螳牙・縺ｪ繧ｳ繝ｳ繝・く繧ｹ繝育ｮ｡逅・
		ImGuiContext* savedContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(m_context);

		try
		{
			ImGuiIO& io = ImGui::GetIO();

			// 繧ｵ繧､繧ｺ縺悟ｮ滄圀縺ｫ螟峨ｏ縺｣縺溷ｴ蜷医・縺ｿ譖ｴ譁ｰ
			if (std::abs(io.DisplaySize.x - width) > 1.0f || std::abs(io.DisplaySize.y - height) > 1.0f)
			{
				io.DisplaySize = ImVec2(static_cast<float>(width), static_cast<float>(height));
				io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

				Utils::log_info(std::format("ImGui display size updated to: {}x{}", width, height));
			}
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Exception in ImGui resize"));
		}

		// 繧ｳ繝ｳ繝・く繧ｹ繝医ｒ蠕ｩ蜈・
		if (savedContext)
		{
			ImGui::SetCurrentContext(savedContext);
		}
	}


	void ImGuiManager::invalidateDeviceObjects()
	{
		// 菴輔ｂ縺励↑縺・- reinitializeForResize()繧剃ｽｿ逕ｨ縺吶ｋ
		Utils::log_info("invalidateDeviceObjects called - use reinitializeForResize instead");
	}

	void ImGuiManager::createDeviceObjects()
	{
		// 菴輔ｂ縺励↑縺・- reinitializeForResize()繧剃ｽｿ逕ｨ縺吶ｋ
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

		// 迴ｾ蝨ｨ縺ｮ繧ｳ繝ｳ繝・く繧ｹ繝医ｒ菫晄戟
		ImGui::SetCurrentContext(m_context);

		// DX12繝舌ャ繧ｯ繧ｨ繝ｳ繝峨ｒ螳悟・縺ｫ繧ｷ繝｣繝・ヨ繝繧ｦ繝ｳ
		ImGui_ImplDX12_Shutdown();

		// DX12繝舌ャ繧ｯ繧ｨ繝ｳ繝峨ｒ蜀榊・譛溷喧
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

		// 繝輔か繝ｳ繝医ユ繧ｯ繧ｹ繝√Ε繧貞・菴懈・
		auto fontResult = createFontTextureManually();
		if (!fontResult)
		{
			return fontResult;
		}

		Utils::log_info("ImGui reininitialized successfully for resize");
		return {};
	}
	//=====================================================================
	//DebugWindow螳溯｣・
	//=====================================================================
	void DebugWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			// ====== Play Controls ======
			ImGui::Text("Play Controls");
			ImGui::Separator();

			if (ImGui::Button("▶ Play")) {
				if (m_playModeController) {
					m_playModeController->play();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("⏸ Pause")) {
				if (m_playModeController) {
					m_playModeController->pause();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("■ Stop")) {
				if (m_playModeController) {
					m_playModeController->stop();
				}
			}

			ImGui::Spacing();

			// ====== Performance ======
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
	//Scene HierarchyWindow螳溯｣・
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
			// 繧ｪ繝悶ず繧ｧ繧ｯ繝医・譛牙柑諤ｧ繝√ぉ繝・け
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

			// GameObject繧呈緒逕ｻ
			const auto& gameObjects = m_scene->getGameObjects();
			for (const auto& gameObject : gameObjects)
			{
				if (gameObject && gameObject->isActive())
				{
					drawGameObject(gameObject.get());
				}
			}

			// 蜿ｳ繧ｯ繝ｪ繝・け繝｡繝九Η繝ｼ・育ｩｺ逋ｽ驛ｨ蛻・ｼ・
			if (m_contextMenu)
			{
				m_contextMenu->drawHierarchyContextMenu();
			}

			// 繝｢繝ｼ繝繝ｫ繝繧､繧｢繝ｭ繧ｰ繧呈緒逕ｻ・磯㍾隕・ｼ咤egin縺ｮ荳ｭ縺ｧ蜻ｼ縺ｶ・・
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

		// 繝・Μ繝ｼ繝弱・繝峨ヵ繝ｩ繧ｰ險ｭ螳・
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

		// 繝ｦ繝九・繧ｯ縺ｪID繧堤函謌・
		ImGui::PushID(gameObject);
		bool nodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), flags);

		// 繧ｯ繝ｪ繝・け蜃ｦ逅・
		if (ImGui::IsItemClicked())
		{
			m_selectedObject = gameObject;
			if (m_onSelectionChanged)
			{
				m_onSelectionChanged(gameObject);
			}
		}

		// 蜿ｳ繧ｯ繝ｪ繝・け繝｡繝九Η繝ｼ
		if (m_contextMenu)
		{
			m_contextMenu->drawGameObjectContextMenu(gameObject);
		}

		// 蟄舌が繝悶ず繧ｧ繧ｯ繝医ｒ謠冗判
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
	//InspectorWindow
	//=======================================================================
	void InspectorWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			if (m_selectedObject)
			{
				std::string objectName = m_selectedObject->getName();
				ImGui::Text("Object: %s", objectName.c_str());
				ImGui::Separator();

				// Transform / Render はそのまま
				if (auto* transform = m_selectedObject->getTransform())
					drawTransformComponent(transform);

				ImGui::Spacing();

				if (auto* renderComponent = m_selectedObject->getComponent<Graphics::RenderComponent>())
					drawRenderComponent(renderComponent);

				ImGui::Spacing();

				// ==== Script ====
				auto* scriptComponent = m_selectedObject->getComponent<Engine::Scripting::ScriptComponent>();
				if (scriptComponent)
				{
					drawScriptComponent(scriptComponent);
				}
				else
				{
					ImGui::Text("Script");
					ImGui::Dummy(ImVec2(220, 40));
					ImGui::SameLine();
					ImGui::TextDisabled("<None>");
					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
						{
							const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
							if (dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
							{
								m_selectedObject->addScriptComponent(dropped->path);
								Utils::log_info(std::format("Lua script attached: {}", dropped->path));
							}
						}
						ImGui::EndDragDropTarget();
					}

				}
			}
			else
			{
				ImGui::Text("No object selected");
			}
		}
		ImGui::End();
	}



	void InspectorWindow::drawTransformComponent(Core::Transform* transform)
	{
		if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
		{
			
			auto& pos = transform->getPosition();
			float position[3] = { pos.x, pos.y, pos.z };
			if (ImGui::DragFloat3("Position", position, 0.1f))
			{
				transform->setPosition(Math::Vector3(position[0], position[1], position[2]));
			}

			
			auto& rot = transform->getRotation();
			float rotation[3] = { rot.x, rot.y, rot.z };
			if (ImGui::DragFloat3("Rotation", rotation, 1.0f))
			{
				transform->setRotation(Math::Vector3(rotation[0], rotation[1], rotation[2]));
			}

			
			auto& scale = transform->getScale();
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
	
			bool visible = renderComponent->isVisible();
			if (ImGui::Checkbox("Visible", &visible))
			{
				renderComponent->setVisible(visible);
			}

	
			const char* types[] = { "Triangle", "Cube" };
			int currentType = static_cast<int>(renderComponent->getRenderableType());
			if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
			{
				renderComponent->setRenderableType(static_cast<Graphics::RenderableType>(currentType));
			}

		
			auto& color = renderComponent->getColor();
			float colorArray[3] = { color.x, color.y, color.z };
			if (ImGui::ColorEdit3("Color", colorArray))
			{
				renderComponent->setColor(Math::Vector3(colorArray[0], colorArray[1], colorArray[2]));
			}

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

			// 繝槭ユ繝ｪ繧｢繝ｫ驕ｸ謚・
			static char materialNameBuffer[256] = "";
			if (currentMaterial)
			{
				strncpy_s(materialNameBuffer, currentMaterial->getName().c_str(), sizeof(materialNameBuffer));
			}

			if (ImGui::InputText("Material Name", materialNameBuffer, sizeof(materialNameBuffer)))
			{
				// 繝槭ユ繝ｪ繧｢繝ｫ蜷榊､画峩譎ゅ・蜃ｦ逅・
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

			// 繝槭ユ繝ｪ繧｢繝ｫ繝励Ο繝代ユ繧｣邱ｨ髮・
			if (currentMaterial)
			{
				ImGui::Separator();

				auto properties = currentMaterial->getProperties();
				bool changed = false;

				// PBR繝励Ο繝代ユ繧｣
				if (ImGui::CollapsingHeader("PBR Properties", ImGuiTreeNodeFlags_DefaultOpen))
				{
					// 繧｢繝ｫ繝吶ラ
					float albedo[3] = { properties.albedo.x, properties.albedo.y, properties.albedo.z };
					if (ImGui::ColorEdit3("Albedo", albedo))
					{
						properties.albedo = Math::Vector3(albedo[0], albedo[1], albedo[2]);
						changed = true;
					}

					// 繝｡繧ｿ繝ｪ繝・け
					if (ImGui::SliderFloat("Metallic", &properties.metallic, 0.0f, 1.0f))
					{
						changed = true;
					}

					// 繝ｩ繝輔ロ繧ｹ
					if (ImGui::SliderFloat("Roughness", &properties.roughness, 0.0f, 1.0f))
					{
						changed = true;
					}

					// AO
					if (ImGui::SliderFloat("AO", &properties.ao, 0.0f, 1.0f))
					{
						changed = true;
					}

					// 繧｢繝ｫ繝輔ぃ
					if (ImGui::SliderFloat("Alpha", &properties.alpha, 0.0f, 1.0f))
					{
						changed = true;
					}
				}

				// 繧ｨ繝溘ャ繧ｷ繝ｧ繝ｳ
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

				// 繝・け繧ｹ繝√Ε繧ｹ繝ｭ繝・ヨ
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

				// UV險ｭ螳・
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

	void InspectorWindow::drawScriptComponent(Scripting::ScriptComponent* scriptComponent)
	{
		if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
		{
			std::string fileName = scriptComponent->getScriptFileName();

			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Attached Script:");
			ImGui::SameLine();
			ImGui::Text("%s", fileName.c_str());

			// 再アタッチ用のドロップ領域
			ImGui::Dummy(ImVec2(200, 30));
			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ASSET"))
				{
					const AssetPayload* dropped = static_cast<const AssetPayload*>(payload->Data);
					if (dropped->type == static_cast<int>(UI::AssetInfo::Type::Script))
					{
						m_selectedObject->addScriptComponent(dropped->path);
						Utils::log_info(std::format("Lua script re-attached: {}", dropped->path));
					}
				}
				ImGui::EndDragDropTarget();
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

		// 繝・け繧ｹ繝√Ε陦ｨ遉ｺ・医し繝繝阪う繝ｫ・・
		if (currentTexture)
		{
			// 繝・け繧ｹ繝√Ε蜷阪ｒ陦ｨ遉ｺ
			ImGui::Button(currentTexture->getDesc().debugName.c_str(), ImVec2(150, 30));

			// 繝・・繝ｫ繝√ャ繝励〒繝代せ諠・ｱ繧定｡ｨ遉ｺ
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Texture: %s", currentTexture->getDesc().debugName.c_str());
				ImGui::Text("Size: %dx%d", currentTexture->getWidth(), currentTexture->getHeight());
				ImGui::EndTooltip();
			}

			// 繝峨Λ繝・げ&繝峨Ο繝・・蜿励￠蜈･繧・
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
			// ドラッグ場所表示
			ImGui::Button("Drag texture here", ImVec2(150, 30));

			//テクスチャ情報取得
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
