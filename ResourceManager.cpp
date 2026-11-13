#include "stdafx.h"
#include "ResourceManager.h"

#include "RenderModule.h"

#include "DuckingEngine.h"

#include "Material.h"
#include "Model.h"
#include "Skeleton.h"
#include "Animation.h"
#include "ModelProperty.h"
#include "FileUtil.h"

namespace DK
{
	static bool checkResourcePathEmpty(const DKString& resourcePath)
	{
		if (resourcePath.empty() == false)
			return false;

		DK_ASSERT_LOG(false, "ResourcePath가 비어있습니다. ResourceLoading에 실패합니다.");
		return true;
	}

	float2 parseFloat2FromString(const DKString& str)
	{
		const DKVector<DKString> tokens = StringUtil::split(str, " ");
		return float2(
			StringUtil::atof(tokens[0].c_str()),
			StringUtil::atof(tokens[1].c_str())
		);
	}
	const bool LoadSkeletonInternal(const DKString& skeletonPath, SkeletonRef& outSkeleton)
	{
		ScopeString<DK_MAX_PATH> skeletonFullPath = GlobalPath::makeResourceFullPath(skeletonPath);

		TiXmlDocument doc;
		if (doc.LoadFile(skeletonFullPath.c_str()) == false)
		{
			DK_ASSERT_LOG(false, "Skeleton XML 로딩에 실패!, %s", doc.ErrorDesc());
			return false;
		}

		TiXmlElement* rootNode = doc.FirstChildElement("Skeleton");
		if (rootNode == nullptr)
		{
			DK_ASSERT_LOG(false, "Skeleton XML의 RootNode가 Skeleton이 아닙니다.");
			return false;
		}

		TiXmlAttribute* boneCountAttr = rootNode->FirstAttribute();
		if (boneCountAttr == nullptr)
		{
			DK_ASSERT_LOG(false, "BoneCount Attribute를 찾을 수 없습니다.");
			return false;
		}

		uint32 boneCount = atoi(boneCountAttr->Value());
		if (boneCount == 0 || boneCount > Bone::kMaxBoneCount)
		{
			DK_ASSERT_LOG(false, "BoneCount가 0입니다.");
			return false;
		}

		DKVector<Bone> bones;
		bones.reserve(boneCount);
		for (TiXmlElement* boneNode = rootNode->FirstChildElement(); boneNode != nullptr; boneNode = boneNode->NextSiblingElement())
		{
			DKString boneName = boneNode->Attribute("Name");
			DKString parentBoneName = boneNode->Attribute("Parent") != nullptr ? boneNode->Attribute("Parent") : "";

			TiXmlElement* positionNode = boneNode->FirstChildElement("Position");
			TiXmlElement* rotationNode = boneNode->FirstChildElement("Rotation");
			TiXmlElement* scaleNode = boneNode->FirstChildElement("Scale");
			if (positionNode == nullptr || rotationNode == nullptr || scaleNode == nullptr)
			{
				DK_ASSERT_LOG(false, "Transform노드를 Decompose할 수 없습니다.");
				return false;
			}

			DKString positionStr = positionNode->GetText();
			DKString rotationStr = rotationNode->GetText();
			DKString scaleStr = scaleNode->GetText();

			float3 position = StringUtil::parseFloat3FromString(positionStr);
			float3 scale = StringUtil::parseFloat3FromString(scaleStr);
			Quaternion rotation = StringUtil::parseQuaternionFromString(rotationStr);

			uint32 currentBoneCount = static_cast<uint32>(bones.size());
			uint32 parentBoneIndex = -1;
			for (uint32 i = 0; i < currentBoneCount; ++i)
			{
				if (bones[i]._boneName == parentBoneName)
				{
					parentBoneIndex = i;
					break;
				}
			}
			DK_ASSERT_LOG(parentBoneName.empty() == true || parentBoneIndex != -1, "parentBone을 찾을 수 없습니다. 스키닝이 정상동작하지 않을 수 있습니다.");

			Bone bone(boneName, parentBoneName, parentBoneIndex, Transform(position, rotation, scale));
			bones.push_back(bone);

			float4x4 mat;
			bone._transform.tofloat4x4(mat);
		}

		outSkeleton->SetBones(bones);

		return true;
	}

