#include "StringUtil.h"
#include "CommonLog.h"
#include "FileUtil.h"
#include <Windows.h>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <limits>
#include <locale.h>

namespace DK
{
	DKVector<DKString> StringUtil::split(const DKString& str, const DKString& delimiter)
	{
		DKVector<DKString> tokens;
		if (delimiter.empty())
			return tokens;

		DKString::size_type start = 0;
		while (start < str.length())
		{
			const DKString::size_type end = str.find(delimiter, start);
			if (end == DKString::npos)
			{
				tokens.emplace_back(str.substr(start));
				break;
			}
			if (end > start)
				tokens.emplace_back(str.substr(start, end - start));
			start = end + delimiter.length();
		}
		return tokens;
	}

    bool StringUtil::isWhitespace(char value)
    {
        return value == ' ' || value == '\t' || value == '\n' || value == '\r' || value == '\v' || value == '\f';
    }

    DKVector<DKString> StringUtil::splitWhitespace(const DKString& str)
    {
        DKVector<DKString> tokens;
        size_t cursor = 0;
        while (cursor < str.size())
        {
            while (cursor < str.size() && isWhitespace(str[cursor]))
            {
                ++cursor;
            }
            const size_t start = cursor;
            while (cursor < str.size() && isWhitespace(str[cursor]) == false)
            {
                ++cursor;
            }
            if (cursor > start)
            {
                tokens.push_back(str.substr(start, cursor - start));
            }
        }
        return tokens;
    }

    bool StringUtil::parseInt(const DKString& str, int& value)
    {
        const char* begin = str.data();
        const char* end = begin + str.size();
        if (begin < end && *begin == '+')
        {
            ++begin;
            if (begin == end || *begin == '-')
            {
                DK_ASSERT_LOG(false, "Invalid integer sign");
                return false;
            }
        }
        int parsed = 0;
        const std::from_chars_result result = std::from_chars(begin, end, parsed);
        if (result.ec != std::errc() || result.ptr != end)
        {
            DK_ASSERT_LOG(false, "Invalid integer: %s", str.c_str());
            return false;
        }
        value = parsed;
        return true;
    }

    bool StringUtil::parseFloat(const DKString& str, float& value)
    {
        const char* begin = str.data();
        const char* end = begin + str.size();
        if (begin < end && *begin == '+')
        {
            ++begin;
            if (begin == end || *begin == '-')
            {
                DK_ASSERT_LOG(false, "Invalid float sign");
                return false;
            }
        }
        float parsed = 0.0f;
        const std::from_chars_result result = std::from_chars(begin, end, parsed, std::chars_format::general);
        if (result.ec != std::errc() || result.ptr != end || std::isfinite(parsed) == false)
        {
            DK_ASSERT_LOG(false, "Invalid float: %s", str.c_str());
            return false;
        }
        value = parsed;
        return true;
    }

    DKString StringUtil::toString(int value)
    {
        char buffer[16];
        const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if (result.ec != std::errc())
        {
            DK_ASSERT_LOG(false, "Cannot format integer");
            return {};
        }
        return DKString(buffer, static_cast<size_t>(result.ptr - buffer));
    }

    DKString StringUtil::toString(uint32 value)
    {
        char buffer[16];
        const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if (result.ec != std::errc())
        {
            DK_ASSERT_LOG(false, "Cannot format unsigned integer");
            return {};
        }
        return DKString(buffer, static_cast<size_t>(result.ptr - buffer));
    }

    DKString StringUtil::toString(float value)
    {
        if (std::isfinite(value) == false)
        {
            DK_ASSERT_LOG(false, "Cannot format non-finite float");
            return {};
        }
        char buffer[64];
        const std::to_chars_result result = std::to_chars(buffer, buffer + sizeof(buffer), value, std::chars_format::general, std::numeric_limits<float>::max_digits10);
        if (result.ec != std::errc())
        {
            DK_ASSERT_LOG(false, "Cannot format float");
            return {};
        }
        return DKString(buffer, static_cast<size_t>(result.ptr - buffer));
    }

