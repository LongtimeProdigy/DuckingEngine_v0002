#pragma once
#include "RenderModule.h"

namespace DK
{
    class RaytracingRenderer
    {
    public:
        static constexpr uint32 kRaytracingDescriptorCount = 1024;
        static constexpr uint32 BINDLESSVERTEXARRAY_SPACE = 11;
        static constexpr uint32 BINDLESSINDEXARRAY_SPACE = 12;
        static constexpr uint32 BINDLESSMATERIALARRAY_SPACE = 13;

        const bool initialize(RenderModule* renderModule, uint32 width, uint32 height);
        bool updateRaytracingRenderer(RenderModule& renderModule);
        void dispatchRay(RenderModule& renderModule, const IBufferRef& sceneConstants);

        const uint32 getAccumulationCount() const { return _sampleCount; }
        void resetAccumulation() { _sampleCount = 0; }

    private:
        uint32 _width = 0, _height = 0;
        bool _supported = false;
        ITextureRef _outputTexture;
        ITextureRef _outputAccumulateTexture;
        RaytracingAccelerationStructure _scene;
        DKVector<RaytracingGeometry> _geometries;
        DKVector<ITextureRef> _materialTextures;

        uint32 _sampleCount = 0;
    };
}
