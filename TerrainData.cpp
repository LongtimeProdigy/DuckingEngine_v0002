#include "CommonLog.h"
#include "TerrainData.h"
#include "FileUtil.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace DK
{
#if defined(_DK_DEBUG_)
    constexpr uint64_t maxChunkBytes = 64ull * 1024 * 1024;
#endif
    static uint32_t columns(const ArchiveHeader& h)
    {
        return (h._width - 1) / h._chunkCellCountPerSide;
    }
    static uint32_t rows(const ArchiveHeader& h)
    {
        return (h._height - 1) / h._chunkCellCountPerSide;
    }

#if defined(_DK_DEBUG_)
    static bool validHeader(const ArchiveHeader& h)
    {
        const bool version3 = h._version == 3 && std::memcmp(h._magic, "DKTRN003", sizeof(h._magic)) == 0 && (h._chunkCellCountPerSide & (h._chunkCellCountPerSide - 1)) == 0;
        const bool valid = version3 && h._headerBytes == sizeof(ArchiveHeader) && h._directoryOffset == sizeof(ArchiveHeader) && h._width >= 2 && h._height >= 2 && h._width <= 1048576 && h._height <= 1048576 && h._chunkCellCountPerSide >= 1 && h._chunkCellCountPerSide <= 512 && h._reserved == 0 && std::isfinite(h._sampleWorldSpacing) && h._sampleWorldSpacing > 0 && std::isfinite(h._heightOffset) && std::isfinite(h._heightScale) && h._heightScale > 0 && std::isfinite(h._heightOffset + h._heightScale);
        if (valid == false)
            return false;

        if ((h._width - 1) % h._chunkCellCountPerSide != 0 || (h._height - 1) % h._chunkCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Terrain archive cell counts must be divisible by the chunk cell count");
            return false;
        }
        return true;
    }
#endif
    template<class T> static bool read(FileReader& in, T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Terrain binary data must be trivially copyable");
        return in.read(&value, sizeof(value));
    }
    template<class T> static bool write(FileWriter& out, const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Terrain binary data must be trivially copyable");
        return out.write(&value, sizeof(value));
    }
#if defined(_DK_DEBUG_)
    static bool validLayout(const ChunkDataBlockInfo& s, const ChunkInfo& c, const ArchiveHeader& h)
    {
        const uint32_t vertices = (c._cellCountX + 1) * (c._cellCountZ + 1);
        uint32_t count = vertices, stride = 0;
        Format format = Format::R16UInt;
        switch (s._type)
        {
        case ChunkDataType::Height:
        case ChunkDataType::Biome:
            stride = 2;
            break;
        case ChunkDataType::Normal:
            stride = 4;
            format = Format::RG16SNorm;
            break;
        case ChunkDataType::MaterialIds:
            stride = 8;
            format = Format::RGBA16UInt;
            break;
        case ChunkDataType::MaterialWeights:
            stride = 4;
            format = Format::RGBA8UNorm;
            break;
        case ChunkDataType::Holes:
            stride = 1;
            format = Format::R8UInt;
            count = c._cellCountX * c._cellCountZ;
            break;
        case ChunkDataType::MeshReferences:
            stride = sizeof(MeshReference);
            format = Format::MeshReference;
            count = s._count;
            break;
        case ChunkDataType::HeightPyramid:
        case ChunkDataType::NormalPyramid:
            count = 0;
            for (uint32_t lod = 1; lod < mipCount(h._chunkCellCountPerSide); ++lod)
            {
                const uint32_t side = (h._chunkCellCountPerSide >> lod) + 1;
                count += side * side;
            }
            stride = s._type == ChunkDataType::HeightPyramid ? 2 : 4;
            format = s._type == ChunkDataType::HeightPyramid ? Format::R16UInt : Format::RG16SNorm;
            break;
        default:
            return false;
        }
        return s._stride == stride && s._format == format && s._count == count && s._bytes == uint64_t(count) * stride && s._bytes <= maxChunkBytes;
    }
#endif
    template<class T> static ChunkDataBlock makeDataBlock(ChunkDataType type, Format format, const DKVector<T>& values)
    {
        static_assert(std::is_trivially_copyable_v<T>, "Terrain payload must be trivially copyable");
        ChunkDataBlock s;
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

    const ChunkDataBlock* Chunk::find(ChunkDataType type) const
    {
        for (const ChunkDataBlock& s : _dataBlocks)
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
        _dataBlocks.clear();

        FileReader in(path);
        if (in.isOpen() == false)
        {
            DK_ASSERT_LOG(false, "Cannot open terrain archive");
            return false;
        }
#if defined(_DK_DEBUG_)
        const uint64_t length = in.size();
        if (length < sizeof(ArchiveHeader))
        {
            DK_ASSERT_LOG(false, "Truncated terrain header");
            return false;
        }
#endif
        ArchiveHeader h;
        if (read(in, h) == false)
        {
            DK_ASSERT_LOG(false, "Unsupported or invalid terrain header");
            return false;
        }
#if defined(_DK_DEBUG_)
        if (validHeader(h) == false)
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
#endif
        DKVector<ChunkInfo> chunks(h._chunkCount);
        for (ChunkInfo& c : chunks)
        {
            if (read(in, c) == false)
            {
                DK_ASSERT_LOG(false, "Truncated chunk record");
                return false;
            }
        }
        DKVector<DKVector<ChunkDataBlockInfo>> dataBlocks(h._chunkCount);
#if defined(_DK_DEBUG_)
        uint64 next = directoryEnd;
#endif
        for (uint32 i = 0; i < h._chunkCount; ++i)
        {
            const ChunkInfo& c = chunks[i];
#if defined(_DK_DEBUG_)
            const uint32 x = i % columns(h);
            const uint32 z = i / columns(h);
            if ((c._x == x) == false || (c._z == z) == false || c._reserved > 0 || c._dataBlockCount == 0 || c._dataBlockCount > static_cast<uint32>(ChunkDataType::NormalPyramid) || c._cellCountX != h._chunkCellCountPerSide || c._cellCountZ != h._chunkCellCountPerSide || std::isfinite(c._minY) == false || std::isfinite(c._maxY) == false || c._minY > c._maxY || c._minY < h._heightOffset || c._maxY > h._heightOffset + h._heightScale || (c._dataBlocksOffset == next) == false || c._endOffset > h._fileBytes || c._endOffset < next || c._endOffset - next > maxChunkBytes)
            {
                DK_ASSERT_LOG(false, "Invalid chunk bounds, coordinates or offsets");
                return false;
            }

            next += uint64(c._dataBlockCount) * sizeof(ChunkDataBlockInfo);
            if (next > c._endOffset)
            {
                DK_ASSERT_LOG(false, "Truncated chunk data block table");
                return false;
            }
#endif
            if (in.seek(c._dataBlocksOffset) == false)
            {
                DK_ASSERT_LOG(false, "Cannot seek terrain chunk data block table");
                return false;
            }

#if defined(_DK_DEBUG_)
            uint32 seen = 0;
#endif
            for (uint32 j = 0; j < c._dataBlockCount; ++j)
            {
                ChunkDataBlockInfo s;
                if (read(in, s) == false)
                {
                    DK_ASSERT_LOG(false, "Invalid chunk data block format or size");
                    return false;
                }

#if defined(_DK_DEBUG_)
                if (validLayout(s, c, h) == false)
                {
                    DK_ASSERT_LOG(false, "Invalid chunk data block format or size");
                    return false;
                }
                const uint32 bit = 1u << static_cast<uint32>(s._type);
                if ((seen & bit) || (s._offset == next) == false || s._bytes > c._endOffset - next)
                {
                    DK_ASSERT_LOG(false, "Duplicate or overlapping terrain chunk data block");
                    return false;
                }

                seen |= bit;
                next += s._bytes;
#endif
                dataBlocks[i].push_back(s);
            }

#if defined(_DK_DEBUG_)
            if ((seen & (1u << static_cast<uint32>(ChunkDataType::Height))) == 0 || (next == c._endOffset) == false || (bool(seen & (1u << static_cast<uint32>(ChunkDataType::MaterialIds))) == bool(seen & (1u << static_cast<uint32>(ChunkDataType::MaterialWeights)))) == false)
            {
                DK_ASSERT_LOG(false, "Missing heights, incomplete material layers or invalid chunk size");
                return false;
            }
            if (((seen & (1u << static_cast<uint32>(ChunkDataType::HeightPyramid))) == 0 || (seen & (1u << static_cast<uint32>(ChunkDataType::NormalPyramid))) == 0 || (seen & (1u << static_cast<uint32>(ChunkDataType::Normal))) == 0))
            {
                DK_ASSERT_LOG(false, "Missing terrain mip pyramid or normals");
                return false;
            }
#endif
        }

#if defined(_DK_DEBUG_)
        if ((next == h._fileBytes) == false)
        {
            DK_ASSERT_LOG(false, "Unexpected trailing terrain data");
            return false;
        }
#endif

        _path = path;
        _header = h;
        _chunks = DK::move(chunks);
        _dataBlocks = DK::move(dataBlocks);
        return true;
    }

    bool Archive::loadChunk(uint32_t x, uint32_t z, Chunk& out) const
    {
#if defined(_DK_DEBUG_)
        if (isOpen() == false || x >= columns(_header) || z >= rows(_header))
        {
            DK_ASSERT_LOG(false, "Chunk is outside terrain");
            return false;
        }
#endif

        FileReader in(_path);
        if (in.isOpen() == false)
        {
            DK_ASSERT_LOG(false, "Cannot reopen terrain archive");
            return false;
        }

        const uint32_t index = z * columns(_header) + x;
        Chunk loaded;
        loaded._info = _chunks[index];
        for (const ChunkDataBlockInfo& info : _dataBlocks[index])
        {
            ChunkDataBlock dataBlock;
            dataBlock._info = info;
            dataBlock._data.resize(static_cast<size_t>(info._bytes));
            if (in.seek(info._offset) == false || in.read(dataBlock._data.data(), dataBlock._data.size()) == false)
            {
                DK_ASSERT_LOG(false, "Truncated terrain payload");
                return false;
            }
            loaded._dataBlocks.push_back(DK::move(dataBlock));
        }
        out = DK::move(loaded);
        return true;
    }

    bool writeArchive(const DKString& path, ArchiveHeader h, const DKVector<Chunk>& chunks)
    {
#if defined(_DK_DEBUG_)
        if (validHeader(h) == false || (chunks.size() == uint64_t(columns(h)) * rows(h)) == false || chunks.size() > 1048576)
        {
            DK_ASSERT_LOG(false, "Invalid terrain build settings or chunk count");
            return false;
        }
#endif

        h._chunkCount = static_cast<uint32>(chunks.size());
        DKVector<ChunkInfo> infos;
        DKVector<DKVector<ChunkDataBlockInfo>> tables;
        uint64 offset = sizeof(ArchiveHeader) + chunks.size() * sizeof(ChunkInfo);
        for (const Chunk& c : chunks)
        {
            ChunkInfo info = c._info;
#if defined(_DK_DEBUG_)
            if (info._x != infos.size() % columns(h) || info._z != infos.size() / columns(h) || info._cellCountX != h._chunkCellCountPerSide || info._cellCountZ != h._chunkCellCountPerSide)
            {
                DK_ASSERT_LOG(false, "Terrain archive chunks must fill the uniform chunk grid in row order");
                return false;
            }
            if (c.find(ChunkDataType::Height) == nullptr || c.find(ChunkDataType::Normal) == nullptr || c.find(ChunkDataType::HeightPyramid) == nullptr || c.find(ChunkDataType::NormalPyramid) == nullptr || (c.find(ChunkDataType::MaterialIds) == nullptr) != (c.find(ChunkDataType::MaterialWeights) == nullptr))
            {
                DK_ASSERT_LOG(false, "Terrain chunk requires height and normal mip data and complete material layers before writing");
                return false;
            }
            if (c._dataBlocks.empty() || c._dataBlocks.size() > 9)
            {
                DK_ASSERT_LOG(false, "Invalid chunk data block count");
                return false;
            }
#endif

            info._dataBlockCount = static_cast<uint32>(c._dataBlocks.size());
            info._dataBlocksOffset = offset;
            offset += c._dataBlocks.size() * sizeof(ChunkDataBlockInfo);
            DKVector<ChunkDataBlockInfo> table;
#if defined(_DK_DEBUG_)
            uint32 seen = 0;
#endif
            for (const ChunkDataBlock& dataBlock : c._dataBlocks)
            {
                ChunkDataBlockInfo s = dataBlock._info;
#if defined(_DK_DEBUG_)
                if (dataBlock._data.size() != s._bytes || validLayout(s, info, h) == false)
                {
                    DK_ASSERT_LOG(false, "Terrain data block format, sample count or payload size does not match the chunk layout");
                    return false;
                }
                const uint32 bit = 1u << static_cast<uint32>(s._type);
                if ((seen & bit) != 0)
                {
                    DK_ASSERT_LOG(false, "Terrain chunk data block types must be unique before writing");
                    return false;
                }
                seen |= bit;
#endif
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

        if (write(out, h) == false)
            return false;

        for (const ChunkInfo& info : infos)
        {
            if (write(out, info) == false)
                return false;
        }
        for (size_t i = 0; i < chunks.size(); ++i)
        {
            for (const ChunkDataBlockInfo& dataBlockInfo : tables[i])
            {
                if (write(out, dataBlockInfo) == false)
                    return false;
            }
            for (const ChunkDataBlock& dataBlock : chunks[i]._dataBlocks)
            {
                if (out.write(dataBlock._data.data(), dataBlock._data.size()) == false)
                    return false;
            }
        }

        const bool good = out.close();
        if (good == false)
        {
            FileUtil::remove(temporary);
            DK_ASSERT_LOG(false, "Failed writing terrain archive");
            return false;
        }
#if defined(_DK_DEBUG_)
        Archive check;
        if (check.open(temporary) == false)
        {
            FileUtil::remove(temporary);
            return false;
        }
#endif

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
        const double nx = -(double(heightAt(h, heights, right, z)) - heightAt(h, heights, left, z)) / ((right - left) * double(h._sampleWorldSpacing));
        const double nz = -(double(heightAt(h, heights, x, bottom)) - heightAt(h, heights, x, top)) / ((bottom - top) * double(h._sampleWorldSpacing));
        const double length = std::sqrt(nx * nx + 1 + nz * nz);
        return DKArray<int16_t, 2>{ static_cast<int16_t>(std::lround(nx / length * 32767)), static_cast<int16_t>(std::lround(nz / length * 32767)) };
    }

    bool buildHeightfield(const ArchiveHeader& h, const DKVector<uint16_t>& heights, DKVector<Chunk>& out)
    {
#if defined(_DK_DEBUG_)
        if (validHeader(h) == false || (heights.size() == uint64_t(h._width) * h._height) == false || uint64_t(columns(h)) * rows(h) > 1048576)
        {
            DK_ASSERT_LOG(false, "Invalid heightfield dimensions or build settings");
            return false;
        }
#endif
        DKVector<Chunk> built;
        for (uint32_t z = 0; z < rows(h); ++z)
        {
            for (uint32_t x = 0; x < columns(h); ++x)
            {
                Chunk c;
                c._info._x = x;
                c._info._z = z;
                c._info._cellCountX = h._chunkCellCountPerSide;
                c._info._cellCountZ = h._chunkCellCountPerSide;
                c._info._minY = std::numeric_limits<float>::max();
                c._info._maxY = std::numeric_limits<float>::lowest();
                DKVector<uint16_t> samples;
                DKVector<DKArray<int16_t, 2>> normals;
                for (uint32_t v = 0; v <= c._info._cellCountZ; ++v)
                {
                    for (uint32_t u = 0; u <= c._info._cellCountX; ++u)
                    {
                        const uint32_t sx = x * h._chunkCellCountPerSide + u, sz = z * h._chunkCellCountPerSide + v;
                        samples.push_back(heights[size_t(sz) * h._width + sx]);
                        const float y = heightAt(h, heights, sx, sz);
                        c._info._minY = std::min(c._info._minY, y);
                        c._info._maxY = std::max(c._info._maxY, y);
                        normals.push_back(normalAt(h, heights, sx, sz, 1));
                    }
                }
                c._dataBlocks.push_back(makeDataBlock(ChunkDataType::Height, Format::R16UInt, samples));
                c._dataBlocks.push_back(makeDataBlock(ChunkDataType::Normal, Format::RG16SNorm, normals));
                samples.clear();
                normals.clear();
                for (uint32_t lod = 1; lod < mipCount(h._chunkCellCountPerSide); ++lod)
                {
                    const uint32_t step = 1u << lod, side = (h._chunkCellCountPerSide >> lod) + 1;
                    for (uint32_t v = 0; v < side; ++v)
                    {
                        for (uint32_t u = 0; u < side; ++u)
                        {
                            const uint32_t sx = x * h._chunkCellCountPerSide + u * step, sz = z * h._chunkCellCountPerSide + v * step;
                            samples.push_back(heights[size_t(sz) * h._width + sx]);
                            normals.push_back(normalAt(h, heights, sx, sz, step));
                        }
                    }
                }
                c._dataBlocks.push_back(makeDataBlock(ChunkDataType::HeightPyramid, Format::R16UInt, samples));
                c._dataBlocks.push_back(makeDataBlock(ChunkDataType::NormalPyramid, Format::RG16SNorm, normals));
                built.push_back(DK::move(c));
            }
        }
        out = DK::move(built);
        return true;
    }

    uint32_t mipCount(uint32_t chunkCellCountPerSide)
    {
        uint32_t count = 1;
        while (chunkCellCountPerSide > 1)
        {
            chunkCellCountPerSide >>= 1;
            ++count;
        }
        return count;
    }

    bool Archive::loadMip(uint32_t x, uint32_t z, uint32_t lod, MipTile& out) const
    {
#if defined(_DK_DEBUG_)
        if (isOpen() == false || x >= columns(_header) || z >= rows(_header) || lod >= mipCount(_header._chunkCellCountPerSide))
        {
            DK_ASSERT_LOG(false, "Terrain mip is outside archive");
            return false;
        }
#endif

        const uint32_t index = z * columns(_header) + x;
        DK_ASSERT_LOG(_chunks[index]._cellCountX == _header._chunkCellCountPerSide && _chunks[index]._cellCountZ == _header._chunkCellCountPerSide, "Terrain mip must use the full uniform chunk cell count");
        const ChunkDataType heightType = lod == 0 ? ChunkDataType::Height : ChunkDataType::HeightPyramid;
        const ChunkDataType normalType = lod == 0 ? ChunkDataType::Normal : ChunkDataType::NormalPyramid;
        const ChunkDataBlockInfo* heightInfo = nullptr;
        const ChunkDataBlockInfo* normalInfo = nullptr;
        for (const ChunkDataBlockInfo& s : _dataBlocks[index])
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
#if defined(_DK_DEBUG_)
        if (heightInfo == nullptr || normalInfo == nullptr)
        {
            DK_ASSERT_LOG(false, "Terrain mip data blocks are missing; rebuild the archive");
            return false;
        }
#endif

        uint64_t first = 0;
        for (uint32_t level = 1; level < lod; ++level)
        {
            const uint32_t side = (_header._chunkCellCountPerSide >> level) + 1;
            first += uint64_t(side) * side;
        }

        const uint32 sampleCountPerSide = (_header._chunkCellCountPerSide >> lod) + 1;
        const size_t count = size_t(sampleCountPerSide) * sampleCountPerSide;
        DK_ASSERT_LOG(first + count <= heightInfo->_count && first + count <= normalInfo->_count, "Terrain mip samples must fit inside both data blocks");
        DKVector<uint16_t> heights(count);
        DKVector<DKArray<int16_t, 2>> normals(count);
        FileReader in(_path);
        if (in.seek(heightInfo->_offset + first * sizeof(uint16)) == false || in.read(heights.data(), count * sizeof(uint16)) == false || in.seek(normalInfo->_offset + first * sizeof(normals[0])) == false || in.read(normals.data(), count * sizeof(normals[0])) == false)
        {
            DK_ASSERT_LOG(false, "Cannot read terrain mip payload");
            return false;
        }

        MipTile tile;
        tile._x = x;
        tile._z = z;
        tile._lod = lod;
        tile._sampleCountPerSide = sampleCountPerSide;
        tile._heights = DK::move(heights);
        tile._normals = DK::move(normals);

        out = DK::move(tile);
        return true;
    }

    bool Archive::loadGrid(uint32 startX, uint32 startZ, uint32 lod, uint32 width, uint32 height, TerrainOverview& out) const
    {
        const uint32 step = 1u << lod;
        const uint32 sourceLod = std::min(lod, mipCount(_header._chunkCellCountPerSide) - 1);
        const uint32 sourceStep = 1u << sourceLod;
        if (width == 0 || height == 0 || startX % sourceStep != 0 || startZ % sourceStep != 0 || uint64(startX) + uint64(width - 1) * step > _header._width - 1 || uint64(startZ) + uint64(height - 1) * step > _header._height - 1)
        {
            DK_ASSERT_LOG(false, "Terrain sample grid must be aligned to the source LOD and fit entirely inside the archive");
            return false;
        }
        const uint32 sourceSide = (_header._chunkCellCountPerSide >> sourceLod) + 1;
        const uint32 gridWidth = width;
        const uint32 gridHeight = height;
        DKVector<uint32> positionsX(gridWidth);
        DKVector<uint32> positionsZ(gridHeight);
        for (uint32 x = 0; x < gridWidth; ++x)
        {
            positionsX[x] = startX + x * step;
        }
        for (uint32 z = 0; z < gridHeight; ++z)
        {
            positionsZ[z] = startZ + z * step;
        }

        uint64 mipOffset = 0;
        for (uint32 level = 1; level < sourceLod; ++level)
        {
            const uint32 side = (_header._chunkCellCountPerSide >> level) + 1;
            mipOffset += uint64(side) * side;
        }

        FileReader reader(_path);
        if (reader.isOpen() == false)
            return false;

        DKVector<uint16> samples(size_t(gridWidth) * gridHeight);
        DKVector<DKArray<int16_t, 2>> normals(samples.size());
        const uint32 chunkColumns = columns(_header);
        const uint32 chunkRows = rows(_header);
        for (uint32 z = 0; z < gridHeight; ++z)
        {
            const uint32 chunkZ = std::min(positionsZ[z] / _header._chunkCellCountPerSide, chunkRows - 1);
            const uint32 localZ = positionsZ[z] - chunkZ * _header._chunkCellCountPerSide;
            DK_ASSERT_LOG(localZ % sourceStep == 0, "Terrain sample Z must be divisible by the source LOD sample step");
            const uint32 sampleZ = localZ / sourceStep;
            for (uint32 x = 0; x < gridWidth;)
            {
                const uint32 chunkX = std::min(positionsX[x] / _header._chunkCellCountPerSide, chunkColumns - 1);
                const uint32 localX = positionsX[x] - chunkX * _header._chunkCellCountPerSide;
                DK_ASSERT_LOG(localX % sourceStep == 0, "Terrain sample X must be divisible by the source LOD sample step");
                const uint32 sampleX = localX / sourceStep;
                const uint32 id = chunkZ * chunkColumns + chunkX;
                const uint32 rowWidth = sourceLod == 0 ? _chunks[id]._cellCountX + 1 : sourceSide;
                uint32 count = 1;
                while (x + count < gridWidth && positionsX[x + count] == positionsX[x] + count * sourceStep && std::min(positionsX[x + count] / _header._chunkCellCountPerSide, chunkColumns - 1) == chunkX)
                {
                    ++count;
                }

                const ChunkDataType type = sourceLod == 0 ? ChunkDataType::Height : ChunkDataType::HeightPyramid;
                const DKVector<ChunkDataBlockInfo>::const_iterator dataBlock = std::find_if(_dataBlocks[id].begin(), _dataBlocks[id].end(), [type](const ChunkDataBlockInfo& value)
                {
                    return value._type == type;
                });
#if defined(_DK_DEBUG_)
                if (dataBlock == _dataBlocks[id].end())
                {
                    DK_ASSERT_LOG(false, "Terrain height mip is missing");
                    return false;
                }
#endif

                const ChunkDataType normalType = sourceLod == 0 ? ChunkDataType::Normal : ChunkDataType::NormalPyramid;
                const DKVector<ChunkDataBlockInfo>::const_iterator normalBlock = std::find_if(_dataBlocks[id].begin(), _dataBlocks[id].end(), [normalType](const ChunkDataBlockInfo& value)
                {
                    return value._type == normalType;
                });
#if defined(_DK_DEBUG_)
                if (normalBlock == _dataBlocks[id].end())
                {
                    DK_ASSERT_LOG(false, "Terrain normal mip is missing");
                    return false;
                }
#endif
                DK_ASSERT_LOG(sampleZ < rowWidth && sampleX + count <= rowWidth, "Terrain grid read must fit inside the selected mip row");
                const uint64 first = mipOffset + uint64(sampleZ) * rowWidth + sampleX;
                DK_ASSERT_LOG(first + count <= dataBlock->_count && first + count <= normalBlock->_count, "Terrain grid samples must fit inside both data blocks");
                const size_t destination = size_t(z) * gridWidth + x;
                if (reader.seek(dataBlock->_offset + first * sizeof(uint16)) == false || reader.read(samples.data() + destination, size_t(count) * sizeof(uint16)) == false ||
                    reader.seek(normalBlock->_offset + first * sizeof(normals[0])) == false || reader.read(normals.data() + destination, size_t(count) * sizeof(normals[0])) == false)
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
        result._heights = DK::move(samples);
        result._normals = DK::move(normals);

        out = DK::move(result);
        return true;
    }

    bool Archive::loadVirtualTile(uint32 x, uint32 z, uint32 lod, MipTile& out) const
    {
#if defined(_DK_DEBUG_)
        if (isOpen() == false || lod > 20 || uint64(x) * _header._chunkCellCountPerSide * (1ull << lod) >= _header._width - 1 || uint64(z) * _header._chunkCellCountPerSide * (1ull << lod) >= _header._height - 1)
        {
            DK_ASSERT_LOG(false, "Virtual terrain tile is outside archive");
            return false;
        }
#endif
        const uint32 tileCellCountPerSide = _header._chunkCellCountPerSide << lod;
        if ((_header._width - 1) % tileCellCountPerSide != 0 || (_header._height - 1) % tileCellCountPerSide != 0)
        {
            DK_ASSERT_LOG(false, "Terrain archive cell counts must be divisible by the virtual tile cell count");
            return false;
        }
        TerrainOverview grid;
        const uint32 startX = (x * _header._chunkCellCountPerSide) << lod, startZ = (z * _header._chunkCellCountPerSide) << lod;
        if (loadGrid(startX, startZ, lod, _header._chunkCellCountPerSide + 1, _header._chunkCellCountPerSide + 1, grid) == false)
        {
            return false;
        }
        MipTile tile;
        tile._x = x;
        tile._z = z;
        tile._lod = lod;
        tile._sampleCountPerSide = _header._chunkCellCountPerSide + 1;
        tile._heights = DK::move(grid._heights);
        tile._normals = DK::move(grid._normals);
        out = DK::move(tile);
        return true;
    }

    bool Archive::loadOverview(uint32 lod, TerrainOverview& out) const
    {
#if defined(_DK_DEBUG_)
        if (isOpen() == false || lod > 20)
        {
            DK_ASSERT_LOG(false, "Invalid terrain overview LOD");
            return false;
        }
#endif

        const uint32 step = 1u << lod;
        if ((_header._width - 1) % step != 0 || (_header._height - 1) % step != 0)
        {
            DK_ASSERT_LOG(false, "Terrain archive cell counts must be divisible by the overview sample step");
            return false;
        }
        const uint32 width = (_header._width - 1) / step + 1;
        const uint32 height = (_header._height - 1) / step + 1;
        return loadGrid(0, 0, lod, width, height, out);
    }
}
