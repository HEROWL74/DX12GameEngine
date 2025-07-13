//src/Core/GameObject.hpp
#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <typeindex>
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
}


