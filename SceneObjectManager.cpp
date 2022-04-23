#include "stdafx.h"
#include "SceneObjectManager.h"

#include "Matrix4x4.h"

#include "DuckingEngine.h"
#include "RenderModule.h"
#include "IResource.h"

#include "SceneObject.h"
#include "SkinnedMeshComponent.h"
#include "Skeleton.h"

SceneObjectManager::SceneObjectManager()
{
}

SceneObjectManager::~SceneObjectManager()
{
	_characterSceneObjectContainer.clear();
}

const AppearanceRawRef SceneObjectManager::loadCharacter_LoadAppearanceFile(const char* appearancePath)
{
	using FindResult = std::unordered_map<const char*, AppearanceRawRef>::iterator;
	FindResult findResult = _appearanceRawContainers.find(appearancePath);
	if (findResult != _appearanceRawContainers.end())
	{
		return findResult->second;
	}

	TiXmlDocument ReadDoc;
	ReadDoc.LoadFile(appearancePath);

	TiXmlElement* ReadRoot = ReadDoc.FirstChildElement("Appearance");
	TiXmlElement* modelNode = ReadRoot->FirstChildElement("Model");
	const char* modelPath = modelNode->GetText();
	TiXmlElement* skeletonNode = ReadRoot->FirstChildElement("Skeleton");
	const char* skeletonPath = skeletonNode->GetText();
	TiXmlElement* animationSetNode = ReadRoot->FirstChildElement("AnimationSet");
	const char* animationPath = animationSetNode->GetText();
	TiXmlElement* modelPropertyNode = ReadRoot->FirstChildElement("ModelProperty");
	const char* modelPropertyPath = modelPropertyNode->GetText();

	auto insertResult = _appearanceRawContainers.insert(
		std::pair<const char*, AppearanceRawRef>(
			appearancePath, dk_new AppearanceRaw(modelPath, skeletonPath, animationPath, modelPropertyPath)
			)
	);

	return insertResult.first->second;
}

SceneObject* SceneObjectManager::CreateCharacter(const char* appearancePath)
{
	// load appearance
	const AppearanceRawRef appearanceRaw = loadCharacter_LoadAppearanceFile(appearancePath);

	// #todo- SceneObject�� GUID�� ������ �ְ� �ش� GUID�� Key�� ���� �� ���ƺ��̱���..
	// ���� ���������� 4byte�� �Ѵ� ĳ���Ͱ� �����Ǹ� ������ ����
	// ĳ���Ͱ� �����Ǵ� ��쿡�� ������ key�� �� �� ���� ������ ����
	const uint key = static_cast<uint>(_characterSceneObjectContainer.size());
	auto success =_characterSceneObjectContainer.insert(std::pair<const uint, SceneObject>(key, SceneObject()));
	if (success.second == false)
	{
		return &success.first->second;
	}

	SceneObject& sceneObject = success.first->second;

	// Transform GPU resource
	const RenderModule* renderModule = DuckingEngine::_duckingEngine->GetRenderModule();
	SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
	sceneObject._worldTransform.ToMatrix4x4(sceneObjectConstantBufferData._worldMatrix);
	sceneObject._sceneObjectConstantBuffer = renderModule->CreateUploadBuffer(&sceneObjectConstantBufferData, sizeof(sceneObjectConstantBufferData));
	if (sceneObject._sceneObjectConstantBuffer == nullptr)
	{
		DK_ASSERT_LOG(false, "�߸��� MaterialType�� ���Ͽ� UpdateTechnique�� �õ����Դϴ�. �ݵ�� ����ٶ��ϴ�.");
		_characterSceneObjectContainer.erase(key);
		return nullptr;
	}

	// Skeleton GPU resource
	SkeletonConstantBufferStruct skeletonConstantBuffer;
	sceneObject._skeletonConstantBuffer = renderModule->CreateUploadBuffer(&skeletonConstantBuffer, sizeof(skeletonConstantBuffer));
	if (sceneObject._skeletonConstantBuffer == nullptr)
	{
		DK_ASSERT_LOG(false, "�߸��� MaterialType�� ���Ͽ� UpdateTechnique�� �õ����Դϴ�. �ݵ�� ����ٶ��ϴ�.");
		_characterSceneObjectContainer.erase(key);
		return nullptr;
	}

	SkinnedMeshComponent* mainSkinnedMeshComponent = dk_new SkinnedMeshComponent;
	if (mainSkinnedMeshComponent->LoadResource(
		appearanceRaw->_modelPath.c_str(), 
		appearanceRaw->_skeletonPath.c_str(), 
		appearanceRaw->_animationSetPath.c_str(), 
		appearanceRaw->_modelPropertyPath.c_str(), &sceneObject) == false
		)
	{
		DK_ASSERT_LOG(false, "SkinnedMeshComponent LoadResource - Failed");
		_characterSceneObjectContainer.erase(key);
		return nullptr;
	}
	sceneObject.AddComponent(mainSkinnedMeshComponent);

	const std::vector<Bone>& bones = mainSkinnedMeshComponent->GetSkeleton()->GetBones();
	uint boneCount = bones.size();
	std::vector<Matrix4x4> boneMatrices;
	boneMatrices.resize(boneCount);
	for (uint i = 0; i < boneCount; ++i)
	{
		bones[i]._transform.ToMatrix4x4(boneMatrices[i]);
	}
	uint8* skeletonConstantBufferAddress = sceneObject._skeletonConstantBuffer->Map();
	memcpy(skeletonConstantBufferAddress, &boneMatrices[0], sizeof(Matrix4x4) * boneMatrices.size());
	sceneObject._skeletonConstantBuffer->UnMap();

	return &sceneObject;
}