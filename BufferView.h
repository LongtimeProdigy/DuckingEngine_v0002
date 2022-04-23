#pragma once

#pragma comment(lib, "d3d12.lib")
#include <d3d12.h>

template <typename T>
class BufferView
{
public:
	BufferView(const T& view)
		: _view(view)
	{}
	~BufferView() {}

	dk_inline const T& GetView() const noexcept
	{
		return _view;
	}
	dk_inline T& GetViewWritable() noexcept
	{
		return _view;
	}

private:
	T _view;
};

//class VertexBufferView
//{
//public:
//	VertexBufferView(D3D12_VERTEX_BUFFER_VIEW& view)
//		: _view(view)
//	{}
//	~VertexBufferView() {}
//
//	dk_inline const D3D12_VERTEX_BUFFER_VIEW& GetView() const noexcept
//	{
//		return _view;
//	}
//	dk_inline D3D12_VERTEX_BUFFER_VIEW& GetViewWritable() const noexcept
//	{
//		return _view;
//	}
//
//private:
//	D3D12_VERTEX_BUFFER_VIEW _view;
//};
//
//class IndexBufferView
//{
//public:
//	IndexBufferView(D3D12_INDEX_BUFFER_VIEW& view)
//		: _view(view)
//	{}
//	~IndexBufferView() {}
//
//	dk_inline const D3D12_INDEX_BUFFER_VIEW GetView() const noexcept
//	{
//
//	}
//
//private:
//	D3D12_INDEX_BUFFER_VIEW _view;
//};