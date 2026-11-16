#pragma once

#include "TerrainData.h"

namespace DK
{
    struct TerrainSection
    {
        uint32 _id = 0;
        uint32 _x = 0;
        uint32 _z = 0;
        DKString _archivePath;
    };

    struct TerrainWorld
    {
        ArchiveHeader _header;
        uint32 _sectionCellCountPerSide = 0;
        uint32 _sectionCountX = 0;
        uint32 _sectionCountZ = 0;
        uint32 _overviewLod = 0;
        DKVector<TerrainSection> _sections;

        bool open(const DKString& path);
        bool matchesSectionArchive(uint32 sectionId, const ArchiveHeader& header) const;
        DKVector<uint32> selectSections(double cameraX, double cameraZ) const;
    };

    bool splitTerrainWorld(const DKString& archivePath, const DKString& directoryPath, uint32 sectionCellCountPerSide);
}
