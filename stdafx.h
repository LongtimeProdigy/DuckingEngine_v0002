#pragma once

#ifndef __DEFINE_STDAFX__
#define __DEFINE_STDAFX__

/*
* DEV 관련
*/
#if defined(_DEBUG)
#define _DK_DEBUG_
#endif

/*
* Platform 관련
*/
#if defined(_WIN64)
#define _DK_WINDOW_
#endif

/*
* 메모리 관련
*/
#define dk_new new
#define dk_delete delete
#define dk_delete_array delete[]

/*
* SAL 및 inline 관련
*/
#include <sal.h>
#define _IN_ _In_
#define _OUT_ _Out_
#define dk_inline inline

/*
* 타입 관련
*/
typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;
typedef unsigned int uint32;
typedef unsigned long long uint64;

/*
* Logger 관련
*/
#ifdef _DK_DEBUG_
#include <stdio.h>		// for sprintf_s(), strcat_s();
#include <intrin.h>		// for __debugbreak();
#ifdef _DK_WINDOW_
/////////////////////// #todo- 나중에 꼭 삭제할 것 /////////////////////// 
#include <Windows.h>	// for OutputDebugStringA
/////////////////////// #todo- 나중에 꼭 삭제할 것 /////////////////////// 
#endif
namespace DK
{
#define MAX_LOG_BUFFER_LENGTH 32768
#define DK_LOG(text, ...)												\
{																		\
	char buffer[MAX_LOG_BUFFER_LENGTH];									\
	sprintf_s(buffer, MAX_LOG_BUFFER_LENGTH, text, __VA_ARGS__);		\
	strcat_s(buffer, MAX_LOG_BUFFER_LENGTH, "\n");						\
	OutputDebugStringA(buffer);											\
}

#define DK_WLOG(text, ...)												\
{																		\
	wchar_t buffer[MAX_LOG_BUFFER_LENGTH];								\
	swprintf_s(buffer, MAX_LOG_BUFFER_LENGTH, text, __VA_ARGS__);		\
	wcscat_s(buffer, MAX_LOG_BUFFER_LENGTH, L"\n");						\
	OutputDebugStringW(buffer);											\
}

#define DK_ASSERT_LOG(condition, text, ...)								\
{																		\
	if(!(condition))													\
	{																	\
		DK_LOG(text, __VA_ARGS__);										\
		__debugbreak();													\
	}																	\
}

#define DK_ASSERT_WLOG(condition, text, ...)							\
{																		\
	if(!(condition))													\
	{																	\
		DK_WLOG(text, __VA_ARGS__);										\
		__debugbreak();													\
	}																	\
}
#endif
}

/*
* #todo- 외부 라이브러리 목록 (필요시 나중에 따로 자체제작 할 목록들)
*/
#define USE_IMGUI
#ifdef USE_IMGUI
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#endif
#pragma endregion

#define USE_ASSIMP
#ifdef USE_ASSIMP
#include "assimp/Importer.hpp"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#endif
#pragma endregion

#ifdef _DK_DEBUG_
#define USE_PIX
#endif

#define USE_DIRECTX
#ifdef USE_DIRECTX
#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>
#pragma comment(lib, "dxgi.lib")
#include <dxgi1_6.h>
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>
#include "d3dx12.h"
#endif

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
namespace DK
{
#define DKVector std::vector
#define DKHashMap std::unordered_map
#define DKHashSet std::unordered_set
#define DKPair std::pair
}

#define USE_TINYXML
#ifdef USE_TINYXML
#include "tinyxml.h"
#else
static_assert(false, "XML 파싱 Class 정의가 필요합니다.");
#endif

/*
* ETC Helper 함수
*/
namespace DK
{
#define DK_ARRAYSIZE_OF ARRAYSIZE

	template <typename T>
	dk_inline void swap(T&& lhs, T&& rhs) noexcept
	{
		std::swap(lhs, rhs);
	}

	dk_inline void* memcpy(void* dest, void const* src, uint32 size)
	{
		return ::memcpy(dest, src, size);
	}

	template<typename T>
	dk_inline T&& move(T& value)
	{
		return std::move(value);
	}

	dk_inline float atof(const char* string)
	{
		return static_cast<float>(::atof(string));
	}

	dk_inline int atoi(const char* string)
	{
		return ::atoi(string);
	}
}

/*
* String 함수
*/
namespace DK
{
	using DKString = std::string;
#define DK_MAX_PATH MAX_PATH

	class StringUtil
	{
	public:
		static float atof(const char* str) noexcept
		{
			return static_cast<float>(std::atof(str));
		}
		static bool strcmp(const char* str1, const char* str2) noexcept
		{
			return std::strcmp(str1, str2) == 0;
		}
		static uint32 strlen(const char* str)
		{
			return static_cast<uint32>(std::strlen(str));
		}
		static char* strcpy(char* dest, const char* src)
		{
#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
			return ::strcpy(dest, src);
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
		}
		static char* strcat(char* dest, const char* src)
		{
#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
			return ::strcat(dest, src);
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
		}

		//wchar_t 에서 char 로의 형변환 함수
		static DKString convertWCtoC(wchar_t* str)
		{
			DKVector<char> pStr;
			int strSize = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
			pStr.resize(strSize);
			WideCharToMultiByte(CP_ACP, 0, str, -1, pStr.data(), strSize, 0, 0);
			return pStr.data();
		}

