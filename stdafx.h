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
#define DK_WINDOW
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
/////////////////////// #todo- 나중에 꼭 삭제할 것 /////////////////////// 
#include <Windows.h>	// for OutputDebugStringA
/////////////////////// #todo- 나중에 꼭 삭제할 것 /////////////////////// 

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

/*
* #todo- 외부 라이브러리 목록 (필요시 나중에 따로 자체제작 할 목록들)
*/
#define USE_IMGUI
#define USE_ASSIMP
#ifdef _DK_DEBUG_
#define USE_PIX
#endif

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#define DKVector std::vector
#define DKHashMap std::unordered_map
#define DKPair std::pair
using DKString = std::string;

#define USE_TINYXML
#include "tinyxml.h"

#define DK_COUNT_OF ARRAYSIZE

/*
* RAII 관련
*/
#define DEFINE_REFCOUNTED_STRCUT(name) struct name; using name##Ref = std::shared_ptr<name>;
#define DEFINE_REFCOUNTED(name) class name; using name##Ref = std::shared_ptr<name>;
DEFINE_REFCOUNTED_STRCUT(AppearanceRaw);
DEFINE_REFCOUNTED(Model);
DEFINE_REFCOUNTED(Skeleton);
DEFINE_REFCOUNTED(Animation);
DEFINE_REFCOUNTED(ITexture);

// template을 사용한 클래스여서그런가.. #include "Bufferview.h"를 포함해야만 작동하네..
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;
using VertexBufferViewRef = std::shared_ptr<D3D12_VERTEX_BUFFER_VIEW>;
using IndexBufferViewRef = std::shared_ptr<D3D12_INDEX_BUFFER_VIEW>;

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

	dk_inline void assign(T* ptr) noexcept
	{
		release();
		_ptr = ptr;
	}
	dk_inline void release() noexcept
	{
		dk_delete _ptr;
	}

	dk_inline T* get() const noexcept
	{
		return _ptr;
	}

	dk_inline const T* operator->() const noexcept
	{
		return _ptr;
	}
	dk_inline T* operator->() noexcept
	{
		return _ptr;
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

/*
* ETC Helper 함수
*/
template <typename T>
void Swap(T&& lhs, T&& rhs) noexcept
{
	std::swap(lhs, rhs);
}

namespace DK
{
	class StringUtil
	{
	public:
		static float atof(const char* str) noexcept
		{
			return static_cast<float>(std::atof(str));
		}
	};
}

/*
* Math Utility
*/
struct float3
{
	static const float3 Identity;
	static const float3 Zero;

	float3()
		: m{ 0, 0, 0 }
	{}

	float3(float _x, float _y, float _z)
		: m{ _x, _y, _z }
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

	dk_inline float3 operator*(const int rhs) const
	{
		return float3(x * rhs, y * rhs, z * rhs);
	}
	dk_inline float3 operator*(const float rhs) const
	{
		return float3(x * rhs, y * rhs, z * rhs);
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

namespace DK
{
	class Math
	{
	public:
		constexpr static float PI = 3.141592f;
		constexpr static float Half_PI = 3.141592f / 2.0f;
		constexpr static float kToRadian = PI / 180.0f;

		static float cos(const float rad)
		{
			return ::cos(rad);
		}
		static float sin(const float rad)
		{
			return ::sin(rad);
		}
		static float asin(const float rad)
		{
			return ::asin(rad);
		}
		static float tan(const float rad)
		{
			return ::tan(rad);
		}
		static float atan2(const float x1, const float x2)
		{
			return ::atan2(x1, x2);
		}

		static float copysign(const float number, float sign)
		{
			return std::copysign(number, sign);
		}
	};

	struct Quaternion
	{
	public:
		Quaternion()
			: x(0.0f), y(0.0f), z(0.0f), w(1.0f)
		{}
		Quaternion(float _x, float _y, float _z, float _w)
			: x(_x), y(_y), z(_z), w(_w)
		{}

	public:
		union
		{
			float m[4];
			struct
			{
				float x, y, z, w;
			};
		};
	};
}
#endif // !__DEFINE_STDAFX__