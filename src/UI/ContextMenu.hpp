//src/UI/ContextMenu.hpp
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include "../Core/GameObject.hpp"
#include "../Graphics/RenderComponent.hpp"

namespace Engine::UI
{
	//3D�I�u�W�F�N�g�쐬�̎��
	enum class PrimitiveType
	{
		Cube,
		Sphere,
		Plane,
		Cylinder
	};

	//�R���e�L�X�g���j���[���ڂ̎��
	enum class ContextMenuAction
	{
		CreateCube,
		CreateSphere,
		CreatePlane,
		CreateCylinder,
		DeleteObject,
		DuplicateObject,
		RenameObject
	};

	//�R���e�L�X�g���j���[�N���X
	class ContextMenu
	{
	public:
		ContextMenu() = default;
		~ContextMenu() = default;

		//�E�N���b�N���j���[�\��
		bool drawHierarchyContextMenu();
		bool drawGameObjectContextMenu(Core::GameObject* selectedObject);

		//�R�[���o�b�N�ݒ�
		void setCreateObjectCallback(std::function<Core::GameObject* (PrimitiveType, const std::string&)> callback)
		{
			m_createObjectCallback = callback;
		}

		void setDeleteObjectCallback(std::function<void(Core::GameObject*)> callback)
		{
			m_deleteObjectCallback = callback;
		}

		void setDuplicateObjectCallback(std::function<Core::GameObject* (Core::GameObject*)> callback)
		{
			m_duplicateObjectCallback = callback;
		}

		void setRenameObjectCallback(std::function<void(Core::GameObject*, const std::string&)> callback)
		{
			m_renameObjectCallback = callback;
		}
		void drawModals();
	private:
		//�R�[���o�b�N�֐�
		std::function<Core::GameObject* (PrimitiveType, const std::string&)> m_createObjectCallback;
		std::function<void(Core::GameObject*)> m_deleteObjectCallback;
		std::function<Core::GameObject* (Core::GameObject*)> m_duplicateObjectCallback;
		std::function<void(Core::GameObject*, const std::string&)> m_renameObjectCallback;

		//�������\�b�h
		void drawCreateMenu();
		void draw3DObjectMenu();
		std::string generateUniqueName(const std::string& baseName);

		bool m_showRenameDialog = false;
		bool m_showDeleteConfirm = false;
		char m_renameBuffer[256] = "";
		Core::GameObject* m_renameTarget = nullptr;
		Core::GameObject* m_deleteTarget = nullptr;
	};
}