		///////////////////////////////////////////////////////////////////////
		//char 에서 wchar_t 로의 형변환 함수
		//static DKStringW ConverCtoWC(char* str)
		//{
		//	//wchar_t형 변수 선언
		//	wchar_t* pStr;
		//	//멀티 바이트 크기 계산 길이 반환
		//	int strSize = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, NULL);
		//	//wchar_t 메모리 할당
		//	pStr = new WCHAR[strSize];
		//	//형 변환
		//	MultiByteToWideChar(CP_ACP, 0, str, strlen(str) + 1, pStr, strSize);
		//	return pStr;
		//}
	};

	class StringSplitter
	{
	public:
		StringSplitter() = delete;
		StringSplitter(const DKString& str, const DKString& delimiter)
		{
			size_t start = 0U;
			size_t end = str.find(delimiter);
			while (end != DKString::npos) {
				_container.push_back(str.substr(start, end - start));
				start = end + delimiter.length();
				end = str.find(delimiter, start);
			}
			_container.push_back(str.substr(start, end));
		}

		const DKString& operator[](const uint32 index) const
		{
			return _container[index];
		}

	private:
		DKVector<DKString> _container;
	};

	template <uint SIZE>
	class ScopeString
	{
	public:
		ScopeString(const char* str)
		{
			StringUtil::strcpy(_string, str);
		}

		const char* c_str() const
		{
			return _string;
		}

		void append(const char* str)
		{
#ifdef _DK_DEBUG_
			const uint32 length = StringUtil::strlen(_string) + StringUtil::strlen(str);
			DK_ASSERT_LOG(length < SIZE, "Size를 넘어갑니다. %d/%d", length, SIZE);
#endif
			StringUtil::strcat(_string, str);
		}

	private:
		char _string[SIZE];
	};
}

/*
* Global Path
*/
namespace DK
{
	class GlobalPath
	{
	public:
		static DKString kResourcePath;

	public:
		static ScopeString<DK_MAX_PATH> makeResourceFullPath(const DKString& resourcePath)
		{
			ScopeString<DK_MAX_PATH> resourceFullPath(GlobalPath::kResourcePath.c_str());
			resourceFullPath.append("/");
			resourceFullPath.append(resourcePath.c_str());

			return resourceFullPath;
		}
	};
}

/*
* Global Variable
*/

/*
* Refleciton 관련
*/
namespace DK
{
	enum class ReflectionFlag : uint8
	{
		NoSave, 
	};

#define DK_REFLECTION_PROPERTY(type, name) \
	type name; \
public: \
	dk_inline const type& get##name() const noexcept { return name; } \
	dk_inline type& get##name##Writable() noexcept { return name; } \
	dk_inline void set##name(const type& value) noexcept { name = value; }

#define DK_REFLECTION_PTR_PROPERTY(type, name) \
	Ptr<type> name; \
public: \
	dk_inline const Ptr<type>& get##name() const noexcept { return name; } \
	dk_inline Ptr<type>& get##name##Writable() noexcept { return name; } \
	dk_inline void set##name(type* value) noexcept { name.assign(value); }

#define DK_REFLECTION_PTR_PROPERTY_FLAG(type, name, flag) \
	Ptr<type> name; \
public: \
	dk_inline const Ptr<type>& get##name() const noexcept { return name; } \
	dk_inline Ptr<type>& get##name##Writable() noexcept { return name; } \
	dk_inline void set##name(type* value) noexcept { name.assign(value); }

#define DK_REFLECTION_VECTOR_PROPERTY(type, name) \
	DKVector<type> name; \
public: \
	dk_inline const DKVector<type>& get##name() const noexcept { return name; } \
	dk_inline DKVector<type>& get##name##Writable() noexcept { return name; } \
	dk_inline void move##name(DKVector<type>&& rhs) noexcept{ name = DK::move(rhs); }
}

/*
* RAII 관련
*/
namespace DK
{
#define DEFINE_REFCOUNTED_STRCUT(name) struct name; using name##Ref = std::shared_ptr<name>;
#define DEFINE_REFCOUNTED(name) class name; using name##Ref = std::shared_ptr<name>;
	DEFINE_REFCOUNTED_STRCUT(AppearanceData);
	DEFINE_REFCOUNTED(ModelProperty);
	DEFINE_REFCOUNTED(StaticMeshModel);
	DEFINE_REFCOUNTED(SkinnedMeshModel);
	DEFINE_REFCOUNTED(Skeleton);
	DEFINE_REFCOUNTED(Animation);
	DEFINE_REFCOUNTED(ITexture);

#ifdef USE_DIRECTX
	using VertexBufferViewRef = std::shared_ptr<D3D12_VERTEX_BUFFER_VIEW>;
	using IndexBufferViewRef = std::shared_ptr<D3D12_INDEX_BUFFER_VIEW>;
#endif

	template <typename T>
	class Ptr
	{
	public:
		Ptr()
		{
			_ptr = nullptr;
		}
		~Ptr()
		{
			release();
		}

		Ptr(T* ptr)
			: _ptr(ptr)
		{}

