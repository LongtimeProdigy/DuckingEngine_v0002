#include "stdafx.h"
#include "Model.h"

SubMesh::~SubMesh()
{
	_vertices.clear();
	_indices.clear();

	dk_delete _material;
}