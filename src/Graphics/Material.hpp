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
	//�O���錾
	class Texture;

	//======================================================================
	//�e�N�X�`���^�C�v�񋓌^
	//======================================================================
	enum class TextureType
	{
		Albeno, //�x�[�X�J���[�i�g�U���ːF�j
		Normal, //�@���}�b�v
		Metallic, //���^���b�N
		RoughNess,//���t�l�X
		AO,      //�A���r�G���g�I�N���[�W����
		Emissive, //����
		Height,   //�����}�b�v
		Count  //�e�N�X�`���̐��̌v�Z�p
	};

	//======================================================================
	//�}�e���A���v���p�e�B�\����
	//======================================================================
	struct MaterialProperties
	{
		//PBR�p�����[�^
		Math::Vector3 albeno = Math::Vector3(1.0f, 1.0f, 1.0f);  //�x�[�X�J���[
		float metallic = 0.0f;  //���^���b�N�l�i0.0 == �����, 1.0 == �����j
		float roughness = 0.5f; //���t�l�X�l�i0.0 == ���S�Ɋ��炩, 1.0 == ���S�ɑe���j
		float ao = 1.0f;  //�A���r�G���g�I�N���[�W����

		//����
		Math::Vector3 emissive = Math::Vector3(0.0f, 0.0f, 0.0f); //�����F
		float emissiveStrength = 1.0f;  //�������x

		//���̑��̃v���p�e�B
		float normalStrength = 1.0f;  //�@���}�b�v�̋��x
		float heightScale = 0.05f;    //�����}�b�v�̃X�P�[��

		//�A���t�@�֘A
		float alpha = 1.0f;    //�����x
		bool useAlphaText = false; //�A���t�@�e�X�g���g�p���邩���Ȃ���
		float alphaTestThreshold = 0.5f;  //�A���t�@�e�X�g��臒l�i�������j


	};  
}
