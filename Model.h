#pragma once

#include "Material.h"

namespace DK
{
	struct StaticMeshVertex
	{
	public:
		StaticMeshVertex() {}
		StaticMeshVertex(const float3& position, const float3& normal, const float2& uv)
			: _position(position), _normal(normal), _uv(uv)
		{}

	protected:
		const float3 _position = float3::Zero;
		const float3 _normal = float3::Zero;
		const float2 _uv = float2::Zero;
	};

#define MAX_SKINNING_COUNT 4
	struct SkinnedMeshVertex : public StaticMeshVertex
	{
	public:
		SkinnedMeshVertex() {}
		SkinnedMeshVertex(const float3& position, const float3& normal, const float2& uv)
			: StaticMeshVertex(position, normal, uv)
		{}

	public:
		uint32 boneIndexes[MAX_SKINNING_COUNT] = { static_cast<uint32>(-1), };
		float weights[MAX_SKINNING_COUNT] = { 0.0f, };
	};

	template<typename T>
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
		SubMesh(DKVector<SkinnedMeshVertex>& vertices, DKVector<uint32>& indices)
			: _vertices(vertices)
			, _indices(indices)
		{}

	public:
		DKVector<T> _vertices;
		DKVector<uint32> _indices;
		VertexBufferViewRef _vertexBufferView;
		IndexBufferViewRef _indexBufferView;

		Material _material;
	};

	class StaticMeshModel
	{
	public:
		using SubMeshType = SubMesh<StaticMeshVertex>;
	private:
		DK_REFLECTION_VECTOR_DECLARE(SubMeshType, _subMeshes);
	};

	class SkinnedMeshModel
	{
	public:
		using SubMeshType = SubMesh<SkinnedMeshVertex>;
	private:
		DK_REFLECTION_VECTOR_DECLARE(SubMeshType, _subMeshes);
	};
}