	const ModelPropertyRef ResourceManager::loadModelProperty(const DKString& modelPropertyPath)
	{
		if (checkResourcePathEmpty(modelPropertyPath) == true)
			return nullptr;

		using FindResult = DKHashMap<DKString, ModelPropertyRef>::const_iterator;
		FindResult findResult = _modelPropertyContainer.find(modelPropertyPath);
		if (findResult != _modelPropertyContainer.end())
			return (*findResult).second;

		// Load Modelproperty
		ScopeString<DK_MAX_PATH> modelPropertyFullPath = GlobalPath::makeResourceFullPath(modelPropertyPath);

		TiXmlDocument modelPropertyDocument;
		if (modelPropertyDocument.LoadFile(modelPropertyFullPath.c_str()) == false)
		{
			DK_ASSERT_LOG(false, "ModelProperty의 경로가 올바르지 않습니다. Path: %s, error: %s", modelPropertyPath.c_str(), modelPropertyDocument.ErrorDesc());
			// #todo- nullptr 내보낼 수 있어야함
		}

		const TiXmlElement* modelPropertyNode = modelPropertyDocument.RootElement();

		const TiXmlElement* subMeshArrNode = modelPropertyNode->FirstChildElement("SubMeshArr");
		const uint32 subMeshCount = atoi(subMeshArrNode->Attribute("Count"));
		DKVector<MaterialDefinition> materialDefinitionArr;
		materialDefinitionArr.reserve(subMeshCount);
		for (const TiXmlElement* subMeshNode = subMeshArrNode->ToElement()->FirstChildElement(); subMeshNode != nullptr; subMeshNode = subMeshNode->NextSiblingElement())
		{
			MaterialDefinition subMeshProperty;
			subMeshProperty._materialName = subMeshNode->Attribute("Name");
			const TiXmlElement* parametersNode = subMeshNode->FirstChildElement("Parameters");
			for (const TiXmlElement* parameterNode = parametersNode->FirstChildElement(); parameterNode != nullptr; parameterNode = parameterNode->NextSiblingElement())
			{
				MaterialParameterDefinition parameterDefinition;
				parameterDefinition._name = parameterNode->Attribute("Name");
				parameterDefinition._type = convertStringToEnum(parameterNode->Attribute("Type"));
				parameterDefinition._value = parameterNode->GetText();
				subMeshProperty._parameters.push_back(DK::move(parameterDefinition));
			}

			materialDefinitionArr.push_back(DK::move(subMeshProperty));
		}

		typedef DKPair<DKHashMap<DKString, ModelPropertyRef>::iterator, bool> InsertResult;
		InsertResult success = _modelPropertyContainer.insert(DKPair<DKString, ModelPropertyRef>(modelPropertyPath, dk_new ModelProperty(materialDefinitionArr)));
		if (success.second == false)
		{
			DK_ASSERT_LOG(false, "Container Insert Failed");
			return nullptr;
		}

		return (*success.first).second;
	}