		Ptr(Ptr& rhs) : _ptr(rhs._ptr)
		{
			rhs._ptr = nullptr;
		}
		Ptr(Ptr&& rhs) : _ptr(rhs._ptr)
		{
			rhs._ptr = nullptr;
		}

		dk_inline const Ptr& operator=(Ptr& rhs)
		{
			_ptr = rhs._ptr;
			rhs._ptr = nullptr;

			return *this;
		}
		dk_inline const Ptr& operator=(Ptr&& rhs)
		{
			_ptr = rhs._ptr;
			rhs._ptr = nullptr;

			return *this;
		}
		dk_inline T* operator->() noexcept
		{
			return _ptr;
		}
		dk_inline const T* operator->() const noexcept
		{
			return _ptr;
		}

		dk_inline void assign(T* ptr) noexcept
		{
			release();
			_ptr = ptr;
		}
		dk_inline T* relocate() noexcept
		{
			T* ptr = _ptr;
			_ptr = nullptr;
			return ptr;
		}
		dk_inline void release() noexcept
		{
			dk_delete _ptr;
		}
		dk_inline T* get() noexcept
		{
			return _ptr;
		}
		dk_inline const T* get() const noexcept
		{
			return _ptr;
		}
		dk_inline void swap(Ptr& rhs) noexcept
		{
			T* temp = rhs._ptr;
			rhs._ptr = _ptr;
			_ptr = temp;
		}

	private:
		T* _ptr;
	};

	template <typename T>
	class RenderResourcePtr
	{
	public:
		dk_inline RenderResourcePtr()
		{
			_ptr = nullptr;
		}
		dk_inline ~RenderResourcePtr();

		dk_inline RenderResourcePtr(T* ptr)
			: _ptr(ptr)
		{}
		RenderResourcePtr(RenderResourcePtr& rhs) : _ptr(rhs._ptr)
		{
			rhs._ptr = nullptr;
		}
		RenderResourcePtr(RenderResourcePtr&& rhs) : _ptr(rhs._ptr)
		{
			rhs._ptr = nullptr;
		}

		dk_inline const RenderResourcePtr& operator=(RenderResourcePtr& rhs)
		{
			_ptr = rhs._ptr;
			rhs._ptr = nullptr;

			return *this;
		}
		dk_inline const RenderResourcePtr& operator=(RenderResourcePtr&& rhs)
		{
			_ptr = rhs._ptr;
			rhs._ptr = nullptr;

			return *this;
		}

		dk_inline T* operator->() noexcept
		{
			return _ptr;
		}
		dk_inline const T* operator->() const noexcept
		{
			return operator->();
		}

		dk_inline T* get() noexcept
		{
			return _ptr;
		}
		dk_inline T** getAddress() noexcept
		{
			return &_ptr;
		}

	private:
		T* _ptr;
	};
}

/*
* Math Utility
*/
#define USE_DIRECTX_MATH
#ifdef USE_DIRECTX_MATH
#include <DirectXMath.h>
#endif
namespace DK
{
	struct float2
	{
		static const float2 Identity;
		static const float2 Zero;

		float2()
			: m{ 0, 0 }
		{}
		float2(const float _x)
			: m{ _x, _x }
		{}
		float2(const float _x, const float _y)
			: m{ _x, _y }
		{}

		dk_inline float2 operator+(const float2& rhs) const
		{
			return float2(x + rhs.x, y + rhs.y);
		}
		dk_inline float2 operator-(const float2& rhs) const
		{
			return float2(x - rhs.x, y - rhs.y);
		}
		dk_inline float2 operator*(const float2& rhs) const
		{
			return float2(x * rhs.x, y * rhs.y);
		}
		dk_inline float2 operator*(const float& rhs) const
		{
			return float2(x * rhs, y * rhs);
		}
		dk_inline float2 operator/(const float& rhs) const
		{
			return float2(x / rhs, y / rhs);
		}

	public:
		union
		{
			float m[2];
			struct
			{
				float x, y;
			};
		};
	};

	struct float3
	{
		static const float3 Identity;
		static const float3 Zero;

		float3()
			: m{ 0, 0, 0 }
		{}

		float3(const float& _x, const float& _y, const float& _z)
			: m{ _x, _y, _z }
		{}
		float3(const float2& value0, const float value1)
			: m{ value0.x, value0.y, value1 }
		{}

		dk_inline void operator+=(const float3& rhs) noexcept
		{
			x += rhs.x;
			y += rhs.y;
			z += rhs.z;
		}

		dk_inline const float3 operator+(const float3& rhs) const noexcept
		{
			return float3(x + rhs.x, y + rhs.y, z + rhs.z);
		}

		dk_inline float3 operator-(const float3& rhs) const noexcept
		{
			return float3(x - rhs.x, y - rhs.y, z - rhs.z);
		}

		dk_inline void operator*=(const int rhs) noexcept
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
		}
		dk_inline void operator*=(const float rhs) noexcept
		{
			x *= rhs;
			y *= rhs;
			z *= rhs;
		}
		dk_inline float3 operator*(const int rhs) const
		{
			return float3(x * rhs, y * rhs, z * rhs);
		}
		dk_inline float3 operator*(const float rhs) const
		{
			return float3(x * rhs, y * rhs, z * rhs);
		}
		dk_inline float3 operator/(const float rhs) const
		{
			return float3(x / rhs, y / rhs, z / rhs);
		}

