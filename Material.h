#pragma once

class IRootSignature;
class IPipelineStateObject;
class IDescriptorHeap;

class IMaterialParameter;

class SceneObject;

class IRootParameter
{
public:
	IRootParameter();
	~IRootParameter();

public:
	uint _rootParameterIndex = -1;
	uint _constanceData;						// RootParameter Constant 일 경우에만 사용됩니다.
	IResource* _constantBuffer;					// RootParameter Constant 일 경우에만 사용됩니다.
	IDescriptorHeap* _descriptor;
};

enum class MaterialType : uint8
{
	STATICMESH, 
	SKINNEDMESH, 
	COUNT
};

class Material
{
public:
	Material(const MaterialType type)
		: _type(type)
	{}
	~Material();

public:
	void UpdateTechnique(const SceneObject* sceneObject) noexcept;
	void UploadParameters() const noexcept;

public:
	const MaterialType _type;

	IRootSignatureRef _rootSignature = nullptr;
	IPipelineStateObjectRef _pipelineStateObject = nullptr;

	std::vector<IRootParameter> _constant32BitParamters;
	std::vector<IRootParameter> _constantBufferParamters;
	std::vector<IRootParameter> _descriptorHeapParameters;

	// Paramters
	std::vector<IMaterialParameter*> _parameters;
};