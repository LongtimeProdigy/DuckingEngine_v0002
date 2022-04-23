#pragma once

/*
* DEV ����
*/
#if defined(_DEBUG)
#define DK_DEBUG
#endif

/*
* Platform ����
* _TEMPXBOX ���� ���Ƿ� ���� ��.. XBOX���� ��ó���� Ű���� ã�Ƽ� ��ü�� ��
*/
#if defined(WIN32) || defined(_WIN64) || defined(_TEMPXBOX)
#define DK_XBOX
#if !defined _TEMPXBOX
#define DK_WINDOW
#endif
#endif

#if defined(_PS4) || defined(_PS5)
#define DK_PS
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
#ifdef DK_DEBUG
#include <stdio.h>		// for sprintf_s(), strcat_s();
#include <intrin.h>		// for __debugbreak();
/////////////////////// #todo- ���߿� �� ������ �� /////////////////////// 
#include <Windows.h>	// for OutputDebugStringA
/////////////////////// #todo- ���߿� �� ������ �� /////////////////////// 

#define MAX_LOG_BUFFER_LENGTH 1024

#define DK_LOG(text, ...)											\
{																	\
char buffer[MAX_LOG_BUFFER_LENGTH];									\
sprintf_s(buffer, MAX_LOG_BUFFER_LENGTH, text, __VA_ARGS__);		\
strcat_s(buffer, MAX_LOG_BUFFER_LENGTH, "\n");						\
OutputDebugStringA(buffer);											\
}

#define DK_WLOG(text, ...)											\
{																	\
wchar_t buffer[MAX_LOG_BUFFER_LENGTH];								\
swprintf_s(buffer, MAX_LOG_BUFFER_LENGTH, text, __VA_ARGS__);		\
wcscat_s(buffer, MAX_LOG_BUFFER_LENGTH, L"\n");						\
OutputDebugStringW(buffer);											\
}

#define DK_ASSERT_LOG(condition, text, ...)							\
{																	\
if(!(condition))													\
{																	\
DK_LOG(text, __VA_ARGS__);											\
__debugbreak();														\
}																	\
}

#define DK_ASSERT_WLOG(condition, text, ...)						\
{																	\
if(!(condition))													\
{																	\
DK_WLOG(text, __VA_ARGS__);											\
__debugbreak();														\
}																	\
}
#endif

/*
* �ܼ� ���۸�ũ�� �Ǵ� �����Լ�
*/
#define CHECK_BOOL_AND_RETURN(value) if((value) == false) return value;
#define CHECK_BOOL_AND_CUSTOMRETURN(value, custom) if((value) == false) return custom;

/*
* #todo- ���߿� ���� ��ü���� �� ��ϵ�
*/
#define USE_IMGUI

#include <vector>
#include <unordered_map>
#include <memory>

#include "tinyxml.h"

#define DEFINE_REFCOUNTED_STRCUT(name) struct name; using name##Ref = std::shared_ptr<name>;
#define DEFINE_REFCOUNTED(name) class name; using name##Ref = std::shared_ptr<name>;
DEFINE_REFCOUNTED_STRCUT(AppearanceRaw);
DEFINE_REFCOUNTED(Model);
DEFINE_REFCOUNTED(Skeleton);
DEFINE_REFCOUNTED(Animation);
DEFINE_REFCOUNTED(TextureRaw);
DEFINE_REFCOUNTED(IRootSignature);
DEFINE_REFCOUNTED(IPipelineStateObject);
DEFINE_REFCOUNTED(IShader);
DEFINE_REFCOUNTED(IResource);

// template�� ����� Ŭ���������׷���.. #include "Bufferview.h"�� �����ؾ߸� �۵��ϳ�..
template <typename T> class BufferView;
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;
using VertexBufferView = BufferView<D3D12_VERTEX_BUFFER_VIEW>;
using IndexBufferView = BufferView<D3D12_INDEX_BUFFER_VIEW>;
using VertexBufferViewRef = std::shared_ptr<VertexBufferView>;
using IndexBufferViewRef = std::shared_ptr<IndexBufferView>;

template <typename T>
void Swap(T&& lhs, T&& rhs) noexcept
{
	std::swap(lhs, rhs);
}
//// char to wchar
//size_t convertedSize = 0;
//const size_t cSize = strlen(texturePath) + 1;
//wchar_t destString[MAX_PATH] = L"";
//mbstowcs_s(&convertedSize, destString, texturePath, cSize);

// wchar -> char
//char* WCharToChar(const wchar_t* pwstrSrc)
//{
//#if !defined DK_DEBUG
//    int len = 0;
//    len = (wcslen(pwstrSrc) + 1) * 2;
//    char* pstr = (char*)malloc(sizeof(char) * len);
//
//    WideCharToMultiByte(949, 0, pwstrSrc, -1, pstr, len, NULL, NULL);
//#else
//
//    int nLen = static_cast<int>(wcslen(pwstrSrc));
//
//    char* pstr = (char*)malloc(sizeof(char) * nLen + 1);
//
//    size_t tcnt;
//    wcstombs_s(&tcnt, pstr, nLen + 1, pwstrSrc, nLen + 1);
//#endif
//
//    return pstr;
//}