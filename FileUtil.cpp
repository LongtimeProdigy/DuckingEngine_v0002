#include "FileUtil.h"
#include "CommonLog.h"
#include <filesystem>

namespace DK
{
    bool FileUtil::createDirectory(const DKString& path)
    {
        if (path.empty())
        {
            return true;
        }
        std::error_code status;
        std::filesystem::create_directories(std::filesystem::u8path(path.c_str()), status);
        if (status.value() != 0)
        {
            DK_ASSERT_LOG(false, "Cannot create directory: %s", path.c_str());
            return false;
        }
        return true;
    }

    bool FileUtil::exist(const DKString& path)
    {
        std::error_code status;
        const bool found = std::filesystem::exists(std::filesystem::u8path(path.c_str()), status);
        if (status.value() != 0)
        {
            DK_ASSERT_LOG(false, "Cannot query file: %s", path.c_str());
            return false;
        }
        return found;
    }

    bool FileUtil::remove(const DKString& path)
    {
        std::error_code status;
        std::filesystem::remove(std::filesystem::u8path(path.c_str()), status);
        if (status.value() != 0)
        {
            DK_ASSERT_LOG(false, "Cannot remove file: %s", path.c_str());
            return false;
        }
        return true;
    }

    bool FileUtil::rename(const DKString& source, const DKString& destination, bool overwrite)
    {
        const std::filesystem::path target = std::filesystem::u8path(destination.c_str());
        std::error_code status;
        if (overwrite == false)
        {
            const bool found = std::filesystem::exists(target, status);
            if (found || status.value() != 0)
            {
                DK_ASSERT_LOG(false, "Cannot use rename destination: %s", destination.c_str());
                return false;
            }
        }
        std::filesystem::rename(std::filesystem::u8path(source.c_str()), target, status);
        if (status.value() != 0)
        {
            DK_ASSERT_LOG(false, "Cannot rename file: %s", source.c_str());
            return false;
        }
        return true;
    }

    DKString FileUtil::extension(const DKString& path)
    {
        return DKString(std::filesystem::u8path(path.c_str()).extension().u8string().c_str());
    }

    DKString FileUtil::parentPath(const DKString& path)
    {
        return DKString(std::filesystem::u8path(path.c_str()).parent_path().u8string().c_str());
    }

    DKString FileUtil::jointPath(const DKString& parent, const DKString& child)
    {
        return DKString((std::filesystem::u8path(parent.c_str()) / std::filesystem::u8path(child.c_str())).u8string().c_str());
    }

    DKString FileUtil::fromWidePath(const DKStringW& path)
    {
        return DKString(std::filesystem::path(path.c_str()).u8string().c_str());
    }

    DKStringW FileUtil::toWidePath(const DKString& path)
    {
        return DKStringW(std::filesystem::u8path(path.c_str()).wstring().c_str());
    }

    FileReader::FileReader(const DKString& path)
    {
        open(path);
    }

    bool FileReader::open(const DKString& path)
    {
        if (close() == false)
        {
            return false;
        }
        _stream.open(std::filesystem::u8path(path.c_str()), std::ios::binary | std::ios::ate);
        if (_stream.is_open() == false)
        {
            DK_ASSERT_LOG(false, "Cannot open file for reading: %s", path.c_str());
            return false;
        }
        const std::streampos length = _stream.tellg();
        if (length < 0)
        {
            DK_ASSERT_LOG(false, "Cannot read file size: %s", path.c_str());
            close();
            return false;
        }
        _size = static_cast<uint64_t>(length);
        if (seek(0) == false)
        {
            close();
            return false;
        }
        return true;
    }

    bool FileReader::close()
    {
        _stream.clear();
        _size = 0;
        if (_stream.is_open())
        {
            _stream.close();
            if (_stream.fail())
            {
                DK_ASSERT_LOG(false, "Cannot close file reader");
                return false;
            }
        }
        return true;
    }

    bool FileReader::seek(uint64_t offset)
    {
        if (isOpen() == false || offset > _size)
        {
            DK_ASSERT_LOG(false, "Cannot seek closed file or beyond file size");
            return false;
        }
        _stream.clear();
        _stream.seekg(static_cast<std::streamoff>(offset));
        if (_stream.fail())
        {
            DK_ASSERT_LOG(false, "Cannot seek file");
            return false;
        }
        return true;
    }

    bool FileReader::read(void* data, size_t bytes)
    {
        if (isOpen() == false)
        {
            DK_ASSERT_LOG(false, "Cannot read closed file");
            return false;
        }
        if (bytes > 0)
        {
            _stream.read(static_cast<char*>(data), static_cast<std::streamsize>(bytes));
        }
        if (_stream.fail())
        {
            DK_ASSERT_LOG(false, "Cannot read file bytes");
            return false;
        }
        return true;
    }

    bool FileReader::readText(DKString& text)
    {
        if (seek(0) == false)
        {
            return false;
        }
        DKString loaded(static_cast<size_t>(_size), '\0');
        if (read(loaded.data(), loaded.size()) == false)
        {
            return false;
        }
        text = DK::move(loaded);
        return true;
    }

    FileWriter::FileWriter(const DKString& path)
    {
        open(path);
    }

    bool FileWriter::open(const DKString& path)
    {
        if (close() == false)
        {
            return false;
        }
        _stream.clear();
        _stream.open(std::filesystem::u8path(path.c_str()), std::ios::binary | std::ios::trunc);
        if (_stream.is_open() == false)
        {
            DK_ASSERT_LOG(false, "Cannot open file for writing: %s", path.c_str());
            return false;
        }
        return true;
    }

    bool FileWriter::close()
    {
        if (_stream.is_open())
        {
            _stream.flush();
            _stream.close();
            if (_stream.fail())
            {
                DK_ASSERT_LOG(false, "Cannot flush or close file writer");
                return false;
            }
        }
        return true;
    }

    bool FileWriter::write(const void* data, size_t bytes)
    {
        if (isOpen() == false)
        {
            DK_ASSERT_LOG(false, "Cannot write closed file");
            return false;
        }
        if (bytes > 0)
        {
            _stream.write(static_cast<const char*>(data), static_cast<std::streamsize>(bytes));
        }
        if (_stream.fail())
        {
            DK_ASSERT_LOG(false, "Cannot write file bytes");
            return false;
        }
        return true;
    }
}