    DKStringW StringUtil::toWideString(uint32 value)
    {
        const DKString number = toString(value);
        DKStringW result;
        for (char digit : number)
        {
            result.push_back(static_cast<wchar_t>(digit));
        }
        return result;
    }

    bool StringUtil::format(DKString& output, const char* pattern, ...)
    {
        _locale_t locale = _create_locale(LC_ALL, "C");
        if (locale == nullptr)
        {
            DK_ASSERT_LOG(false, "Cannot create numeric formatting locale");
            return false;
        }
        va_list args;
        va_start(args, pattern);
        const int length = _vscprintf_l(pattern, locale, args);
        va_end(args);
        if (length < 0)
        {
            _free_locale(locale);
            DK_ASSERT_LOG(false, "Cannot measure formatted string");
            return false;
        }
        DKString formatted(static_cast<size_t>(length) + 1, '\0');
        va_start(args, pattern);
        const int written = _vsnprintf_s_l(formatted.data(), formatted.size(), _TRUNCATE, pattern, locale, args);
        va_end(args);
        _free_locale(locale);
        if (written != length)
        {
            DK_ASSERT_LOG(false, "Cannot format string");
            return false;
        }
        formatted.resize(static_cast<size_t>(length));
        output = DK::move(formatted);
        return true;
    }

	char* StringUtil::itoa(const uint32 value, char* buffer, int radix) noexcept
	{
#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
		return ::_itoa(value, buffer, radix);
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
	}
	int StringUtil::atoi(const char* str) noexcept
	{
		return std::atoi(str);
	}
	float StringUtil::atof(const char* str) noexcept
	{
		return static_cast<float>(std::atof(str));
	}
	int StringUtil::strcmp(const char* str1, const char* str2) noexcept
	{
		return std::strcmp(str1, str2);
	}
	int StringUtil::wcscmp(const wchar_t* str1, const wchar_t* str2) noexcept
	{
		return std::wcscmp(str1, str2);
	}
	uint32 StringUtil::strlen(const char* str)
	{
		return static_cast<uint32>(std::strlen(str));
	}
	uint32 StringUtil::strlen(const wchar_t* str)
	{
		return static_cast<uint32>(std::wcslen(str));
	}
	char* StringUtil::strcpy(char* dest, const char* src)
	{
#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
		return ::strcpy(dest, src);
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
	}
	wchar_t* StringUtil::strcpy(wchar_t* dest, const wchar_t* src)
	{
#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
		return ::wcscpy(dest, src);
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
	}
	char* StringUtil::strcat(char* dest, const char* src)
	{
#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
		return ::strcat(dest, src);
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
	}
	wchar_t* StringUtil::strcat(wchar_t* dest, const wchar_t* src)
	{
#pragma warning(push)
#pragma warning(disable : 4996)
#define _CRT_SECURE_NO_WARNINGS
		return ::wcscat(dest, src);
#undef _CRT_SECURE_NO_WARNINGS
#pragma warning(pop)
	}

	DKString StringUtil::extension(const DKString& source)
	{
		return convertWCtoC(FileUtil::toWidePath(FileUtil::extension(FileUtil::fromWidePath(convertCtoWC(source.c_str())))).c_str());
	}

	void StringUtil::lower(DKString& inoutString)
	{
		std::transform(inoutString.begin(), inoutString.end(), inoutString.begin(), [](unsigned char c) {
			return static_cast<char>(std::tolower(c));
			});
	}

	DKString StringUtil::convertWCtoC(const wchar_t* str)
	{
		DKVector<char> pStr;
		int strSize = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
		pStr.resize(strSize);
		WideCharToMultiByte(CP_ACP, 0, str, -1, pStr.data(), strSize, 0, 0);
		return pStr.data();
	}

	DKStringW StringUtil::convertCtoWC(const char* str)
	{
		DKVector<wchar_t> pStr;
		int strSize = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, NULL);
		pStr.resize(strSize);
		MultiByteToWideChar(CP_ACP, 0, str, strlen(str) + 1, pStr.data(), strSize);
		return pStr.data();
	}
}
