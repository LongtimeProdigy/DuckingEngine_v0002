#include "CommonLog.h"
#include "TerrainData.h"
#include "FileUtil.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace DK
{
    constexpr uint64_t maxChunkBytes = 64ull * 1024 * 1024;
    static uint32_t columns(const ArchiveHeader& h)
    {
        return (h._width - 2) / h._chunkCells + 1;
    }
    static uint32_t rows(const ArchiveHeader& h)
    {
        return (h._height - 2) / h._chunkCells + 1;
    }

    static bool validHeader(const ArchiveHeader& h)
    {
        const bool version3 = h._version == 3 && std::memcmp(h._magic, "DKTRN003", 8) == 0 && (h._chunkCells & (h._chunkCells - 1)) == 0;
        return version3 && h._headerBytes == sizeof(ArchiveHeader) && h._directoryOffset == sizeof(ArchiveHeader) && h._width >= 2 && h._height >= 2 && h._width <= 1048576 && h._height <= 1048576 && h._chunkCells >= 1 && h._chunkCells <= 512 && h._reserved == 0 && std::isfinite(h._sampleSpacing) && h._sampleSpacing > 0 && std::isfinite(h._heightOffset) && std::isfinite(h._heightScale) && h._heightScale > 0 && std::isfinite(h._heightOffset + h._heightScale);
    }
    template<class T> static bool read(FileReader& in, T& value)
    {
        return in.read(&value, sizeof(value));
    }
    template<class T> static void write(FileWriter& out, const T& value)
    {
        out.write(&value, sizeof(value));
    }
    static bool validLayout(const SectionInfo& s, const ChunkInfo& c, const ArchiveHeader& h)
    {
        const uint32_t vertices = (c._cellsX + 1) * (c._cellsZ + 1);
        uint32_t count = vertices, stride = 0;
        Format format = Format::R16UInt;
        switch (s._type)
        {
        case SectionType::Height:
        case SectionType::Biome:
            stride = 2;
            break;
        case SectionType::Normal:
            stride = 4;
            format = Format::RG16SNorm;
            break;
        case SectionType::MaterialIds:
            stride = 8;
            format = Format::RGBA16UInt;
            break;
        case SectionType::MaterialWeights:
            stride = 4;
            format = Format::RGBA8UNorm;
            break;
        case SectionType::Holes:
            stride = 1;
            format = Format::R8UInt;
            count = c._cellsX * c._cellsZ;
            break;
        case SectionType::MeshReferences:
            stride = sizeof(MeshReference);
            format = Format::MeshReference;
            count = s._count;
            break;
        case SectionType::HeightPyramid:
        case SectionType::NormalPyramid:
            count = 0;
            for (uint32_t lod = 1; lod < mipCount(h._chunkCells); ++lod)
            {
                const uint32_t side = (h._chunkCells >> lod) + 1;
                count += side * side;
            }
            stride = s._type == SectionType::HeightPyramid ? 2 : 4;
            format = s._type == SectionType::HeightPyramid ? Format::R16UInt : Format::RG16SNorm;
            break;
        default:
            return false;
        }
        return s._stride == stride && s._format == format && s._count == count && s._bytes == uint64_t(count) * stride && s._bytes <= maxChunkBytes;
    }
    template<class T> static Section makeSection(SectionType type, Format format, const DKVector<T>& values)
    {
        Section s;
        s._info._type = type;
        s._info._format = format;
        s._info._count = static_cast<uint32_t>(values.size());
        s._info._stride = sizeof(T);
        s._info._bytes = values.size() * sizeof(T);
        s._data.resize(static_cast<size_t>(s._info._bytes));
        if (s._data.empty() == false)
        {
            DK::memcpy(s._data.data(), values.data(), s._data.size());
        }
        return s;
    }

    const Section* Chunk::find(SectionType type) const
    {
        for (const Section& s : _sections)
        {
            if (s._info._type == type)
            {
                return &s;
            }
        }
        return nullptr;
    }

    bool Archive::open(const DKString& path)
    {
        _path.clear();
        _header = {};
        _chunks.clear();
        _sections.clear();

        FileReader in(path);
        if (in.isOpen() == false)
        {
            DK_ASSERT_LOG(false, "Cannot open terrain archive");
            return false;
        }
        const uint64_t length = in.size();
        if (length < sizeof(ArchiveHeader))
        {
            DK_ASSERT_LOG(false, "Truncated terrain header");
            return false;
        }
        ArchiveHeader h;
        if (read(in, h) == false || validHeader(h) == false)
        {
            DK_ASSERT_LOG(false, "Unsupported or invalid terrain header");
            return false;
        }
        const uint64_t count = uint64_t(columns(h)) * rows(h);
        if (count > 1048576 || (h._chunkCount == count) == false || (h._fileBytes == static_cast<uint64_t>(length)) == false)
        {
            DK_ASSERT_LOG(false, "Invalid terrain file size or chunk count");
            return false;
        }
        const uint64_t directoryEnd = h._directoryOffset + count * sizeof(ChunkInfo);
        if (directoryEnd > h._fileBytes)
        {
            DK_ASSERT_LOG(false, "Truncated terrain directory");
            return false;
        }
        DKVector<ChunkInfo> chunks(h._chunkCount);
        for (ChunkInfo& c : chunks)
        {
            if (read(in, c) == false)
            {
                DK_ASSERT_LOG(false, "Truncated chunk record");
                return false;
            }
        }
        DKVector<DKVector<SectionInfo>> sections(h._chunkCount);
        uint64 next = directoryEnd;
        for (uint32 i = 0; i < h._chunkCount; ++i)
        {
            const ChunkInfo& c = chunks[i];
            const uint32 x = i % columns(h);
            const uint32 z = i / columns(h);
            if ((c._x == x) == false || (c._z == z) == false || c._reserved > 0 || c._sectionCount == 0 || c._sectionCount > 9 || (c._cellsX == std::min(h._chunkCells, h._width - 1 - x * h._chunkCells)) == false || (c._cellsZ == std::min(h._chunkCells, h._height - 1 - z * h._chunkCells)) == false || std::isfinite(c._minY) == false || std::isfinite(c._maxY) == false || c._minY > c._maxY || c._minY < h._heightOffset || c._maxY > h._heightOffset + h._heightScale || (c._sectionsOffset == next) == false || c._endOffset > h._fileBytes || c._endOffset < next || c._endOffset - next > maxChunkBytes)
            {
                DK_ASSERT_LOG(false, "Invalid chunk bounds, coordinates or offsets");
                return false;
            }

            next += uint64(c._sectionCount) * sizeof(SectionInfo);
            if (next > c._endOffset)
            {
                DK_ASSERT_LOG(false, "Truncated section table");
                return false;
            }
            if (in.seek(c._sectionsOffset) == false)
            {
                DK_ASSERT_LOG(false, "Cannot seek terrain section table");
                return false;
            }

            uint32 seen = 0;
            for (uint32 j = 0; j < c._sectionCount; ++j)
            {
                SectionInfo s;
                if (read(in, s) == false || validLayout(s, c, h) == false)
                {
                    DK_ASSERT_LOG(false, "Invalid section format or size");
                    return false;
                }

                const uint32 bit = 1u << static_cast<uint32>(s._type);
                if ((seen & bit) || (s._offset == next) == false || s._bytes > c._endOffset - next)
                {
                    DK_ASSERT_LOG(false, "Duplicate or overlapping terrain section");
                    return false;
                }

                seen |= bit;
                next += s._bytes;
                sections[i].push_back(s);
            }

            if ((seen & (1u << static_cast<uint32>(SectionType::Height))) == 0 || (next == c._endOffset) == false || (bool(seen & (1u << 3)) == bool(seen & (1u << 4))) == false)
            {
                DK_ASSERT_LOG(false, "Missing heights, incomplete material layers or invalid chunk size");
                return false;
            }
            if (((seen & (1u << 8)) == 0 || (seen & (1u << 9)) == 0 || (seen & (1u << 2)) == 0))
            {
                DK_ASSERT_LOG(false, "Missing terrain mip pyramid or normals");
                return false;
            }
        }

        if ((next == h._fileBytes) == false)
        {
            DK_ASSERT_LOG(false, "Unexpected trailing terrain data");
            return false;
        }

        _path = path;
        _header = h;
        _chunks = DK::move(chunks);
        _sections = DK::move(sections);
        return true;
    }

    bool Archive::loadChunk(uint32_t x, uint32_t z, Chunk& out) const
    {
        if (isOpen() == false || x >= columns(_header) || z >= rows(_header))
        {
            DK_ASSERT_LOG(false, "Chunk is outside terrain");
            return false;
        }

        FileReader in(_path);
        if (in.isOpen() == false)
        {
            DK_ASSERT_LOG(false, "Cannot reopen terrain archive");
            return false;
        }

        const uint32_t index = z * columns(_header) + x;
        Chunk loaded;
        loaded._info = _chunks[index];
        for (const SectionInfo& info : _sections[index])
        {
            Section section;
            section._info = info;
            section._data.resize(static_cast<size_t>(info._bytes));
            if (in.seek(info._offset) == false || in.read(section._data.data(), section._data.size()) == false)
            {
                DK_ASSERT_LOG(false, "Truncated terrain payload");
                return false;
            }
            loaded._sections.push_back(DK::move(section));
        }
        out = DK::move(loaded);
        return true;
    }

    bool writeArchive(const DKString& path, ArchiveHeader h, const DKVector<Chunk>& chunks)
    {
        if (validHeader(h) == false || (chunks.size() == uint64_t(columns(h)) * rows(h)) == false || chunks.size() > 1048576)
        {
            DK_ASSERT_LOG(false, "Invalid terrain build settings or chunk count");
            return false;
        }

        h._chunkCount = static_cast<uint32>(chunks.size());
        DKVector<ChunkInfo> infos;
        DKVector<DKVector<SectionInfo>> tables;
        uint64 offset = sizeof(ArchiveHeader) + chunks.size() * sizeof(ChunkInfo);
        for (const Chunk& c : chunks)
        {
            ChunkInfo info = c._info;
            if (c._sections.empty() || c._sections.size() > 9)
            {
                DK_ASSERT_LOG(false, "Invalid section count");
                return false;
            }

            info._sectionCount = static_cast<uint32>(c._sections.size());
            info._sectionsOffset = offset;
            offset += c._sections.size() * sizeof(SectionInfo);
            DKVector<SectionInfo> table;
            for (const Section& section : c._sections)
            {
                SectionInfo s = section._info;
                if ((section._data.size() == s._bytes) == false)
                {
                    DK_ASSERT_LOG(false, "Section payload size mismatch");
                    return false;
                }
                s._offset = offset;
                offset += s._bytes;
                table.push_back(s);
            }
            info._endOffset = offset;
            infos.push_back(info);
            tables.push_back(DK::move(table));
        }

        h._fileBytes = offset;

        DKString temporary = path;
        temporary += ".tmp";
        if (FileUtil::exist(path) || FileUtil::exist(temporary))
        {
            DK_ASSERT_LOG(false, "Output or temporary archive already exists; choose a new output path");
            return false;
        }

        FileWriter out(temporary);
        if (out.isOpen() == false)
        {
            DK_ASSERT_LOG(false, "Cannot create terrain archive");
            return false;
        }

        write(out, h);

        for (const ChunkInfo& info : infos)
            write(out, info);
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            for (const SectionInfo& s : tables[i])
                write(out, s);
            for (const Section& s : chunks[i]._sections)
                out.write(s._data.data(), s._data.size());
        }

        const bool good = out.close();
        Archive check;
        if (good == false || check.open(temporary) == false)
        {
            FileUtil::remove(temporary);
            if (good == false)
            {
                DK_ASSERT_LOG(false, "Failed writing terrain archive");
                return false;
            }
            return false;
        }

        if (FileUtil::rename(temporary, path) == false)
        {
            FileUtil::remove(temporary);
            DK_ASSERT_LOG(false, "Cannot publish terrain archive");
            return false;
        }

        return true;
    }

    static float heightAt(const ArchiveHeader& h, const DKVector<uint16_t>& heights, uint32_t x, uint32_t z)
    {
        return h._heightOffset + h._heightScale * (float(heights[size_t(z) * h._width + x]) / 65535.0f);
    }

    static DKArray<int16_t, 2> normalAt(const ArchiveHeader& h, const DKVector<uint16_t>& heights, uint32_t x, uint32_t z, uint32_t step)
    {
        const uint32_t left = x >= step ? x - step : 0, right = std::min(x + step, h._width - 1);
        const uint32_t top = z >= step ? z - step : 0, bottom = std::min(z + step, h._height - 1);
        const double nx = -(double(heightAt(h, heights, right, z)) - heightAt(h, heights, left, z)) / ((right - left) * double(h._sampleSpacing));
        const double nz = -(double(heightAt(h, heights, x, bottom)) - heightAt(h, heights, x, top)) / ((bottom - top) * double(h._sampleSpacing));
        const double length = std::sqrt(nx * nx + 1 + nz * nz);
        return DKArray<int16_t, 2>{ static_cast<int16_t>(std::lround(nx / length * 32767)), static_cast<int16_t>(std::lround(nz / length * 32767)) };
    }

    bool buildHeightfield(const ArchiveHeader& h, const DKVector<uint16_t>& heights, DKVector<Chunk>& out)
    {
        if (validHeader(h) == false || (heights.size() == uint64_t(h._width) * h._height) == false || uint64_t(columns(h)) * rows(h) > 1048576)
        {
            DK_ASSERT_LOG(false, "Invalid heightfield dimensions or build settings");
            return false;
        }
        DKVector<Chunk> built;
        for (uint32_t z = 0; z < rows(h); ++z) for (uint32_t x = 0; x < columns(h); ++x)
        {
            Chunk c;
            c._info._x = x;
            c._info._z = z;
            c._info._cellsX = std::min(h._chunkCells, h._width - 1 - x * h._chunkCells);
            c._info._cellsZ = std::min(h._chunkCells, h._height - 1 - z * h._chunkCells);
            c._info._minY = std::numeric_limits<float>::max();
            c._info._maxY = std::numeric_limits<float>::lowest();
            DKVector<uint16_t> samples;
            DKVector<DKArray<int16_t, 2>> normals;
            for (uint32_t v = 0; v <= c._info._cellsZ; ++v) for (uint32_t u = 0; u <= c._info._cellsX; ++u)
            {
                const uint32_t sx = x * h._chunkCells + u, sz = z * h._chunkCells + v;
                samples.push_back(heights[size_t(sz) * h._width + sx]);
                const float y = heightAt(h, heights, sx, sz);
                c._info._minY = std::min(c._info._minY, y);
                c._info._maxY = std::max(c._info._maxY, y);
                normals.push_back(normalAt(h, heights, sx, sz, 1));
            }
            c._sections.push_back(makeSection(SectionType::Height, Format::R16UInt, samples));
            c._sections.push_back(makeSection(SectionType::Normal, Format::RG16SNorm, normals));
            samples.clear();
            normals.clear();
            for (uint32_t lod = 1; lod < mipCount(h._chunkCells); ++lod)
            {
                const uint32_t step = 1u << lod, side = (h._chunkCells >> lod) + 1;
                for (uint32_t v = 0; v < side; ++v) for (uint32_t u = 0; u < side; ++u)
                {
                    const uint32_t sx = std::min(x * h._chunkCells + u * step, h._width - 1), sz = std::min(z * h._chunkCells + v * step, h._height - 1);
                    samples.push_back(heights[size_t(sz) * h._width + sx]);
                    normals.push_back(normalAt(h, heights, sx, sz, step));
                }
            }
            c._sections.push_back(makeSection(SectionType::HeightPyramid, Format::R16UInt, samples));
            c._sections.push_back(makeSection(SectionType::NormalPyramid, Format::RG16SNorm, normals));
            built.push_back(DK::move(c));
        }
        out = DK::move(built);
        return true;
    }

    uint32_t mipCount(uint32_t chunkCells)
    {
        uint32_t count = 1;
        while (chunkCells > 1)
        {
            chunkCells >>= 1;
            ++count;
        }
        return count;
    }

    bool Archive::loadMip(uint32_t x, uint32_t z, uint32_t lod, MipTile& out) const
    {
        if (isOpen() == false || x >= columns(_header) || z >= rows(_header) || lod >= mipCount(_header._chunkCells))
        {
            DK_ASSERT_LOG(false, "Terrain mip is outside archive");
            return false;
        }

        const uint32_t index = z * columns(_header) + x;
        const ChunkInfo& chunk = _chunks[index];
        const SectionType heightType = lod == 0 ? SectionType::Height : SectionType::HeightPyramid;
        const SectionType normalType = lod == 0 ? SectionType::Normal : SectionType::NormalPyramid;
        const SectionInfo* heightInfo = nullptr;
        const SectionInfo* normalInfo = nullptr;
        for (const SectionInfo& s : _sections[index])
        {
            if (s._type == heightType)
            {
                heightInfo = &s;
            }
            if (s._type == normalType)
            {
                normalInfo = &s;
            }
        }
        if (heightInfo == nullptr || normalInfo == nullptr)
        {
            DK_ASSERT_LOG(false, "Terrain mip sections are missing; rebuild the archive");
            return false;
        }

        uint64_t first = 0;
        for (uint32_t level = 1; level < lod; ++level)
        {
            const uint32_t side = (_header._chunkCells >> level) + 1;
            first += uint64_t(side) * side;
        }

        const uint32_t sourceWidth = lod == 0 ? chunk._cellsX + 1 : (_header._chunkCells >> lod) + 1;
        const uint32_t sourceHeight = lod == 0 ? chunk._cellsZ + 1 : sourceWidth;
        const size_t count = size_t(sourceWidth) * sourceHeight;
        DKVector<uint16_t> heights(count);
        DKVector<DKArray<int16_t, 2>> normals(count);
        FileReader in(_path);
        if (in.seek(heightInfo->_offset + first * 2) == false || in.read(heights.data(), count * 2) == false || in.seek(normalInfo->_offset + first * 4) == false || in.read(normals.data(), count * 4) == false)
        {
            DK_ASSERT_LOG(false, "Cannot read terrain mip payload");
            return false;
        }

        MipTile tile;
        tile._x = x;
        tile._z = z;
        tile._lod = lod;
        tile._side = (_header._chunkCells >> lod) + 1;
        tile._heights.resize(size_t(tile._side) * tile._side);
        tile._normals.resize(tile._heights.size());
        for (uint32_t v = 0; v < tile._side; ++v)
        {
            for (uint32 u = 0; u < tile._side; ++u)
            {
                const size_t source = size_t(std::min(v, sourceHeight - 1)) * sourceWidth + std::min(u, sourceWidth - 1), destination = size_t(v) * tile._side + u;
                tile._heights[destination] = heights[source];
                tile._normals[destination] = normals[source];
            }
        }

        out = DK::move(tile);
        return true;
    }

    bool Archive::loadGrid(uint32 startX, uint32 startZ, uint32 lod, uint32 width, uint32 height, TerrainOverview& out) const
    {
        const uint32 step = 1u << lod;
        const uint32 sourceLod = std::min(lod, mipCount(_header._chunkCells) - 1);
        const uint32 sourceStep = 1u << sourceLod;
        const uint32 sourceSide = (_header._chunkCells >> sourceLod) + 1;
        const uint32 gridWidth = width + 2;
        const uint32 gridHeight = height + 2;
        DKVector<uint32> positionsX(gridWidth);
        DKVector<uint32> positionsZ(gridHeight);
        for (uint32 x = 0; x < gridWidth; ++x)
            positionsX[x] = static_cast<uint32>(std::clamp(int64(startX) + (int64(x) - 1) * step, int64(0), int64(_header._width - 1)));
        for (uint32 z = 0; z < gridHeight; ++z)
            positionsZ[z] = static_cast<uint32>(std::clamp(int64(startZ) + (int64(z) - 1) * step, int64(0), int64(_header._height - 1)));

        uint64 mipOffset = 0;
        for (uint32 level = 1; level < sourceLod; ++level)
        {
            const uint32 side = (_header._chunkCells >> level) + 1;
            mipOffset += uint64(side) * side;
        }

        FileReader reader(_path);
        if (reader.isOpen() == false)
            return false;

        DKVector<uint16> samples(size_t(gridWidth) * gridHeight);
        const uint32 chunkColumns = columns(_header);
        const uint32 chunkRows = rows(_header);
        for (uint32 z = 0; z < gridHeight; ++z)
        {
            const uint32 chunkZ = std::min(positionsZ[z] / _header._chunkCells, chunkRows - 1);
            const uint32 localZ = positionsZ[z] - chunkZ * _header._chunkCells;
            const uint32 sampleZ = (localZ + sourceStep - 1) / sourceStep;
            for (uint32 x = 0; x < gridWidth;)
            {
                const uint32 chunkX = std::min(positionsX[x] / _header._chunkCells, chunkColumns - 1);
                const uint32 localX = positionsX[x] - chunkX * _header._chunkCells;
                const uint32 sampleX = (localX + sourceStep - 1) / sourceStep;
                const uint32 id = chunkZ * chunkColumns + chunkX;
                const uint32 rowWidth = sourceLod == 0 ? _chunks[id]._cellsX + 1 : sourceSide;
                uint32 count = 1;
                while (x + count < gridWidth && positionsX[x + count] == positionsX[x] + count * sourceStep && std::min(positionsX[x + count] / _header._chunkCells, chunkColumns - 1) == chunkX)
                {
                    ++count;
                }

                const SectionType type = sourceLod == 0 ? SectionType::Height : SectionType::HeightPyramid;
                const DKVector<SectionInfo>::const_iterator section = std::find_if(_sections[id].begin(), _sections[id].end(), [type](const SectionInfo& value) { return value._type == type; });
                if (section == _sections[id].end())
                {
                    DK_ASSERT_LOG(false, "Terrain height mip is missing");
                    return false;
                }

                const uint64 offset = section->_offset + (mipOffset + uint64(sampleZ) * rowWidth + sampleX) * 2;
                if (reader.seek(offset) == false || reader.read(samples.data() + size_t(z) * gridWidth + x, size_t(count) * 2) == false)
                {
                    return false;
                }

                x += count;
            }
        }

        TerrainOverview result;
        result._width = width;
        result._height = height;
        result._lod = lod;
        result._heights.resize(size_t(width) * height);
        result._normals.resize(result._heights.size());
        for (uint32 z = 0; z < height; ++z)
        {
            for (uint32 x = 0; x < width; ++x)
            {
                const size_t center = size_t(z + 1) * gridWidth + x + 1;
                result._heights[size_t(z) * width + x] = samples[center];
                const float left = _header._heightOffset + _header._heightScale * (float(samples[center - 1]) / 65535.0f);
                const float right = _header._heightOffset + _header._heightScale * (float(samples[center + 1]) / 65535.0f);
                const float top = _header._heightOffset + _header._heightScale * (float(samples[center - gridWidth]) / 65535.0f);
                const float bottom = _header._heightOffset + _header._heightScale * (float(samples[center + gridWidth]) / 65535.0f);
                const uint32 distanceX = positionsX[x + 2] - positionsX[x];
                const uint32 distanceZ = positionsZ[z + 2] - positionsZ[z];
                const double nx = distanceX > 0 ? -(double(right) - left) / (distanceX * double(_header._sampleSpacing)) : 0;
                const double nz = distanceZ > 0 ? -(double(bottom) - top) / (distanceZ * double(_header._sampleSpacing)) : 0;
                const double length = std::sqrt(nx * nx + 1 + nz * nz);
                result._normals[size_t(z) * width + x] = { static_cast<int16_t>(std::lround(nx / length * 32767)), static_cast<int16_t>(std::lround(nz / length * 32767)) };
            }
        }

        out = DK::move(result);
        return true;
    }

    bool Archive::loadVirtualTile(uint32 x, uint32 z, uint32 lod, MipTile& out) const
    {
        if (isOpen() == false || lod > 20 || uint64(x) * _header._chunkCells * (1ull << lod) >= _header._width - 1 || uint64(z) * _header._chunkCells * (1ull << lod) >= _header._height - 1)
        {
            DK_ASSERT_LOG(false, "Virtual terrain tile is outside archive");
            return false;
        }
        TerrainOverview grid;
        const uint32 startX = (x * _header._chunkCells) << lod, startZ = (z * _header._chunkCells) << lod;
        if (loadGrid(startX, startZ, lod, _header._chunkCells + 1, _header._chunkCells + 1, grid) == false)
        {
            return false;
        }
        MipTile tile;
        tile._x = x;
        tile._z = z;
        tile._lod = lod;
        tile._side = _header._chunkCells + 1;
        tile._heights = DK::move(grid._heights);
        tile._normals = DK::move(grid._normals);
        out = DK::move(tile);
        return true;
    }

    bool Archive::loadOverview(uint32 lod, TerrainOverview& out) const
    {
        if (isOpen() == false || lod > 20)
        {
            DK_ASSERT_LOG(false, "Invalid terrain overview LOD");
            return false;
        }

        const uint32 step = 1u << lod;
        const uint32 width = (_header._width - 2) / step + 2;
        const uint32 height = (_header._height - 2) / step + 2;
        return loadGrid(0, 0, lod, width, height, out);
    }
}
