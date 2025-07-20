//src/Graphics/Material.hpp
#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>
#include "../Math/Math.hpp"
#include "../Utils/Common.hpp"
#include "Device.hpp"

using Microsoft::WRL::ComPtr;

namespace Engine::Graphics
{
	//前方宣言
	class Texture;

	//======================================================================
	//テクスチャタイプ列挙型
	//======================================================================
	enum class TextureType
	{
		Albeno, //ベースカラー（拡散反射色）
		Normal, //法線マップ
		Metallic, //メタリック
		RoughNess,//ラフネス
		AO,      //アンビエントオクルージョン
		Emissive, //発光
		Height,   //高さマップ
		Count  //テクスチャの数の計算用
	};

	//======================================================================
	//マテリアルプロパティ構造体
	//======================================================================
	struct MaterialProperties
	{
		//PBRパラメータ
		Math::Vector3 albeno = Math::Vector3(1.0f, 1.0f, 1.0f);  //ベースカラー
		float metallic = 0.0f;  //メタリック値（0.0 == 非金属, 1.0 == 金属）
		float roughness = 0.5f; //ラフネス値（0.0 == 完全に滑らか, 1.0 == 完全に粗い）
		float ao = 1.0f;  //アンビエントオクルージョン

		//発光
		Math::Vector3 emissive = Math::Vector3(0.0f, 0.0f, 0.0f); //発光色
		float emissiveStrength = 1.0f;  //発光強度

		//その他のプロパティ
		float normalStrength = 1.0f;  //法線マップの強度
		float heightScale = 0.05f;    //高さマップのスケール

		//アルファ関連
		float alpha = 1.0f;    //透明度
		bool useAlphaText = false; //アルファテストを使用するかしないか
		float alphaTestThreshold = 0.5f;  //アルファテストの閾値（いきち）


	};  
}
