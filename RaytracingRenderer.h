#pragma once

struct ID3D12Resource;

namespace DK
{
	class RenderModule;

    struct BLAS
    {
        RenderResourcePtr<ID3D12Resource> _blas = nullptr;
        RenderResourcePtr<ID3D12Resource> _scratch = nullptr;

        BLAS()
        {}
        BLAS(ID3D12Resource* blas, ID3D12Resource* scratch)
            : _blas(blas)
            , _scratch(scratch)
        {}
        BLAS(BLAS&& rhs)
            : _blas(DK::move(rhs._blas))
            , _scratch(DK::move(rhs._scratch))
        {
            rhs._blas = nullptr;
            rhs._scratch = nullptr;
        }

        const bool isValid() const
        {
            return _blas.get() != nullptr && _scratch.get() != nullptr;
        }
    };
    struct TLAS
    {
        DKVector<BLAS> _blases;
        RenderResourcePtr<ID3D12Resource> _tlas = nullptr;
        RenderResourcePtr<ID3D12Resource> _scratch = nullptr;

        TLAS()
        {}
        TLAS(DKVector<BLAS>&& blases, ID3D12Resource* tlas, ID3D12Resource* scratch)
            : _blases(DK::move(blases))
            , _tlas(tlas)
            , _scratch(scratch)
        {}
        TLAS(TLAS&& rhs)
            : _blases(DK::move(rhs._blases))
            , _tlas(DK::move(rhs._tlas))
            , _scratch(DK::move(rhs._scratch))
        {
            rhs._blases.clear();
            rhs._tlas = nullptr;
            rhs._scratch = nullptr;
        }

        const TLAS& operator=(TLAS&& rhs)
        {
            _blases.swap(rhs._blases);
            _tlas = rhs._tlas;
            _scratch = rhs._scratch;

            rhs._blases.clear();
            rhs._tlas = nullptr;
            rhs._scratch = nullptr;

            return *this;
        }

        const bool isValid() const
        {
            return _tlas.get() != nullptr && _scratch.get() != nullptr;
        }

        void release()
        {
            _blases.clear();
            _tlas = nullptr;
            _scratch = nullptr;
        }
    };

	class RaytracingRenderer
	{
	public:
		const bool initialize(RenderModule* renderModule, const uint32 width, const uint32 height);

		void updateRaytracingRenderer(RenderModule& renderModule);
        void dispatchRay(RenderModule& renderModule);

    private:
        DKString _renderPassName;
        DKString _pipelineName;

        // initialize
        static constexpr const UINT64 rayGenOffset = 0;
        static constexpr const UINT64 missOffset = 64;
        static constexpr const UINT64 hitGroupOffset = 128;
        uint32 _width;
        uint32 _height;
        RenderResourcePtr<ID3D12Resource> _SBT = nullptr;
        ITextureRef _outputTexture;

        // update
        bool _refresh = true;
        TLAS _tlas;
	};
}
