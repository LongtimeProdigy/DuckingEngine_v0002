#pragma once

#include "CommonTypes.h"

namespace DK
{
    struct float3;
    struct Quaternion;

    class StringUtil
    {
    public:
        static DKVector<DKString> split(const DKString& str, const DKString& delimiter);
        static DKVector<DKString> splitWhitespace(const DKString& str);
        static bool isWhitespace(char value);
        static bool parseInt(const DKString& str, int& value);
        static bool parseFloat(const DKString& str, float& value);
        static DKString toString(int value);
        static DKString toString(uint32 value);
        static DKString toString(float value);
        static DKStringW toWideString(uint32 value);
        static bool format(DKString& output, const char* pattern, ...);
        static float3 parseFloat3FromString(const DKString& str);
        static Quaternion parseQuaternionFromString(const DKString& str);
        static char* itoa(uint32 value, char* buffer, int radix) noexcept;
        static int atoi(const char* str) noexcept;
        static float atof(const char* str) noexcept;
        static int strcmp(const char* str1, const char* str2) noexcept;
        static int wcscmp(const wchar_t* str1, const wchar_t* str2) noexcept;
        static uint32 strlen(const char* str);
        static uint32 strlen(const wchar_t* str);
        static char* strcpy(char* dest, const char* src);
        static wchar_t* strcpy(wchar_t* dest, const wchar_t* src);
        static char* strcat(char* dest, const char* src);
        static wchar_t* strcat(wchar_t* dest, const wchar_t* src);
        static DKString extension(const DKString& source);
        static void lower(DKString& inoutString);
        static DKString convertWCtoC(const wchar_t* str);
        static DKStringW convertCtoWC(const char* str);
    };
}
