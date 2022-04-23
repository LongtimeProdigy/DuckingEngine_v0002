#pragma once

/*
* DEV 관련
*/
#if defined(_DEBUG)
#define DK_DEBUG
#endif

/*
* Platform 관련
* _TEMPXBOX 내가 임의로 정한 것.. XBOX빌드 전처리기 키워드 찾아서 교체할 것
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
#ifdef DK_DEBUG
#include <stdio.h>		// for sprintf_s(), strcat_s();
#include <intrin.h>		// for __debugbreak();
/////////////////////// #todo- 나중에 꼭 삭제할 것 /////////////////////// 
#include <Windows.h>	// for OutputDebugStringA
/////////////////////// #todo- 나중에 꼭 삭제할 것 /////////////////////// 

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
* 단순 헬퍼매크로 또는 헬퍼함수
*/
#define CHECK_BOOL_AND_RETURN(value) if((value) == false) return value;
#define CHECK_BOOL_AND_CUSTOMRETURN(value, custom) if((value) == false) return custom;

/*
* #todo- 나중에 따로 자체제작 할 목록들
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

// template을 사용한 클래스여서그런가.. #include "Bufferview.h"를 포함해야만 작동하네..
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