//src/UI/ImGuiManager.cpp
#include "ImGuiManager.hpp"
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

	Utils::VoidResult ImGuiManager::initialize(Graphics::Device* device, HWND hwnd, DXGI_FORMAT rtvFormat, UINT frameCount)
	{
		if (m_initialized)
		{
			return {};
		}

		CHECK_CONDITION(device != nullptr, Utils::ErrorType::Unknown, "Device is null");
		CHECK_CONDITION(device->isValid(), Utils::ErrorType::Unknown, "Device is not valid");

		m_device = device;
		m_frameCount = frameCount;

		Utils::log_info("Initializing ImGui...");

		// ImGuiコンテキスト作成
		IMGUI_CHECKVERSION();
		m_context = ImGui::CreateContext();
		ImGui::SetCurrentContext(m_context);

		// 設定
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// **重要: デフォルトフォントを追加**
		io.Fonts->AddFontDefault();

		// **重要: フォントアトラスの事前構築を無効化**
		io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines;

		// スタイル設定
		ImGui::StyleColorsDark();

		// デスクリプタヒープ作成
		auto heapResult = createDescriptorHeap();
		if (!heapResult)
		{
			return heapResult;
		}

		// Win32初期化
		if (!ImGui_ImplWin32_Init(hwnd))
		{
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui Win32"));
		}

		// DX12初期化
		if (!ImGui_ImplDX12_Init(
			m_device->getDevice(),
			static_cast<int>(frameCount),
			rtvFormat,
			m_srvDescHeap.Get(),
			m_srvDescHeap->GetCPUDescriptorHandleForHeapStart(),
			m_srvDescHeap->GetGPUDescriptorHandleForHeapStart()))
		{
			ImGui_ImplWin32_Shutdown(); // 失敗時のクリーンアップ
			return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown, "Failed to initialize ImGui DX12"));
		}

		// フォントテクスチャを事前に作成
		{
			// ダミーのコマンドリストを作成してフォントをアップロード
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

			// フォントアトラスを手動で構築
			unsigned char* pixels;
			int width, height;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

			// フォントテクスチャをアップロード
			ImGui_ImplDX12_CreateDeviceObjects();

			fontCommandList->Close();

			// 即座に実行（同期実行）
			ComPtr<ID3D12CommandQueue> tempQueue;
			D3D12_COMMAND_QUEUE_DESC queueDesc{};
			queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
			queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

			CHECK_HR(m_device->getDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&tempQueue)),
				Utils::ErrorType::DeviceCreation, "Failed to create temp command queue");

			ID3D12CommandList* cmdLists[] = { fontCommandList.Get() };
			tempQueue->ExecuteCommandLists(1, cmdLists);

			// 完了を待機
			ComPtr<ID3D12Fence> tempFence;
			CHECK_HR(m_device->getDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&tempFence)),
				Utils::ErrorType::ResourceCreation, "Failed to create temp fence");

			HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			CHECK_CONDITION(fenceEvent != nullptr, Utils::ErrorType::ResourceCreation, "Failed to create fence event");

			tempQueue->Signal(tempFence.Get(), 1);
			tempFence->SetEventOnCompletion(1, fenceEvent);
			WaitForSingleObject(fenceEvent, INFINITE);
			CloseHandle(fenceEvent);
		}

		m_initialized = true;
		Utils::log_info("ImGui initialized successfully!");
		return {};
	}

	void ImGuiManager::shutdown()
	{
		if (!m_initialized)
		{
			return;
		}

		// GPU作業の完了を待つ
		if (m_device && m_device->isValid())
		{
			// 必要に応じてGPUの完了を待機
		}

		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();

		if (m_context)
		{
			ImGui::DestroyContext(m_context);
			m_context = nullptr;
		}

		m_srvDescHeap.Reset();
		m_initialized = false;

		Utils::log_info("ImGui shutdown complete");
	}

	void ImGuiManager::newFrame()
	{
		if (!m_initialized)
		{
			return;
		}

		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
	}

	void ImGuiManager::render(ID3D12GraphicsCommandList* commandList)
	{
		if (!m_initialized)
		{
			return;
		}

		ImGui::Render();

		//ディスクリプタヒープを設定
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_srvDescHeap.Get() };
		commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
	}

	void ImGuiManager::handleWindowMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (m_initialized)
		{
			ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam);
		}
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
	void SceneHierarchyWindow::draw()
	{
		if (!m_visible || !m_scene) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			const auto& gameObjects = m_scene->getGameObjects();

			for (const auto& gameObject : gameObjects)
			{
				drawGameObject(gameObject.get());
			}
		}
		ImGui::End();
	}

	void SceneHierarchyWindow::drawGameObject(Core::GameObject* gameObject)
	{
		if (!gameObject) return;

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
		if (m_selectedObject == gameObject)
		{
			flags |= ImGuiTreeNodeFlags_Selected;
		}

		if (gameObject->getChildren().empty())
		{
			flags |= ImGuiTreeNodeFlags_Leaf;
		}

		bool nodeOpen = ImGui::TreeNodeEx(gameObject->getName().c_str(), flags);

		if (ImGui::IsItemClicked())
		{
			m_selectedObject = gameObject;
			// 選択変更コールバックを呼び出し
			if (m_onSelectionChanged)
			{
				m_onSelectionChanged(gameObject);
			}
		}

		if (nodeOpen)
		{
			//子オブジェクトを描画
			for (const auto& child : gameObject->getChildren())
			{
				drawGameObject(child.get());
			}
			ImGui::TreePop();
		}
	}

	void SceneHierarchyWindow::setSelectionChangedCallback(std::function<void(Core::GameObject*)> callback)
	{
		m_onSelectionChanged = callback;
	}
	//=======================================================================
	//InspectorWindow実装
	//=======================================================================
	void InspectorWindow::draw()
	{
		if (!m_visible) return;

		if (ImGui::Begin(m_title.c_str(), &m_visible))
		{
			if (m_selectedObject)
			{
				ImGui::Text("Object: %s", m_selectedObject->getName().c_str());
				ImGui::Separator();

				//Transformコンポーネント
				auto* transform = m_selectedObject->getTransform();
				if (transform)
				{
					drawTransformComponent(transform);
				}

				ImGui::Spacing();

				//RenderComponent
				auto* renderComponent = m_selectedObject->getComponent<Graphics::RenderComponent>();
				if (renderComponent)
				{
					drawRenderComponent(renderComponent);
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
			//表示/非表示
			bool visible = renderComponent->isVisible();
			if (ImGui::Checkbox("Visible", &visible))
			{
				renderComponent->setVisible(visible);
			}

			//レンダラブルタイプ
			const char* types[] = { "Triangle", "Cube" };
			int currentType = static_cast<int>(renderComponent->getRenderableType());
			if (ImGui::Combo("Type", &currentType, types, IM_ARRAYSIZE(types)))
			{
				renderComponent->setRenderableType(static_cast<Graphics::RenderableType>(currentType));
			}

			//TODO 色（追加予定のマテリアルシステム用）
			auto color = renderComponent->getColor();
			float colorArray[3] = { color.x, color.y, color.z };
			if (ImGui::ColorEdit3("Color", colorArray))
			{
				renderComponent->setColor(Math::Vector3(colorArray[0], colorArray[1], colorArray[2]));
			}
		}
	}
}