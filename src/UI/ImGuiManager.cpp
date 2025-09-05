//src/UI/ImGuiManager.cpp
#include "ImGuiManager.hpp"
#include "ProjectWindow.hpp"  // AssetInfo使用のため追加
#include <format>

//外部のWin32 メッセージハンドラー
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

		// 入力パラメータの厳密なチェック
		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");
		CHECK_CONDITION(commandQueue != nullptr, Utils::ErrorType::Unknown, "CommandQueue is null");
		CHECK_CONDITION(hwnd != nullptr, Utils::ErrorType::Unknown, "HWND is null");

		// メンバー変数を先に設定
		m_device = device;
		m_hwnd = hwnd;
		m_rtvFormat = rtvFormat;
		m_frameCount = frameCount;
		m_commandQueue = commandQueue;  // ← 重要：必ずnullptrチェック後に設定

		Utils::log_info("Initializing ImGui...");

		try
		{
			// ImGuiコンテキスト作成
			IMGUI_CHECKVERSION();
			m_context = ImGui::CreateContext();
			if (!m_context)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to create ImGui context"));
			}

			ImGui::SetCurrentContext(m_context);

			// 設定
			ImGuiIO& io = ImGui::GetIO();
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

			// デフォルトフォントを追加
			io.Fonts->AddFontDefault();

			// スタイル設定
			ImGui::StyleColorsDark();

			// ディスクリプタヒープ作成
			auto heapResult = createDescriptorHeap();
			if (!heapResult)
			{
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return heapResult;
			}

			// Win32初期化
			if (!ImGui_ImplWin32_Init(hwnd))
			{
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui Win32"));
			}

			// DX12初期化 - commandQueueが有効であることを再確認
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

			// フォントテクスチャを手動作成
			auto fontResult = createFontTextureManually();
			if (!fontResult)
			{
				ImGui_ImplDX12_Shutdown();
				ImGui_ImplWin32_Shutdown();
				ImGui::DestroyContext(m_context);
				m_context = nullptr;
				return fontResult;
			}

			// 初期ウィンドウサイズを設定
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
			shutdown(); // 部分的に初期化されたリソースをクリーンアップ
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ImGui initialization failed"));
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown, "Unknown exception during ImGui initialization"));
			shutdown();
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "ImGui initialization failed"));
		}
	}

	// フォントテクスチャ作成の専用メソッド
	Utils::VoidResult ImGuiManager::createFontTextureManually()
	{
		// commandQueueの有効性を再確認
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
			// フォントアトラスを構築
			ImGuiIO& io = ImGui::GetIO();
			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

			// コマンドアロケータとコマンドリストを作成
			ComPtr<ID3D12CommandAllocator> commandAllocator;
			ComPtr<ID3D12GraphicsCommandList> commandList;

			CHECK_HR(m_device->getDevice()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)),
				Utils::ErrorType::ResourceCreation, "Failed to create font command allocator");

			CHECK_HR(m_device->getDevice()->CreateCommandList(
				0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr,
				IID_PPV_ARGS(&commandList)),
				Utils::ErrorType::ResourceCreation, "Failed to create font command list");

			// ImGuiのデバイスオブジェクトを作成
			// ここでcommandQueueが使用される可能性があるので、事前にチェック
			if (!m_commandQueue)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before CreateDeviceObjects"));
			}

			if (!ImGui_ImplDX12_CreateDeviceObjects())
			{
				Utils::log_warning("ImGui_ImplDX12_CreateDeviceObjects failed, but continuing");
			}

			// コマンドリストをクローズ
			commandList->Close();

			// commandQueueが依然として有効であることを確認
			if (!m_commandQueue)
			{
				return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "CommandQueue became null before execution"));
			}

			// コマンドキューで実行
			ID3D12CommandList* cmdLists[] = { commandList.Get() };
			m_commandQueue->ExecuteCommandLists(1, cmdLists);

			// GPU完了を待機
			ComPtr<ID3D12Fence> fence;
			CHECK_HR(m_device->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
				Utils::ErrorType::ResourceCreation, "Failed to create font fence");

			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			CHECK_CONDITION(fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");

			const UINT64 fenceValue = 1;

			// 最終チェック
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

		// 毎フレーム、確実にコンテキストを設定
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			Utils::log_info("Setting ImGui context in newFrame");
			ImGui::SetCurrentContext(m_context);
		}

		// ウィンドウサイズを毎フレーム更新
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
			throw; // 再投げして上位で処理
		}
		catch (...)
		{
			Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
				"Unknown exception in ImGui newFrame"));
			throw; // 再投げして上位で処理
		}
	}


	void ImGuiManager::shutdown()
	{
		if (!m_initialized)
		{
			return;
		}

		Utils::log_info("Shutting down ImGui...");

		// コンテキストを設定してからシャットダウン
		if (m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		// DX12バックエンドをシャットダウン
		try
		{
			ImGui_ImplDX12_Shutdown();
		}
		catch (...)
		{
			Utils::log_warning("Exception during ImGui_ImplDX12_Shutdown");
		}

		// Win32バックエンドをシャットダウン
		try
		{
			ImGui_ImplWin32_Shutdown();
		}
		catch (...)
		{
			Utils::log_warning("Exception during ImGui_ImplWin32_Shutdown");
		}

		// コンテキストを破棄
		if (m_context)
		{
			ImGui::DestroyContext(m_context);
			m_context = nullptr;
		}

		// リソースをクリア
		m_srvDescHeap.Reset();
		m_commandQueue = nullptr;  // 参照をクリア
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

		// レンダリング前にコンテキストを確認
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		ImGui::Render();

		// デスクリプタヒープを設定
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

	void ImGuiManager::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) const
	{
		if (!m_initialized || !m_context)
		{
			// ImGuiが初期化されていない場合は何もしない
			return;
		}

		// 重要: メッセージハンドラーを呼ぶ前に必ずコンテキストを設定
		ImGuiContext* currentContext = ImGui::GetCurrentContext();
		if (currentContext != m_context)
		{
			ImGui::SetCurrentContext(m_context);
		}

		// メッセージをImGuiに転送
		ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
	}

	Utils::VoidResult ImGuiManager::createDescriptorHeap()
	{
		D3D12_DESCRIPTOR_HEAP_DESC desc{};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;//ImGui用に一つ
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

		// 安全なコンテキスト管理
		ImGuiContext* savedContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(m_context);

		try
		{
			ImGuiIO& io = ImGui::GetIO();

			// サイズが実際に変わった場合のみ更新
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

		// コンテキストを復元
		if (savedContext)
		{
			ImGui::SetCurrentContext(savedContext);
		}
	}


	void ImGuiManager::invalidateDeviceObjects()
	{
		// 何もしない - reinitializeForResize()を使用する
		Utils::log_info("invalidateDeviceObjects called - use reinitializeForResize instead");
	}

	void ImGuiManager::createDeviceObjects()
	{
		// 何もしない - reinitializeForResize()を使用する
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

		// 現在のコンテキストを保持
		ImGui::SetCurrentContext(m_context);

		// DX12バックエンドを完全にシャットダウン
		ImGui_ImplDX12_Shutdown();

		// DX12バックエンドを再初期化
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

		// フォントテクスチャを再作成
		auto fontResult = createFontTextureManually();
		if (!fontResult)
		{
			return fontResult;
		}

		Utils::log_info("ImGui reininitialized successfully for resize");
		return {};
	}
	//=====================================================================
	//DebugWindow実装
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
	//Scene HierarchyWindow実装
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
			// オブジェクトの有効性チェック
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

			// GameObjectを描画
			const auto& gameObjects = m_scene->getGameObjects();
			for (const auto& gameObject : gameObjects)
			{
				if (gameObject && gameObject->isActive())
				{
					drawGameObject(gameObject.get());
				}
			}

			// 右クリックメニュー（空白部分）
			if (m_contextMenu)
			{
				m_contextMenu->drawHierarchyContextMenu();
			}

			// モーダルダイアログを描画（重要：Beginの中で呼ぶ）
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

		// ツリーノードフラグ設定
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

		// ユニークなIDを生成
		ImGui::PushID(gameObject);
		bool nodeOpen = ImGui::TreeNodeEx(nodeName.c_str(), flags);

		// クリック処理
		if (ImGui::IsItemClicked())
		{
			m_selectedObject = gameObject;
			if (m_onSelectionChanged)
			{
				m_onSelectionChanged(gameObject);
			}
		}

		// 右クリックメニュー
		if (m_contextMenu)
		{
			m_contextMenu->drawGameObjectContextMenu(gameObject);
		}

		// 子オブジェクトを描画
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
	//InspectorWindow実装
	//=======================================================================
	void InspectorWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			// 選択されたオブジェクトが有効か確認
			if (m_selectedObject)
			{
				// オブジェクトの名前を安全に取得
				std::string objectName;
				bool isValid = true;

				try {
					objectName = m_selectedObject->getName();
				}
				catch (...) {
					// オブジェクトが無効な場合
					isValid = false;
					m_selectedObject = nullptr;
				}

				if (isValid && m_selectedObject)
				{
					ImGui::Text("Object: %s", objectName.c_str());
					ImGui::Separator();

					// Transformコンポーネント
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
					// オブジェクトが無効になった
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
			//位置
			auto pos = transform->getPosition();
			float position[3] = { pos.x, pos.y, pos.z };
			if (ImGui::DragFloat3("Position", position, 0.1f))
			{
				transform->setPosition(Math::Vector3(position[0], position[1], position[2]));
			}

			//回転
			auto rot = transform->getRotation();
			float rotation[3] = { rot.x, rot.y, rot.z };
			if (ImGui::DragFloat3("Rotation", rotation, 1.0f))
			{
				transform->setRotation(Math::Vector3(rotation[0], rotation[1], rotation[2]));
			}

			//スケール
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
			// 表示/非表示
			bool visible = renderComponent->isVisible();
			if (ImGui::Checkbox("Visible", &visible))
			{
				renderComponent->setVisible(visible);
			}

			// レンダラブルタイプ
			const char* types[] = { "Triangle", "Cube" };
			int currentType = static_cast<int>(renderComponent->getRenderableType());
			if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
			{
				renderComponent->setRenderableType(static_cast<Graphics::RenderableType>(currentType));
			}

			// 色（旧式 - 後で削除予定）
			auto& color = renderComponent->getColor();
			float colorArray[3] = { color.x, color.y, color.z };
			if (ImGui::ColorEdit3("Color", colorArray))
			{
				renderComponent->setColor(Math::Vector3(colorArray[0], colorArray[1], colorArray[2]));
			}

			// マテリアルエディタ
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

			// マテリアル選択
			static char materialNameBuffer[256] = "";
			if (currentMaterial)
			{
				strncpy_s(materialNameBuffer, currentMaterial->getName().c_str(), sizeof(materialNameBuffer));
			}

			if (ImGui::InputText("Material Name", materialNameBuffer, sizeof(materialNameBuffer)))
			{
				// マテリアル名変更時の処理
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

			// マテリアルプロパティ編集
			if (currentMaterial)
			{
				ImGui::Separator();

				auto properties = currentMaterial->getProperties();
				bool changed = false;

				// PBRプロパティ
				if (ImGui::CollapsingHeader("PBR Properties", ImGuiTreeNodeFlags_DefaultOpen))
				{
					// アルベド
					float albedo[3] = { properties.albedo.x, properties.albedo.y, properties.albedo.z };
					if (ImGui::ColorEdit3("Albedo", albedo))
					{
						properties.albedo = Math::Vector3(albedo[0], albedo[1], albedo[2]);
						changed = true;
					}

					// メタリック
					if (ImGui::SliderFloat("Metallic", &properties.metallic, 0.0f, 1.0f))
					{
						changed = true;
					}

					// ラフネス
					if (ImGui::SliderFloat("Roughness", &properties.roughness, 0.0f, 1.0f))
					{
						changed = true;
					}

					// AO
					if (ImGui::SliderFloat("AO", &properties.ao, 0.0f, 1.0f))
					{
						changed = true;
					}

					// アルファ
					if (ImGui::SliderFloat("Alpha", &properties.alpha, 0.0f, 1.0f))
					{
						changed = true;
					}
				}

				// エミッション
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

				// テクスチャスロット
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

				// UV設定
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

		// テクスチャ表示（サムネイル）
		if (currentTexture)
		{
			// テクスチャ名を表示
			ImGui::Button(currentTexture->getDesc().debugName.c_str(), ImVec2(150, 30));

			// ツールチップでパス情報を表示
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Texture: %s", currentTexture->getDesc().debugName.c_str());
				ImGui::Text("Size: %dx%d", currentTexture->getWidth(), currentTexture->getHeight());
				ImGui::EndTooltip();
			}

			// ドラッグ&ドロップ受け入れ
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
			// 空のスロット
			ImGui::Button("Drag texture here", ImVec2(150, 30));

			// ドラッグ&ドロップ受け入れ
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