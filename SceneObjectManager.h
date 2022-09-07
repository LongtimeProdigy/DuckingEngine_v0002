#pragma once

#include "Matrix4x4.h"

struct AppearanceRaw
{
	const std::string _modelPath;
	const std::string _skeletonPath;
	const std::string _animationSetPath;
	const std::string _modelPropertyPath;

	AppearanceRaw(
		const char* modelPath, const char* skeletonPath, const char* animationPath, const char* modelPropertyPath
	) : _modelPath(modelPath), _skeletonPath(skeletonPath), _animationSetPath(animationPath), _modelPropertyPath(modelPropertyPath)
	{}
};

struct SceneObjectConstantBufferStruct
{
	Matrix4x4 _worldMatrix;
};

#define MAX_BONE_COUNT 55
#define INVALID_TEXTURE_INDEX 0xffffffff
struct SkeletonConstantBufferStruct
{
	Matrix4x4 _skeletonMatrixBuffer[MAX_BONE_COUNT];
	uint32 _diffuseTexture;
};

class SceneObject;

class SceneObjectManager
{
public:
	static SceneObject* CreateStaticMesh(const char* modelPath);
	static SceneObject* CreateCharacter(const char* appearancePath);

	void Update(float deltaTime);

private:
	static const AppearanceRawRef loadCharacter_LoadAppearanceFile(const char* appearancePath);

private:
	static DKHashMap<const char*, AppearanceRawRef> _appearanceRawContainers;
	static DKHashMap<const uint, SceneObject> _characterSceneObjectContainer;
};