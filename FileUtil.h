#pragma once

#include "CommonTypes.h"

#include <cstdint>
#include <fstream>

namespace DK
{
    class FileUtil
    {
    public:
        static bool createDirectory(const DKString& path);
        static bool exist(const DKString& path);
        static bool remove(const DKString& path);
        static bool rename(const DKString& source, const DKString& destination, bool overwrite = false);
        static DKString extension(const DKString& path);
        static DKString parentPath(const DKString& path);
        static DKString jointPath(const DKString& parent, const DKString& child);
        static DKString fromWidePath(const DKStringW& path);
        static DKStringW toWidePath(const DKString& path);
    };

    class FileReader
    {
    public:
        FileReader() = default;
        explicit FileReader(const DKString& path);
        bool open(const DKString& path);
        bool close();
        dk_inline bool isOpen() const
        {
            return _stream.is_open();
        }
        dk_inline uint64_t size() const
        {
            return _size;
        }
        bool seek(uint64_t offset);
        bool read(void* data, size_t bytes);
        bool readText(DKString& text);

    private:
        std::ifstream _stream;
        uint64_t _size = 0;
    };

    class FileWriter
    {
    public:
        FileWriter() = default;
        explicit FileWriter(const DKString& path);
        bool open(const DKString& path);
        bool close();
        dk_inline bool isOpen() const
        {
            return _stream.is_open();
        }
        bool write(const void* data, size_t bytes);

    private:
        std::ofstream _stream;
    };
}
