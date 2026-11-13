#pragma once

#define dk_inline inline

#include <array>
#include <cstddef>
#include <cstring>
#include <deque>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

typedef unsigned char byte;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;
typedef int32_t int32;
typedef int64_t int64;

namespace DK
{
    template<typename Signature> using DKFunction = std::function<Signature>;
    using DKString = std::string;
    using DKStringW = std::wstring;
    template<typename T, typename Allocator = std::allocator<T>> using DKVector = std::vector<T, Allocator>;
    template<typename T, size_t Count> using DKArray = std::array<T, Count>;
    template<typename T, typename Allocator = std::allocator<T>> using DKDeque = std::deque<T, Allocator>;
    template<typename T> using DKOptional = std::optional<T>;
    template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Allocator = std::allocator<std::pair<const Key, Value>>> using DKHashMap = std::unordered_map<Key, Value, Hash, Equal, Allocator>;
    template<typename Key, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Allocator = std::allocator<Key>> using DKHashSet = std::unordered_set<Key, Hash, Equal, Allocator>;
    template<typename First, typename Second> using DKPair = std::pair<First, Second>;

    template<typename T> dk_inline typename std::remove_reference<T>::type&& move(T&& value) noexcept
    {
        return std::move(value);
    }

    template<typename T> dk_inline void swap(T&& lhs, T&& rhs) noexcept
    {
        std::swap(lhs, rhs);
    }

    dk_inline void* memcpy(void* destination, const void* source, size_t bytes)
    {
        return std::memcpy(destination, source, bytes);
    }
}