		dk_inline float length() const
		{
			return std::sqrt(x * x + y * y + z * z);
		}

		dk_inline void normalize()
		{
			float len = length();
			if (len == 0)
				len = 1.0f;

			x /= len;
			y /= len;
			z /= len;
		}

	public:
		union
		{
			float m[3];
			struct
			{
				float x, y, z;
			};
		};
	};

	class Math
	{
	public:
		constexpr static float PI = 3.141592f;
		constexpr static float Half_PI = PI / 2.0f;
		constexpr static float kToRadian = PI / 180.0f;
		constexpr static float kToDegree = 1.0f / kToRadian;

		static float cos(const float& rad)
		{
			return ::cos(rad);
		}
		static float sin(const float& rad)
		{
			return ::sin(rad);
		}
		static float asin(const float& rad)
		{
			return ::asin(rad);
		}
		static float tan(const float& rad)
		{
			return ::tan(rad);
		}
		static float atan2(const float& x1, const float& x2)
		{
			return ::atan2(x1, x2);
		}

		static float copysign(const float& number, float& sign)
		{
			return std::copysign(number, sign);
		}

		static float sqrt(const float& value)
		{
			return std::sqrtf(value);
		}

		static float min(const float& lhs, const float& rhs)
		{
			return std::min(lhs, rhs);
		}
		static float max(const float& lhs, const float& rhs)
		{
			return std::max(lhs, rhs);
		}

		static float clamp(const float& value, const float& min, const float& max)
		{
			return Math::max(min, Math::min(value, max));
		}

		static float floor(const float& value)
		{
			return std::floor(value);
		}
		static float3 floor(const float3& value)
		{
			return float3(std::floor(value.x), std::floor(value.y), std::floor(value.z));
		}
		static float2 floor(const float2& value)
		{
			return float2(std::floor(value.x), std::floor(value.y));
		}
	};

	struct float3x3
	{
		static const float3x3 Identity;

		float3x3() : _rows{ float3(0,0,0), float3(0,0,0), float3(0,0,0) }
		{};
		float3x3(float __11, float __12, float __13, float __21, float __22, float __23, float __31, float __32, float __33)
			: _11(__11), _12(__12), _13(__13),
			_21(__21), _22(__22), _23(__23),
			_31(__31), _32(__32), _33(__33)
		{}
		float3x3(const float3& rotation)
		{
			float roll = rotation.z;
			float pitch = rotation.x;
			float yaw = rotation.y;

			float cr = Math::cos(roll);
			float sr = Math::sin(roll);
			float cp = Math::cos(pitch);
			float sp = Math::sin(pitch);
			float cy = Math::cos(yaw);
			float sy = Math::sin(yaw);

			*this = float3x3(
				cr * cy - sr * sp * sy, -sr * cp, cr * sy + sr * sp * cy,
				sr * cy + cr * sp * sy, cr * cp, sr * sy - cr * sp * cy,
				-cp * sy, sp, (cp * cy)
			);
		}

		dk_inline void transpose() noexcept
		{
			DK::swap(_12, _21);
			DK::swap(_13, _31);
			DK::swap(_23, _32);
		}

		dk_inline const float3x3 operator*(const float3x3& rhs) noexcept
		{
			float3x3 temp;
			temp._11 = _11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31;
			temp._12 = _11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32;
			temp._13 = _11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33;
			temp._21 = _21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31;
			temp._22 = _21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32;
			temp._23 = _21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33;
			temp._31 = _31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31;
			temp._32 = _31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32;
			temp._33 = _31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33;

			return temp;
		}

		union
		{
			float3 _rows[3];
			struct
			{
				float _11, _12, _13;
				float _21, _22, _23;
				float _31, _32, _33;
			};
		};
	};

	dk_inline float3 operator*(const float3& position, const float3x3& rotationMatrix)
	{
		return float3(
			position.x * rotationMatrix._11 + position.y * rotationMatrix._21 + position.z * rotationMatrix._31
			, position.x * rotationMatrix._12 + position.y * rotationMatrix._22 + position.z * rotationMatrix._32
			, position.x * rotationMatrix._13 + position.y * rotationMatrix._23 + position.z * rotationMatrix._33
		);
	}

	struct float4
	{
		float4()
			: x(0), y(0), z(0), w(0)
		{}
		float4(float _x, float _y, float _z, float _w)
			: x(_x), y(_y), z(_z), w(_w)
		{}
		float4(const float2& rhs, const float _z, const float _w)
			: m{ rhs.x, rhs.y, _z, _w }
		{}
		float4(const float3& rhs)
			: x(rhs.x), y(rhs.y), z(rhs.z), w(1.0f)
		{}
		float4(const float3& rhs, const float _w)
			: x(rhs.x), y(rhs.y), z(rhs.z), w(_w)
		{}
		float4(const float3&& rhs, const float _w)
			: x(rhs.x), y(rhs.y), z(rhs.z), w(_w)
		{}

		dk_inline float3 xyz() const
		{
			return float3(x, y, z);
		}

		dk_inline float4 operator*(const int rhs) const noexcept
		{
			return float4(x * rhs, y * rhs, z * rhs, w);
		}
		dk_inline float4 operator*(const float rhs) const noexcept
		{
			return float4(x * rhs, y * rhs, z * rhs, w);
		}

		union
		{
			float m[4];
			struct
			{
				float x, y, z, w;
			};
		};
	};

	struct Transform;
	struct float4x4
	{
		static const float4x4 Identity;
		static float4x4 slerp(const float4x4& lhs, const float4x4& rhs, const float ratio)
		{
#ifdef USE_DIRECTX_MATH
			DirectX::XMMATRIX lMatrix = rhs.convertXMMATRIX();
			DirectX::XMVECTOR lTranslation, lQuaternion, lScale;
			DirectX::XMMatrixDecompose(&lScale, &lQuaternion, &lTranslation, lMatrix);

			DirectX::XMMATRIX rMatrix = rhs.convertXMMATRIX();
			DirectX::XMVECTOR rTranslation, rQuaternion, rScale;
			DirectX::XMMatrixDecompose(&rScale, &rQuaternion, &rTranslation, rMatrix);

			DirectX::XMVECTOR lerpTranslation = DirectX::XMVectorLerp(lTranslation, rTranslation, ratio);
			DirectX::XMVECTOR lerpQuaternion = DirectX::XMQuaternionSlerp(lQuaternion, rQuaternion, ratio);
			DirectX::XMVECTOR lerpScale = DirectX::XMVectorLerp(lScale, rScale, ratio);

			DirectX::XMMATRIX lerpTranslationMatrix = DirectX::XMMatrixTranslationFromVector(lerpTranslation);
			DirectX::XMMATRIX lerpQuaternionMatrix = DirectX::XMMatrixRotationQuaternion(lerpQuaternion);
			DirectX::XMMATRIX lerpScaleMatrix = DirectX::XMMatrixScalingFromVector(lerpScale);

			DirectX::XMMATRIX lerpMatrix = lerpTranslationMatrix * lerpQuaternionMatrix * lerpScaleMatrix;
			
			float4x4 outMatrix;
			outMatrix.convertfloat4x4(lerpMatrix);

			return outMatrix;
#else
			static_assert(false, "Matrix slerp 구현 필요합니다");
#endif
		}

		float4x4()
			: _11(1), _12(0), _13(0), _14(0)
			, _21(0), _22(1), _23(0), _24(0)
			, _31(0), _32(0), _33(1), _34(0)
			, _41(0), _42(0), _43(0), _44(1)
		{}
		float4x4(
			float __11, float __12, float __13, float __14
			, float __21, float __22, float __23, float __24
			, float __31, float __32, float __33, float __34
			, float __41, float __42, float __43, float __44
		)
			: _11(__11), _12(__12), _13(__13), _14(__14)
			, _21(__21), _22(__22), _23(__23), _24(__24)
			, _31(__31), _32(__32), _33(__33), _34(__34)
			, _41(__41), _42(__42), _43(__43), _44(__44)
		{}
		float4x4(const float3x3& matrix)
			: _11(matrix._11), _12(matrix._12), _13(matrix._13), _14(0)
			, _21(matrix._21), _22(matrix._22), _23(matrix._23), _24(0)
			, _31(matrix._31), _32(matrix._32), _33(matrix._33), _34(0)
			, _41(0), _42(0), _43(0), _44(1)
		{}

		dk_inline const float4x4& operator+=(const float4x4& rhs) noexcept
		{
			_11 += rhs._11; _12 += rhs._12; _13 += rhs._13; _14 += rhs._14;
			_21 += rhs._21; _22 += rhs._22; _23 += rhs._23; _24 += rhs._24;
			_31 += rhs._31; _32 += rhs._32; _33 += rhs._33; _34 += rhs._34;
			_41 += rhs._41; _42 += rhs._42; _43 += rhs._43; _44 += rhs._44;

			return *this;
		}
		dk_inline const float4x4& operator*(const float& rhs) noexcept
		{
			_11 *= rhs; _12 *= rhs; _13 *= rhs; _14 *= rhs;
			_21 *= rhs; _22 *= rhs; _23 *= rhs; _24 *= rhs;
			_31 *= rhs; _32 *= rhs; _33 *= rhs; _34 *= rhs;
			_41 *= rhs; _42 *= rhs; _43 *= rhs; _44 *= rhs;

			return *this;
		}
		dk_inline float4x4 operator*(const float4x4& rhs) const noexcept
		{
			return float4x4(
				_11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31 + _14 * rhs._41,
				_11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32 + _14 * rhs._42,
				_11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33 + _14 * rhs._43,
				_11 * rhs._14 + _12 * rhs._24 + _13 * rhs._34 + _14 * rhs._44,

				_21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31 + _24 * rhs._41,
				_21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32 + _24 * rhs._42,
				_21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33 + _24 * rhs._43,
				_21 * rhs._14 + _22 * rhs._24 + _23 * rhs._34 + _24 * rhs._44,

				_31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31 + _34 * rhs._41,
				_31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32 + _34 * rhs._42,
				_31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33 + _34 * rhs._43,
				_31 * rhs._14 + _32 * rhs._24 + _33 * rhs._34 + _34 * rhs._44,

				_41 * rhs._11 + _42 * rhs._21 + _43 * rhs._31 + _44 * rhs._41,
				_41 * rhs._12 + _42 * rhs._22 + _43 * rhs._32 + _44 * rhs._42,
				_41 * rhs._13 + _42 * rhs._23 + _43 * rhs._33 + _44 * rhs._43,
				_41 * rhs._14 + _42 * rhs._24 + _43 * rhs._34 + _44 * rhs._44
			);
		}

		dk_inline void transpose() noexcept
		{
			DK::swap(_12, _21);
			DK::swap(_13, _31);
			DK::swap(_14, _41);
			DK::swap(_23, _32);
			DK::swap(_24, _42);
			DK::swap(_34, _43);
		}

		dk_inline void inverse_copy(float4x4& outMatrix) const noexcept
		{
#ifdef USE_DIRECTX_MATH
			DirectX::XMMATRIX matrix = convertXMMATRIX();
			DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(matrix);
			DirectX::XMMATRIX E = DirectX::XMMatrixInverse(&det, matrix);

			outMatrix = float4x4(
				E.r[0].m128_f32[0], E.r[0].m128_f32[1], E.r[0].m128_f32[2], E.r[0].m128_f32[3],
				E.r[1].m128_f32[0], E.r[1].m128_f32[1], E.r[1].m128_f32[2], E.r[1].m128_f32[3],
				E.r[2].m128_f32[0], E.r[2].m128_f32[1], E.r[2].m128_f32[2], E.r[2].m128_f32[3],
				E.r[3].m128_f32[0], E.r[3].m128_f32[1], E.r[3].m128_f32[2], E.r[3].m128_f32[3]
			);
#else
			static_assert(false, "Matrix Inverse 구현 필요합니다");
#endif
		}

		void toTransform(Transform& outTransform) const;

		dk_inline const float3 getTranslation() const noexcept
		{
			return float3(_rows[3].x, _rows[3].y, _rows[3].z);
		}

	private:
#ifdef USE_DIRECTX_MATH
		DirectX::XMMATRIX convertXMMATRIX() const
		{
			DirectX::XMMATRIX matrix;
			matrix.r[0].m128_f32[0] = _rows[0].x; matrix.r[0].m128_f32[1] = _rows[0].y; matrix.r[0].m128_f32[2] = _rows[0].z; matrix.r[0].m128_f32[3] = _rows[0].w;
			matrix.r[1].m128_f32[0] = _rows[1].x; matrix.r[1].m128_f32[1] = _rows[1].y; matrix.r[1].m128_f32[2] = _rows[1].z; matrix.r[1].m128_f32[3] = _rows[1].w;
			matrix.r[2].m128_f32[0] = _rows[2].x; matrix.r[2].m128_f32[1] = _rows[2].y; matrix.r[2].m128_f32[2] = _rows[2].z; matrix.r[2].m128_f32[3] = _rows[2].w;
			matrix.r[3].m128_f32[0] = _rows[3].x; matrix.r[3].m128_f32[1] = _rows[3].y; matrix.r[3].m128_f32[2] = _rows[3].z; matrix.r[3].m128_f32[3] = _rows[3].w;

			return matrix;
		}

		void convertfloat4x4(const DirectX::XMMATRIX& matrix)
		{
			_11 = matrix.r[0].m128_f32[0]; _12 = matrix.r[0].m128_f32[1]; _13 = matrix.r[0].m128_f32[2]; _14 = matrix.r[0].m128_f32[3];
			_21 = matrix.r[1].m128_f32[0]; _22 = matrix.r[1].m128_f32[1]; _23 = matrix.r[1].m128_f32[2]; _24 = matrix.r[1].m128_f32[3];
			_31 = matrix.r[2].m128_f32[0]; _32 = matrix.r[2].m128_f32[1]; _33 = matrix.r[2].m128_f32[2]; _34 = matrix.r[2].m128_f32[3];
			_41 = matrix.r[3].m128_f32[0]; _42 = matrix.r[3].m128_f32[1]; _43 = matrix.r[3].m128_f32[2]; _44 = matrix.r[3].m128_f32[3];
		}
#endif

	public:
		union
		{
			float4 _rows[4];
			struct
			{
				float _11, _12, _13, _14;
				float _21, _22, _23, _24;
				float _31, _32, _33, _34;
				float _41, _42, _43, _44;
			};
		};
	};

	static float4 operator*(const float4& lhs, const float4x4& rhs)
	{
		return float4(
			lhs.x * rhs._11 + lhs.y * rhs._21 + lhs.z * rhs._31 + lhs.w * rhs._41,
			lhs.x * rhs._12 + lhs.y * rhs._22 + lhs.z * rhs._32 + lhs.w * rhs._42,
			lhs.x * rhs._13 + lhs.y * rhs._23 + lhs.z * rhs._33 + lhs.w * rhs._43,
			lhs.x * rhs._14 + lhs.y * rhs._24 + lhs.z * rhs._34 + lhs.w * rhs._44
		);
	}

	struct Quaternion
	{
		static const Quaternion Identity;

		Quaternion(float _x, float _y, float _z, float _w)
#ifdef USE_DIRECTX_MATH
		{
			r.m128_f32[0] = _x;
			r.m128_f32[1] = _y;
			r.m128_f32[2] = _z;
			r.m128_f32[3] = _w;
		}
#else
			: x(_x), y(_y), z(_z), w(_w)
		{}
#endif

		Quaternion(float roll, float pitch, float yaw)
		{
#ifdef USE_DIRECTX_MATH
			r = DirectX::XMQuaternionRotationRollPitchYaw(pitch, yaw, roll);

#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}

		dk_inline const Quaternion& operator*(const Quaternion& rhs)
		{
#ifdef USE_DIRECTX_MATH
			this->r = DirectX::XMQuaternionMultiply(this->r, rhs.r);
			return *this;
#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}
		dk_inline Quaternion operator*(const Quaternion& rhs) const
		{
#ifdef USE_DIRECTX_MATH
			DirectX::XMVECTOR tempQuaternion = DirectX::XMQuaternionMultiply(this->r, rhs.r);
			return Quaternion(tempQuaternion.m128_f32[0], tempQuaternion.m128_f32[1], tempQuaternion.m128_f32[2], tempQuaternion.m128_f32[3]);
#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}

		dk_inline const Quaternion& operator*=(const Quaternion& rhs)
		{
#ifdef USE_DIRECTX_MATH
			* this = this->operator*(rhs);
			return *this;
#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}

		dk_inline void normalize() noexcept
		{
#ifdef USE_DIRECTX_MATH
			r = DirectX::XMQuaternionNormalize(r);
#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}

		dk_inline const void invert()
		{
#ifdef USE_DIRECTX_MATH
			r = DirectX::XMQuaternionInverse(r);
#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}

		dk_inline void toFloat4x4(float4x4& outMatrix) const
		{
#ifdef USE_DIRECTX_MATH
			DirectX::XMMATRIX matrix = DirectX::XMMatrixRotationQuaternion(r);
			outMatrix = float4x4(
				matrix.r[0].m128_f32[0], matrix.r[0].m128_f32[1], matrix.r[0].m128_f32[2], matrix.r[0].m128_f32[3], 
				matrix.r[1].m128_f32[0], matrix.r[1].m128_f32[1], matrix.r[1].m128_f32[2], matrix.r[1].m128_f32[3], 
				matrix.r[2].m128_f32[0], matrix.r[2].m128_f32[1], matrix.r[2].m128_f32[2], matrix.r[2].m128_f32[3], 
				matrix.r[3].m128_f32[0], matrix.r[3].m128_f32[1], matrix.r[3].m128_f32[2], matrix.r[3].m128_f32[3]
			);
#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}

		dk_inline void toEuler(float3& outEuler) const
		{
#ifdef USE_DIRECTX_MATH
			DirectX::XMMATRIX M = DirectX::XMMatrixRotationQuaternion(r);

			float cy = sqrtf(M.r[2].m128_f32[2] * M.r[2].m128_f32[2] + M.r[2].m128_f32[0] * M.r[2].m128_f32[0]);

			DirectX::XMVECTORF32 vResult;
			vResult.f[0] = atan2f(-M.r[2].m128_f32[1], cy);
			if (cy > 16.f * FLT_EPSILON)
			{
				vResult.f[1] = atan2f(M.r[2].m128_f32[0], M.r[2].m128_f32[2]);
				vResult.f[2] = atan2f(M.r[0].m128_f32[1], M.r[1].m128_f32[1]);
			}
			else
			{
				vResult.f[1] = 0.f;
				vResult.f[2] = atan2f(-M.r[1].m128_f32[0], M.r[0].m128_f32[0]);
			}
			vResult.f[3] = 0.f;
			outEuler = float3(vResult.f[0], vResult.f[1], vResult.f[2]);
#else
			static_assert(false, "Quaternion 구현 필요");
#endif
		}

		union
		{
			struct
			{
				float x, y, z, w;
			};
#ifdef USE_DIRECTX_MATH
			DirectX::XMVECTOR r;
#else
			float r[4];
#endif
		};
	};

	static float3 operator*(const float3& lhs, const Quaternion& rhs)
	{
#ifdef USE_DIRECTX_MATH
		DirectX::FXMVECTOR translation = DirectX::XMVectorSet(lhs.x, lhs.y, lhs.z, 1);
		DirectX::FXMVECTOR rotatedTranslation = DirectX::XMVector3Rotate(translation, rhs.r);
		return float3(rotatedTranslation.m128_f32[0], rotatedTranslation.m128_f32[1], rotatedTranslation.m128_f32[2]);
#else
		static_assert(false, "Quaternion 구현 필요");
#endif
	}

	struct Transform
	{
	public: const static Transform Identity;
	public:
		Transform()
			: _translation(float3::Zero)
			, _rotation(Quaternion::Identity)
			, _scale(float3::Identity)
		{}
		Transform(const float3& translation, const Quaternion& rotation, const float3& scale)
			: _translation(translation)
			, _rotation(rotation)
			, _scale(scale)
		{}

		dk_inline void operator+=(const Transform& rhs) noexcept
		{
			_translation += rhs._translation;
			_rotation *= rhs._rotation;
			_scale += rhs._scale;
		}

		dk_inline const Transform& operator*(const Transform& rhs) noexcept
		{
			_translation = rhs._translation + _translation * rhs._rotation;
			_rotation *= rhs._rotation;

			return *this;
		}

		dk_inline void tofloat4x4(float4x4& outMatrix) const noexcept
		{
#ifdef USE_DIRECTX_MATH
			DirectX::FXMMATRIX m = DirectX::XMMatrixRotationQuaternion(_rotation.r);
			outMatrix = float4x4(
				m.r[0].m128_f32[0] * _scale.x,	m.r[0].m128_f32[1],				m.r[0].m128_f32[2],				m.r[0].m128_f32[3],
				m.r[1].m128_f32[0],				m.r[1].m128_f32[1] * _scale.y,	m.r[1].m128_f32[2],				m.r[1].m128_f32[3],
				m.r[2].m128_f32[0],				m.r[2].m128_f32[1],				m.r[2].m128_f32[2] * _scale.z,	m.r[2].m128_f32[3],
				_translation.x,					_translation.y,					_translation.z,					1
			);
#else
			float roll = _rotation.z;
			float pitch = _rotation.x;
			float yaw = _rotation.y;

			float cr = Math::cos(roll);
			float sr = Math::sin(roll);
			float cp = Math::cos(pitch);
			float sp = Math::sin(pitch);
			float cy = Math::cos(yaw);
			float sy = Math::sin(yaw);

			outMatrix = float4x4(
				(cr * cy - sr * sp * sy) * _scale.x, -sr * cp, cr * sy + sr * sp * cy, 0,
				sr * cy + cr * sp * sy, (cr * cp) * _scale.y, sr * sy - cr * sp * cy, 0,
				-cp * sy, sp, (cp * cy) * _scale.z, 0,
				_position.x, _position.y, _position.z, 1
			);
#endif
		}

#if 0
		dk_inline void GetRollRotationMatrix(float3x3& outMatrix) const noexcept
		{
			float roll = _rotation.z;

			outMatrix._11 = cos(roll);
			outMatrix._12 = -sin(roll);
			outMatrix._13 = 0;
			outMatrix._21 = sin(roll);
			outMatrix._22 = cos(roll);
			outMatrix._23 = 0;
			outMatrix._31 = 0;
			outMatrix._32 = 0;
			outMatrix._33 = 1;
		}
		dk_inline void GetYawRotationMatrix(float3x3& outMatrix) const noexcept
		{
			float yaw = _rotation.y;

			outMatrix._11 = cos(yaw);
			outMatrix._12 = 0;
			outMatrix._13 = sin(yaw);
			outMatrix._21 = 0;
			outMatrix._22 = 1;
			outMatrix._23 = 0;
			outMatrix._31 = -sin(yaw);
			outMatrix._32 = 0;
			outMatrix._33 = cos(yaw);
		}

		dk_inline void GetPitchRotationMatrix(float3x3& outMatrix) const noexcept
		{
			float pitch = _rotation.x;

			outMatrix._11 = 1;
			outMatrix._12 = 0;
			outMatrix._13 = 0;
			outMatrix._21 = 0;
			outMatrix._22 = cos(pitch);
			outMatrix._23 = -sin(pitch);
			outMatrix._31 = 0;
			outMatrix._32 = sin(pitch);
			outMatrix._33 = cos(pitch);
		}

		dk_inline float3 GetForward() const noexcept
		{
			static const float3 worldForward = float3(0, 0, 1);

			float3x3 rotationMatrix;
			//GetRoationMatrix(rotationMatrix);
			GetRoationMatrix(rotationMatrix);

			return worldForward * rotationMatrix;
		}

		dk_inline float3 GetRight() const noexcept
		{
			static const float3 worldRight = float3(1, 0, 0);

			float3x3 rotationMatrix;
			GetRoationMatrix(rotationMatrix);

			return worldRight * rotationMatrix;
		}

		dk_inline void GetRoationMatrix(float3x3& outMatrix) const noexcept
		{
#if 0
			float roll = _rotation.z;
			float yaw = _rotation.y;
			float pitch = _rotation.x;

			// 출처 : https://en.wikipedia.org/wiki/Rotation_matrix
			outMatrix._11 = cos(yaw) * cos(pitch);
			outMatrix._12 = cos(yaw) * sin(pitch) * sin(roll) - sin(yaw) * cos(roll);
			outMatrix._13 = cos(yaw) * sin(pitch) * cos(roll) + sin(yaw) * sin(roll);
			outMatrix._21 = sin(yaw) * cos(pitch);
			outMatrix._22 = sin(yaw) * sin(pitch) * sin(roll) + cos(yaw) * cos(roll);
			outMatrix._23 = sin(yaw) * sin(pitch) * cos(roll) - cos(yaw) * sin(roll);
			outMatrix._31 = -sin(pitch);
			outMatrix._32 = cos(pitch) * sin(roll);
			outMatrix._33 = cos(pitch) * cos(roll);
#else
			float3x3 yawMatrix;
			GetYawRotationMatrix(yawMatrix);
			float3x3 rollMatrix;
			GetRollRotationMatrix(rollMatrix);
			float3x3 pitchMatrix;
			GetPitchRotationMatrix(pitchMatrix);

			outMatrix = rollMatrix;
			outMatrix = outMatrix * pitchMatrix;
			outMatrix = outMatrix * yawMatrix;
#endif
		}
#endif

		dk_inline void invert() noexcept
		{
			_translation *= -1;
			_rotation.invert();
		}

	private:
		DK_REFLECTION_PROPERTY(float3, _translation);
		DK_REFLECTION_PROPERTY(Quaternion, _rotation);
		DK_REFLECTION_PROPERTY(float3, _scale);
	};
}
#endif // !__DEFINE_STDAFX__