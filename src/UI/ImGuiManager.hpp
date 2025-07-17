//src/UI/ImGuiManager.hpp
#pragma once

#include <Windows.h>
#include <wrl.h>
#include <d3d12.h>
#include <memory>
#include "../Graphics/Device.hpp"
#include "../Utils/Common.hpp"

//ImGui includes
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

using Microsoft::WRL::ComPtr;
