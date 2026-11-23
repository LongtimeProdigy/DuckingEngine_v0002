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
* SAL 및 inline 관련
*/
#include <sal.h>
#define _IN_ _In_
#define _OUT_ _Out_

/*
* 타입 관련
*/
#include "CommonTypes.h"
#include "StringUtil.h"
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4267)
#include "MathTypes.h"
#include "Ref.h"

/*
* Logger 관련
*/
#include "CommonLog.h"

/*
* #todo- 외부 라이브러리 목록 (필요시 나중에 따로 자체제작 할 목록들)
*/
#define USE_IMGUI
#if defined(USE_IMGUI)
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
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
#pragma region DXC
#pragma comment(lib, "lib/dxc_2026_08_11/lib/x64/dxcompiler.lib")
#include "lib/dxc_2026_08_11/inc/dxcapi.h"
#if defined(_DK_DEBUG_)
//#define SUPPORT_HLGL_DEBUGBREAK
#endif
#pragma endregion
#endif

namespace DK
{

	template<typename T, uint32 SIZE>
	class StaticArray
	{
	public:
		const T& operator[](const uint32 index) const
		{
			return _data[index];
		}
		T& operator[](const uint32 index)
		{
			return _data[index];
		}

	private:
		T _data[SIZE];
	};
}

#define USE_TINYXML
#ifdef USE_TINYXML
#include "tinyxml.h"
#else
static_assert(false, "XML 파싱 Class 정의가 필요합니다.");
#endif

/*
* String 함수
*/
#include "FileUtil.h"
#include <wchar.h>
#include <stdlib.h>
namespace DK
{
#define DK_MAX_PATH MAX_PATH
#define DK_MAX_BUFFER 1024

	struct float3;
	struct Quaternion;


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

		const uint32 size() const { return _container.size(); }

#if defined(_DK_DEBUG_)
		const DKVector<DKString>& getStrings() const { return _container; }
#endif

	private:
		DKVector<DKString> _container;
	};

	template <typename T, uint32 SIZE>
	class ScopeStringBase
	{
	public:
		ScopeStringBase()
		{}
		ScopeStringBase(const T* str)
		{
			StringUtil::strcpy(_string, str);
		}

		bool operator!=(const T* rhs) const
		{
			DK_ASSERT_LOG(rhs != nullptr, "입력값이 nullptr이면 안됩니다.");
			return StringUtil::strcmp(c_str(), rhs) != 0;
		}

		dk_inline const T* c_str() const { return _string; }
		dk_inline T* data() { return _string; }
		dk_inline const uint32 capacity() const { return SIZE; }

		dk_inline void append(const T* str)
		{
#ifdef _DK_DEBUG_
			const uint32 length = StringUtil::strlen(_string) + StringUtil::strlen(str);
			DK_ASSERT_LOG(length < SIZE, "Size를 넘어갑니다. %d/%d", length, SIZE);
#endif
			StringUtil::strcat(_string, str);
		}

	private:
		T _string[SIZE] = { 0, };
	};
	template <uint32 SIZE> using ScopeString = ScopeStringBase<char, SIZE>;
	template <uint32 SIZE> using ScopeStringW = ScopeStringBase<wchar_t, SIZE>;
}

/*
* Global Path
*/
namespace DK
{
	class GlobalPath
	{
	public:
		static ScopeString<DK_MAX_PATH> makeResourceFullPath(const DKString& resourcePath)
		{
			ScopeString<DK_MAX_PATH> resourceFullPath(GlobalPath::kResourcePathA.c_str());
			resourceFullPath.append("/");
			resourceFullPath.append(resourcePath.c_str());

			return resourceFullPath;
		}
		static ScopeStringW<DK_MAX_PATH> makeResourceFullPathW(const DKStringW& resourcePath)
		{
			ScopeStringW<DK_MAX_PATH> resourceFullPath(GlobalPath::kResourcePathW.c_str());
			resourceFullPath.append(L"/");
			resourceFullPath.append(resourcePath.c_str());

			return resourceFullPath;
		}

		static void setResourcePath(const DKString& resourcePath)
		{
			kResourcePathA = resourcePath;
			kResourcePathW = StringUtil::convertCtoWC(kResourcePathA.c_str());
		}

	private:
		static DKString kResourcePathA;
		static DKStringW kResourcePathW;
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

#define DK_REFLECTION_PROPERTY_FLAG(type, name, flag) \
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
*	Thread 관련
*/
#include <processthreadsapi.h>
namespace DK
{
	using ThreadID = DWORD;
#if defined(_DK_DEBUG_)
	extern ThreadID gMainThreadID;
	static const bool isMainThread()
	{
		return gMainThreadID == GetCurrentThreadId();
	}
#endif

#define EnsureMainThread() DK_ASSERT_LOG(isMainThread(), "MainThread가 아닙니다. %d/%d", gMainThreadID, GetCurrentThreadId());
}

namespace DK
{
	class ConstPath
	{
	public:
		static constexpr const char* gMaterialDefinition = "Material/MaterialDefinition.xml";

		static constexpr const char* gTerrainModelProperty = "Scenes/ModelProperty";
		static constexpr const char* gPostProcessModelProperty = "System/ModelProperty";
	};
}
#endif // !__DEFINE_STDAFX__
