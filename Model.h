#pragma once

namespace DK
{
	class Material;

	struct StaticMeshVertex
	{
		const float3 _position = float3::Zero;
		const float3 _normal = float3::Zero;
		const float2 _uv = float2::Zero;
	};

	struct SkinnedMeshVertex
	{
		const float3 _position = float3::Zero;
		const float3 _normal = float3::Zero;
		const float2 _uv = float2::Zero;
#define MAX_SKINNING_COUNT 4
		const uint32 boneIndexArr[MAX_SKINNING_COUNT] = { static_cast<uint32>(-1), static_cast<uint32>(-1),static_cast<uint32>(-1),static_cast<uint32>(-1) };
		const float weights[MAX_SKINNING_COUNT] = { 0.0f,0.0f,0.0f,0.0f };
	};

	template<typename T>
	class SubMesh
	{
	public:
		using VertexType = T;

	public:
		SubMesh(
			const DKVector<VertexType>&& vertices, const DKVector<uint32>&& indices, 
			const VertexBufferViewRef&& vertexBufferView, const IndexBufferViewRef&& indexBufferView, 
			Material* material)
			: _vertices(DK::move(vertices))
			, _indices(DK::move(indices))
			, _vertexBufferView(DK::move(vertexBufferView))
			, _indexBufferView(DK::move(indexBufferView))
			, _material(material)
		{}
		SubMesh(SubMesh&& rhs)
			: _vertices(DK::move(rhs._vertices))
			, _indices(DK::move(rhs._indices))
			, _vertexBufferView(DK::move(rhs._vertexBufferView))
			, _indexBufferView(DK::move(rhs._indexBufferView))
			, _material(rhs._material.relocate())
		{}

	public:
		const DKVector<VertexType> _vertices;
		const DKVector<uint32> _indices;
		const VertexBufferViewRef _vertexBufferView;
		const IndexBufferViewRef _indexBufferView;
		Ptr<Material> _material;
	};

	class StaticMeshModel
	{
	public:
		using SubMeshType = SubMesh<StaticMeshVertex>;

	public:
		StaticMeshModel(DKVector<SubMeshType>&& subMeshArr)
			: _subMeshArr(DK::move(subMeshArr))
		{}

	private:
		DK_REFLECTION_VECTOR_PROPERTY(SubMeshType, _subMeshArr);
	};

	class SkinnedMeshModel
	{
	public:
		using SubMeshType = SubMesh<SkinnedMeshVertex>;

	public:
		SkinnedMeshModel(DKVector<SubMeshType>&& subMeshArr)
			: _subMeshArr(DK::move(subMeshArr))
		{}

	private:
		DK_REFLECTION_VECTOR_PROPERTY(SubMeshType, _subMeshArr);
	};
}