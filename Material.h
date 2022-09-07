#pragma once

class SceneObject;

struct MaterialParameterDefinition;
class MaterialParameter;

struct MaterialDefinition
{
	DKString _materialName;
	DKVector<MaterialParameterDefinition> _parameters;
};

class Material
{
public:
	static Material* createMaterial(const MaterialDefinition& data);

public:
	Material(const DKString& materialname)
		: _materialName(materialname)
	{
	}

public:
	bool setModelProperty(const MaterialDefinition& materialDefinition);

private:
	const DKString _materialName;
	DKVector<Ptr<MaterialParameter>> _parameters;

	DKVector<char> _parameterBufferForCPU;
	ID3D12Resource* _parameterBufferForGPU;
};