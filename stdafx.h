#pragma once

/*
* DEV ����
*/
#if defined(_DEBUG)
#define _DK_DEBUG_
#endif

/*
* Platform ����
*/
#if defined(_WIN64)
#define DK_WINDOW
#endif

/*
* �޸� ����
*/
#define dk_new new
#define dk_delete delete
#define dk_delete_array delete[]

/*
* SAL �� inline ����
*/
#include <sal.h>
#define _IN_ _In_
#define _OUT_ _Out_
#define dk_inline inline

/*
* Ÿ�� ����
*/
typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;
typedef unsigned int uint32;
typedef unsigned long long uint64;

/*
* Logger ����
*/
#ifdef _DK_DEBUG_
#include <stdio.h>		// for sprintf_s(), strcat_s();
#include <intrin.h>		// for __debugbreak();
/////////////////////// #todo- ���߿� �� ������ �� /////////////////////// 
#include <Windows.h>	// for OutputDebugStringA
/////////////////////// #todo- ���߿� �� ������ �� /////////////////////// 

#define MAX_LOG_BUFFER_LENGTH 1024

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
* #todo- �ܺ� ���̺귯�� ��� (�ʿ�� ���߿� ���� ��ü���� �� ��ϵ�)
*/
#define USE_IMGUI
#define USE_ASSIMP
#define USE_PIX

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

/*
* RAII ����
*/
#define DEFINE_REFCOUNTED_STRCUT(name) struct name; using name##Ref = std::shared_ptr<name>;
#define DEFINE_REFCOUNTED(name) class name; using name##Ref = std::shared_ptr<name>;
DEFINE_REFCOUNTED_STRCUT(AppearanceRaw);
DEFINE_REFCOUNTED(Model);
DEFINE_REFCOUNTED(Skeleton);
DEFINE_REFCOUNTED(Animation);
DEFINE_REFCOUNTED(ITexture);

// template�� ����� Ŭ���������׷���.. #include "Bufferview.h"�� �����ؾ߸� �۵��ϳ�..
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
		dk_delete _ptr;
	}

	Ptr(const T* ptr)
		: _ptr(ptr)
	{}

	//Ptr(Ptr& ptr) = delete;
	//Ptr(const Ptr& ptr) = delete;
	//Ptr(Ptr&& ptr) = delete;
	//Ptr(const Ptr&& ptr) = delete;

	dk_inline void assign(const T* ptr) noexcept
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

/*
* ETC Helper �Լ�
*/
template <typename T>
void Swap(T&& lhs, T&& rhs) noexcept
{
	std::swap(lhs, rhs);
}