#pragma once

#include "float2.h"
#include "float3.h"

class IResource;
class Material;

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

	std::vector<uint> boneIndices;
	std::vector<float> weights;
};

class SubMesh
{
public:
	SubMesh() = default;
	SubMesh(std::vector<Vertex>& vertices, std::vector<uint>& indices)
		: _vertices(vertices)
		, _indices(indices)
	{}
	~SubMesh()
	{
		_vertices.clear();
		_indices.clear();
	}

public:
	std::vector<Vertex> _vertices;
	std::vector<uint> _indices;
	VertexBufferViewRef _vertexBufferView;
	IndexBufferViewRef _indexBufferView;

	Material* _material = nullptr;
};

class Model
{
public:
	Model() = default;
	Model(Model&& rvalue) = default;
	~Model()
	{
		_subMeshes.clear();
	}
	
	dk_inline void SetSubMeshes(std::vector<SubMesh>& subMeshes)
	{
		_subMeshes = std::move(subMeshes);
	}
	dk_inline const std::vector<SubMesh>& GetSubMeshes() const noexcept
	{
		return _subMeshes;
	}
	dk_inline std::vector<SubMesh>& GetSubMeshesWritable() noexcept
	{
		return _subMeshes;
	}

private:
	std::vector<SubMesh> _subMeshes;
};