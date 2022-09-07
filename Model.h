#pragma once

#include "float2.h"
#include "float3.h"

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

	uint boneIndices[MAX_SKINNING_COUNT] = { static_cast<uint>(-1), static_cast<uint>(-1), static_cast<uint>(-1), static_cast<uint>(-1) };
	float weights[MAX_SKINNING_COUNT] = { 0.0f, };
};

class SubMesh
{
public:
	SubMesh() = default;
	SubMesh(DKVector<Vertex>& vertices, DKVector<uint>& indices)
		: _vertices(vertices)
		, _indices(indices)
	{}
	~SubMesh();

public:
	DKVector<Vertex> _vertices;
	DKVector<uint> _indices;
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