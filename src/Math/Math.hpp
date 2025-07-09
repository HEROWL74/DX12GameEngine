#pragma once

#include <cmath>
#include <array>
#include <initializer_list>

namespace Engine::Math
{
	//定数
	constexpr float PI = 3.14159265359f;
	constexpr float PI2 = PI * 2.0f;
	constexpr float PI_HALF = PI * 0.5f;
	constexpr float DEG_TO_RAD = PI / 180.0f;
	constexpr float RAD_TO_DEG = 180.0f / PI;

	//ユーティリティ関数
	template <typename T>
	constexpr T clamp(T value, T min, T max)
	{
		return (value < min) ? min : (value > max) ? max : value;
	}

	constexpr float lerp(float a, float b, float t)
	{
		return a + t * (b - a);
	}

	constexpr float degrees(float radians)
	{
		return radians * RAD_TO_DEG;
	}

	constexpr float radians(float degrees)
	{
		return degrees * DEG_TO_RAD;
	}

	//三次元ベクトルクラス
	class Vector3
	{
	public:
		float x, y ,z;

		//コンストラクタ
		constexpr Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
		constexpr Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
		constexpr Vector3(float value) : x(value), y(value), z(value) {}

		//静的定数
		static constexpr Vector3 zero()    { return Vector3(0.0f, 0.0f, 0.0f); }
		static constexpr Vector3 one()     { return Vector3(1.0f, 1.0f, 1.0f); }
		static constexpr Vector3 up()      { return Vector3(0.0f, 1.0f, 1.0f);}
		static constexpr Vector3 down()    { return Vector3(0.0f, -1.0f, 0.0f); }
		static constexpr Vector3 left()    { return Vector3(-1.0f, 0.0f, 0.0f); }
		static constexpr Vector3 right()   { return Vector3(1.0f, 0.0f, 0.0f); }
		static constexpr Vector3 forward() { return Vector3(0.0f, 0.0f, 1.0f); }
		static constexpr Vector3 back()    { return Vector3(0.0f, 0.0f, -1.0f); }

		//演算子オーバーロード
		constexpr Vector3 operator+(const Vector3& other) const
		{
			return Vector3(x + other.x, y + other.y, z + other.z);
		}

		constexpr Vector3 operator-(const Vector3& other) const
		{
			return Vector3(x - other.x, y - other.y, z - other.z);
		}

		constexpr Vector3 operator*(float scalar) const
		{
			return Vector3(x * scalar, y * scalar, z * scalar);
		}
		constexpr Vector3 operator*(const Vector3& other) const
		{
			return Vector3(x * other.x, y * other.y, z * other.z);
		}

		constexpr Vector3 operator/(float scalar) const
		{
			return Vector3(x / scalar, y / scalar, z / scalar);
		}

		Vector3& operator+=(const Vector3& other)
		{
			x += other.x; y += other.y; z += other.z;
			return *this;
		}

		Vector3& operator-=(const Vector3& other)
		{
			x -= other.x; y -= other.y; z -= other.z;
			return *this;
		}

		Vector3& operator*=(float scalar)
		{
			x *= scalar; y *= scalar; z += scalar;
			return *this;
		}

		constexpr Vector3 operator-() const
		{
			return Vector3(-x, -y, -z);
		}

		//ベクトル操作
		float length () const
		{
			return std::sqrt(x * x + y * y + z * z);
		}
		float lengthSquared() const
		{
			return x * x + y * y + z * z;
		}

		Vector3 normalized() const
		{
			float len = length();
			if (len > 0.0f)
				return *this / len;
			return Vector3::zero();
		}

		void normalize()
		{
			*this = normalized();
		}

		//静的関数
		static float dot(const Vector3& a, const Vector3& b)
		{
			return a.x * b.x + a.y * b.y + a.z * b.z;
		}

		static Vector3 cross(const Vector3& a, const Vector3& b)
		{
			return Vector3(
				a.y * b.z - a.z * b.y,
				a.z * b.x - a.x * b.z,
				a.x * b.y - a.y * b.x
			);
		}

		static Vector3 lerp(const Vector3& a, const Vector3 b, float t)
		{
			return a + (b - a) * t;
		}

		static float distance(const Vector3& a, const Vector3& b)
		{
			return (b - a).length();
		}
	};

	//スカラー * Vector3の演算子
	constexpr Vector3 operator*(float scalar, const Vector3& vector)
	{
		return vector * scalar;
	}

	//4x4行列クラス
	class Matrix4
	{
	public:
		//行列データ（行優先）
		std::array<std::array<float, 4>, 4> m;

		//コンストラクタ
		Matrix4()
		{
			identity();
		}

		Matrix4(std::initializer_list<std::initializer_list<float>> values)
		{
			identity();
			int row = 0;
			for (const auto& rowData : values)
			{
				if (row >= 4) break;
				int col = 0;
				for (float value : rowData)
				{
					if (col >= 4) break;
					m[row][col] = value;
					++col;
				}
				++row;
			}
		}

		//要素アクセス
		float& operator()(int row, int col) { return m[row][col]; }
		const float& operator()(int row, int col) const { return m[row][col]; }

		//単位行列に設定
		void identity()
		{
			for (int i = 0; i < 4; ++i) {
				for (int j = 0; j < 4; ++j) {
					m[i][j] = (i == j) ? 1.0f : 0.0f;
				}
			}
		}

		//行列演算
		Matrix4 operator*(const Matrix4& other)
		{
			Matrix4 result;
			for (int i = 0; i < 4; ++i) 
			{
				for (int j = 0; j < 4; ++j) 
				{
					result.m[i][j] = 0.0f;
					for (int k = 0; k < 4; ++k) 
					{
						result.m[i][j] += m[i][k] * other.m[k][j];
					}
				}
			}
			return result;
		}

		Vector3 transformPoint(const Vector3& point) const
		{
			float w = m[3][0] * point.x + m[3][1] * point.y + m[3][2] * point.z + m[3][3];
			if (w != 0.0f)
			{
				return Vector3(
					(m[0][0] * point.x + m[0][1] * point.y + m[0][2] * point.z + m[0][3]) / w,
					(m[0][0] * point.x + m[1][1] * point.y + m[1][2] * point.z + m[1][3]) / w,
					(m[0][0] * point.x + m[2][1] * point.y + m[2][2] * point.z + m[2][3]) / w
				);
			}
			return Vector3::zero();
		}

		Vector3 transformDirection(const Vector3& direction) const
		{
			return Vector3(
				m[0][0] * direction.x + m[0][1] * direction.y + m[0][2] * direction.z,
				m[1][0] * direction.x + m[1][1] * direction.y + m[1][2] * direction.z,
				m[2][0] * direction.x + m[2][1] * direction.y + m[2][2] * direction.z
			);
		}

		//変換行列の作成
		static Matrix4 translation(const Vector3& position)
		{
			Matrix4 result;
			result.m[0][3] = position.x;
			result.m[1][3] = position.y;
			result.m[2][3] = position.z;
			return result;
		}

		//TODO: 回転関数x,y,zの作成
	};
}
