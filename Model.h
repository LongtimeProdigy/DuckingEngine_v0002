#pragma once

#include "Material.h"

namespace DK
{
#define MAX_SKINNING_COUNT 4

	struct Vertex
	{
	public:
		Vertex()
		{}
		Vertex(const float3& position)
			: _position(position)
		{}
		Vertex(const float3& position, const float2& uv)
			: _position(position)
			, _uv(uv)
		{}
		Vertex(const float3& position, const float3& normal, const float2& uv)
			: _position(position)
			, _normal(normal)
			, _uv(uv)
		{}

	public:
		const float3 _position = float3::Zero;
		const float3 _normal = float3::Zero;
		const float2 _uv = float2::Zero;

		uint32 boneIndexes[MAX_SKINNING_COUNT] = { static_cast<uint32>(-1), };
		float weights[MAX_SKINNING_COUNT] = { 0.0f, };
	};

	class SubMesh
	{
	public:
		SubMesh(SubMesh&& rhs)
			: _vertices(std::move(rhs._vertices))
			, _indices(std::move(rhs._indices))
			, _vertexBufferView(std::move(rhs._vertexBufferView))
			, _indexBufferView(std::move(rhs._indexBufferView))
			, _material(std::move(rhs._material))
		{}
		SubMesh(DKVector<Vertex>& vertices, DKVector<uint32>& indices)
			: _vertices(vertices)
			, _indices(indices)
		{}
		~SubMesh()
		{
			_vertices.clear();
			_indices.clear();
		}

	public:
		DKVector<Vertex> _vertices;
		DKVector<uint32> _indices;
		VertexBufferViewRef _vertexBufferView;
		IndexBufferViewRef _indexBufferView;

		Material _material;
	};

	class Model
	{
	public:
		~Model()
		{
			_subMeshes.clear();
		}

		dk_inline void MoveSubMeshesFrom(DKVector<SubMesh>& subMeshes)
		{
			_subMeshes = std::move(subMeshes);
		}
		dk_inline const DKVector<SubMesh>& GetSubMeshes() const noexcept
		{
			return _subMeshes;
		}
		dk_inline DKVector<SubMesh>& GetSubMeshesWritable() noexcept
		{
			return _subMeshes;
		}

	private:
		DKVector<SubMesh> _subMeshes;
	};
}