	const StaticMeshModelRef ResourceManager::loadStaticMesh(const DKString& modelPath, const ModelPropertyRef& modelProperty)
	{
		if (checkResourcePathEmpty(modelPath) == true)
			return nullptr;

		const DKHashMap<DKString, StaticMeshModelRef>::iterator findResult = _staticMeshModelContainer.find(modelPath);
		if (findResult != _staticMeshModelContainer.end())
			return findResult->second;

		ScopeString<DK_MAX_PATH> modelFullPath = GlobalPath::makeResourceFullPath(modelPath);

		FileReader reader(modelFullPath.c_str());
		if (reader.isOpen() == false)
			return nullptr;

		uint64 bytesRead = 0;
		// SubMeshCount
		uint32 subMeshCount = 0;
		if (reader.read(&subMeshCount, sizeof(subMeshCount)) == false)
			return nullptr;
		bytesRead += 4;
		if (subMeshCount == 0)
		{
			DK_ASSERT_LOG(false, "SubMesh 개수는 0개일 수 없습니다!");
			return nullptr;
		}

		const DKVector<MaterialDefinition>& materialDefinitionArr = modelProperty->get_materialDefinitionArr();
		const uint32 materialDefinitionCount = static_cast<const uint32>(materialDefinitionArr.size());
		if (subMeshCount != materialDefinitionCount)
		{
			// #todo- 개수가 다른 경우 DefaultMaterial을 생성해주어야할듯? MaterialEditor?
			DK_ASSERT_LOG(false, "SubMesh개수와 ModelProperty의 개수가 다릅니다.");
			return nullptr;
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		DKVector<StaticMeshModel::SubMeshType> subMeshArr;
		subMeshArr.reserve(subMeshCount);
		for (uint32 i = 0; i < subMeshCount; ++i)
		{
			uint32 indexCount = 0;
			if (reader.read(&indexCount, sizeof(indexCount)) == false)
				return nullptr;
			bytesRead += 4;
			// indexBuffer
			DKVector<uint32> indexBufferData;
			indexBufferData.resize(indexCount);
			if (reader.read(indexBufferData.data(), indexBufferData.size() * sizeof(uint32)) == false)
				return nullptr;
			bytesRead += indexBufferData.size() * sizeof(uint32);

			uint32 vertexCount = 0;
			if (reader.read(&vertexCount, sizeof(vertexCount)) == false)
				return nullptr;
			bytesRead += 4;
			DKVector<StaticMeshModel::SubMeshType::VertexType> vertexBufferData;
			vertexBufferData.resize(vertexCount);
			if (reader.read(vertexBufferData.data(), vertexBufferData.size() * sizeof(StaticMeshModel::SubMeshType::VertexType)) == false)
				return nullptr;
			bytesRead += vertexBufferData.size() * sizeof(StaticMeshModel::SubMeshType::VertexType);

			VertexBufferViewRef vertexBufferView;
			IBufferRef vertexBuffer = renderModule.createVertexBuffer(vertexBufferData.data(), sizeof(StaticMeshModel::SubMeshType::VertexType), vertexCount, vertexBufferView, L"StaticMesh_VertexBuffer");
			if (vertexBuffer == nullptr)
				return nullptr;

			IndexBufferViewRef indexBufferView;
			IBufferRef indexBuffer = renderModule.createIndexBuffer(indexBufferData.data(), indexCount, indexBufferView, L"StaticMesh_IndexBuffer");
			if (indexBuffer == nullptr)
				return nullptr;

			Material* newMaterial = Material::createMaterial(materialDefinitionArr[i]);
			if (newMaterial == nullptr)
				return nullptr;

			StaticMeshModel::SubMeshType subMesh(
				DK::move(vertexBufferData), DK::move(indexBufferData),
				DK::move(vertexBuffer), DK::move(indexBuffer),
				DK::move(vertexBufferView), DK::move(indexBufferView),
				newMaterial
			);
			subMeshArr.push_back(DK::move(subMesh));
		}

		if (bytesRead != reader.size())
		{
			DK_ASSERT_LOG(false, "Mesh file size does not match its contents");
			return nullptr;
		}

		const DKPair<DKHashMap<DKString, StaticMeshModelRef>::iterator, bool> insertResult = _staticMeshModelContainer.insert(DKPair<DKString, StaticMeshModelRef>(modelPath, dk_new StaticMeshModel(DK::move(subMeshArr))));
		if (insertResult.second == false)
		{
			DK_ASSERT_LOG(false, "Container Insert Failed!");
			return nullptr;
		}

		return insertResult.first->second;
	}

	const SkinnedMeshModelRef ResourceManager::loadSkinnedMesh(const DKString& modelPath, const ModelPropertyRef& modelProperty)
	{
		if (checkResourcePathEmpty(modelPath) == true)
			return nullptr;

		const DKHashMap<DKString, SkinnedMeshModelRef>::iterator findResult = _skinnedMeshModelContainer.find(modelPath);
		if (findResult != _skinnedMeshModelContainer.end())
			return findResult->second;

		ScopeString<DK_MAX_PATH> modelFullPath = GlobalPath::makeResourceFullPath(modelPath);

		FileReader reader(modelFullPath.c_str());
		if (reader.isOpen() == false)
			return nullptr;

		uint64 bytesRead = 0;
		// SubMeshCount
		uint32 subMeshCount = 0;
		if (reader.read(&subMeshCount, sizeof(subMeshCount)) == false)
			return nullptr;
		DK_ASSERT_LOG(subMeshCount != 0, "SubMesh 개수는 0개일 수 없습니다!");
		bytesRead += 4;

		const DKVector<MaterialDefinition>& materialDefinitionArr = modelProperty->get_materialDefinitionArr();
		const uint32 materialDefinitionCount = static_cast<const uint32>(materialDefinitionArr.size());
		if (subMeshCount != materialDefinitionCount)
		{
			// #todo- 개수가 다른 경우 DefaultMaterial을 생성해주어야할듯? MaterialEditor?
			DK_ASSERT_LOG(false, "SubMesh개수와 ModelProperty의 개수가 다릅니다.");
			return nullptr;
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		DKVector<SkinnedMeshModel::SubMeshType> subMeshArr;
		subMeshArr.reserve(subMeshCount);
		for (uint32 i = 0; i < subMeshCount; ++i)
		{
			uint32 indexCount = 0;
			if (reader.read(&indexCount, sizeof(indexCount)) == false)
				return nullptr;
			bytesRead += 4;
			DKVector<uint32> indexBufferData;
			indexBufferData.resize(indexCount);
			if (reader.read(indexBufferData.data(), indexBufferData.size() * sizeof(uint32)) == false)
				return nullptr;
			bytesRead += indexBufferData.size() * sizeof(uint32);

			uint32 vertexCount = 0;
			if (reader.read(&vertexCount, sizeof(vertexCount)) == false)
				return nullptr;
			bytesRead += 4;
			DKVector<SkinnedMeshModel::SubMeshType::VertexType> vertexBufferData;
			vertexBufferData.resize(vertexCount);
			if (reader.read(vertexBufferData.data(), vertexBufferData.size() * sizeof(SkinnedMeshModel::SubMeshType::VertexType)) == false)
				return nullptr;
			bytesRead += vertexBufferData.size() * sizeof(SkinnedMeshModel::SubMeshType::VertexType);

#ifdef _DK_DEBUG_
			for (SkinnedMeshVertex vertex : vertexBufferData)
			{
				float sumWeight = 0.0f;
				for (uint32 i = 0; i < MAX_SKINNING_COUNT; ++i)
				{
					sumWeight += vertex.weights[i];

					if (vertex.boneIndexArr[i] == Bone::kInvalidBoneIndex)
						DK_ASSERT_LOG(vertex.weights[i] == 0, "BoneIndex가 Invalid(Bone::kInvalidBoneIndex)한데 Weight(%f)가 존재합니다.", sumWeight);
				}
#define EPSILON 0.00001f
				DK_ASSERT_LOG(1.0f - sumWeight < EPSILON, "weight의 합이 1.0이 되지 않습니다.");
			}
#endif

			VertexBufferViewRef vertexBufferView;
			IBufferRef vertexBuffer = renderModule.createVertexBuffer(vertexBufferData.data(), sizeof(SkinnedMeshModel::SubMeshType::VertexType), vertexCount, vertexBufferView, L"SkinnedMesh_VertexBuffer");
			if (vertexBuffer == nullptr)
				return nullptr;

			IndexBufferViewRef indexBufferView;
			IBufferRef indexBuffer = renderModule.createIndexBuffer(indexBufferData.data(), indexCount, indexBufferView, L"SkinnedMesh_IndexBuffer");
			if (indexBuffer == nullptr)
				return nullptr;

			Material* newMaterial = Material::createMaterial(materialDefinitionArr[i]);
			if (newMaterial == nullptr)
				return nullptr;

			SkinnedMeshModel::SubMeshType subMesh(
				DK::move(vertexBufferData), DK::move(indexBufferData),
				DK::move(vertexBuffer), DK::move(indexBuffer),
				DK::move(vertexBufferView), DK::move(indexBufferView),
				newMaterial
			);
			subMeshArr.push_back(DK::move(subMesh));
		}

		if (bytesRead != reader.size())
		{
			DK_ASSERT_LOG(false, "Mesh file size does not match its contents");
			return nullptr;
		}

		const DKPair<DKHashMap<DKString, SkinnedMeshModelRef>::iterator, bool> insertResult = _skinnedMeshModelContainer.insert(DKPair<DKString, SkinnedMeshModelRef>(modelPath, dk_new SkinnedMeshModel(DK::move(subMeshArr))));
		if (insertResult.second == false)
		{
			DK_ASSERT_LOG(false, "Container Insert Failed!");
			return nullptr;
		}

		return insertResult.first->second;
	}
	const bool ResourceManager::loadSkeleton(const DKString& skeletonPath, const SkinnedMeshModelRef& model, SkeletonRef& outSkeleton)
	{
		if (checkResourcePathEmpty(skeletonPath) == true)
			return false;

		typedef DKPair<DKHashMap<DKString, SkeletonRef>::iterator, bool> InsertResult;
		InsertResult success = _skeletonContainer.insert(DKPair<DKString, SkeletonRef>(skeletonPath, dk_new Skeleton));
		if (success.second == false)
		{
			outSkeleton = success.first->second;
			return true;
		}

		SkeletonRef& skeleton = success.first->second;
		if (LoadSkeletonInternal(skeletonPath, skeleton) == false)
		{
			DK_ASSERT_LOG(true, "Skeleton Loading Failed. path: %s", skeletonPath.c_str());
			_skeletonContainer.erase(skeletonPath);
			return false;
		}

		outSkeleton = success.first->second;
		return true;
	}
	const bool ResourceManager::loadAnimation(const DKString& animationPath, AnimationRef& outAnimation)
	{
		if (checkResourcePathEmpty(animationPath) == true)
			return false;

		typedef DKPair<DKHashMap<DKString, AnimationRef>::iterator, bool> InsertResult;
		InsertResult success = _animationContainer.insert(DKPair<DKString, AnimationRef>(animationPath, dk_new Animation));
		if (success.second == false)
		{
			outAnimation = success.first->second;
			return true;
		}

		if (success.first->second->loadAnimatoin(animationPath) == false)
		{
			DK_ASSERT_LOG(true, "Animation Loading Failed. path: %s", animationPath.c_str());
			_animationContainer.erase(animationPath);
			return false;
		}

		outAnimation = success.first->second;
		return true;
	}
}
