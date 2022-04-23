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

struct SkeletonConstantBufferStruct
{
	Matrix4x4 _skeletonMatrixBuffer[];
};

class SceneObject;

class SceneObjectManager
{
public:
	SceneObjectManager();
	~SceneObjectManager();

	// Helper Function
	// #todo- 비동기 처리 필요
	SceneObject* CreateCharacter(const char* appearancePath);

	dk_inline const std::unordered_map<uint, SceneObject>& GetCharacterSceneObjectContainer() const noexcept
	{
		return _characterSceneObjectContainer;
	}
	const AppearanceRawRef loadCharacter_LoadAppearanceFile(const char* appearancePath);

private:
	std::unordered_map<const char*, AppearanceRawRef> _appearanceRawContainers;

	std::unordered_map<uint, SceneObject> _characterSceneObjectContainer;
};