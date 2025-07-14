//src/Core/GameObject.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeindex>  //ECSでよく使う
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"

namespace Engine::Core
{
	//前方宣言
	class GameObject;
	class Component;
	class Transform;

	//==========================================================
	//Componentベースクラス
	//==========================================================
	class Component
	{
	public:
		Component() = default;
		virtual ~Component() = default;

		//コピー・ムーブ禁止
		Component(const Component&) = delete;
		Component& operator=(const Component&) = delete;
		Component(Component&&) = delete;
		Component& operator=(Component&&) = delete;

		//ライフサイクル
		virtual void start() {}                    //初期化の時に一度だけ呼ぶ
		virtual void update(float deltaTime) {}    //毎フレーム呼ぶ
		virtual void lateUpdate(float deltaTime) {}//updateとあとに呼ぶ
		virtual void onDestroy() {}                //破棄時に呼ぶ

		//ゲームオブジェクトの取得
		GameObject* getGameObject() const { return m_gameObject; }

		//有効か無効か
		bool isEnabled() const { return m_enabled; }
		void setEnabled(bool enabled) { m_enabled = enabled; }

	protected:
		GameObject* m_gameObject = nullptr;
		bool m_enabled = true;

		friend class GameObject;
	};

	//=========================================================
	//Transformコンポーネント（必須コンポーネント）
	//=========================================================
	class Transform : public Component
	{
		Transform() = default;
		~Transform() = default;

		//位置・回転・スケール
		const Math::Vector3& getPosition() const { return m_position; }
		const Math::Vector3& getRotation() const { return m_rotation; }
		const Math::Vector3& getScale() const { return m_scale; }

		void setPosition(const Math::Vector3& position) { m_position = position; m_isDirty = true; }
		void setRotation(const Math::Vector3& rotation) { m_rotation = rotation; m_isDirty = true; }
		void setScale(const Math::Vector3& scale) { m_scale = scale; m_isDirty = true; }

		//移動・回転
		void translate(const Math::Vector3& transition) { m_position += transition; m_isDirty = true; }
		void rotate(const Math::Vector3& rotation) { m_rotation += rotation; m_isDirty = true; }

		//ワールド行列の取得
		const Math::Matrix4& getWorldMatrix();

		//方向ベクトル
		Math::Vector3 getForward() const;
		Math::Vector3 getRight() const;
		Math::Vector3 getUp() const;
	private:
		Math::Vector3 m_position = Math::Vector3::zero();
		Math::Vector3 m_rotation = Math::Vector3::zero();
		Math::Vector3 m_scale = Math::Vector3::one();

		mutable Math::Matrix4 m_worldMatrix;
		mutable bool m_isDirty = true; //isDirtyと書いたのは、状態が最新じゃない場合に処理を追加するため

		void updateWorldMatrix() const;
	};

	//=========================================================
	//ゲームオブジェクトクラス
	//=========================================================
	class GameObject
	{
	public:
		explicit GameObject(const std::string& name = "GameObject");

		~GameObject();

		//コピー・ムーブ禁止
		GameObject(const GameObject&) = delete;
		GameObject& operator=(const GameObject&) = delete;
		GameObject(GameObject&&) = delete;
		GameObject& operator=(GameObject&&) = delete;

		//基本情報
		const std::string& getName() const { return m_name; }
		void setName(const std::string& name) { m_name = name; }

		bool isActive() const { return m_active; }
		void setActive(bool active) { m_active = active; }

		//Transform（必須のコンポーネント）
		Transform* getTransform() const { return m_transform; }

		//コンポーネントの追加・取得・削除
		template<typename T, typename... Args>
		T* addComponent(Args&&... args);

		template<typename T>
		T* getComponent() const;

		template<typename T>
		void removeComponent();

		bool hasComponent(std::type_index type) const;

		template<typename T>
		bool hasComponent() const { return hasComponent(std::type_index(typeid(T))); }

		//ライフサイクル
		void start();
		void update(float deltaTime);
		void lateUpdate(float deltaTime);
		void destroy();

		//子オブジェクト管理
		void addChild(std::unique_ptr<GameObject> child);
		void removeChild(GameObject* child);
		GameObject* findChild(const std::string& name) const;
		const std::vector<std::unique_ptr<GameObject>>& getChildren() const { return m_children; }

		//親オブジェクト
		GameObject* getParent() const { return m_parent; }

	private:
		std::string m_name;
		bool m_active = true;
		bool m_started = false;

		//必須コンポーネント
		Transform* m_transform = nullptr;

		//コンポーネント管理
		std::unordered_map<std::type_index, std::unique_ptr<Component>> m_components;

		//階層構造
		GameObject* m_parent = nullptr;
		std::vector<std::unique_ptr<GameObject>> m_children;
	};

	//=============================================================
	//テンプレート実装
	//=============================================================
	template<typename T, typename... Args>
	T* GameObject::addComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));

		//既に存在する場合は追加しない
		if (hasComponent(typeIndex))
		{
			return static_cast<T*>(m_components[typeIndex].get());
		}

		auto component = std::make_unique<T>(std::forward<Args>(args)...);
		T* componentPtr = component.get();

		component->m_gameObject = this;
		m_components[typeIndex] = std::move(component);

		return componentPtr;

	}

	template<typename T>
	T* GameObject::getComponent() const
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));
		auto it = m_components.find(typeIndex);

		if (it != m_components.end())
		{
			return static_cast<T*>(it->second.get());
		}

		return nullptr;
	}

	template<typename T>
	void GameObject::removeComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

		std::type_index typeIndex(typeid(T));
		auto it = m_components.find(typeIndex);

		if (it != m_components.end())
		{
			it->second->onDestroy();
			m_components.erase(it);
		}
	}
}


