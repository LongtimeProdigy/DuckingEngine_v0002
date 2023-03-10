#include "stdafx.h"
#include "ResourceManager.h"

#include "RenderModule.h"

#include "DuckingEngine.h"

#include "Material.h"
#include "Model.h"
#include "Skeleton.h"
#include "Animation.h"
#include "ModelProperty.h"

namespace DK
{
	static const char* errorString[] =
	{
		"None",
		"Operation not permitted",
		"No such file or directory",
		"No such process",
		"Interrupted function",
		"I/O error",
		"No such device or address",
		"Argument list too long",
		"Exec format error",
		"Bad file number",
		"No spawned processes",
		"No more processes or not enough memoty or maximum nesting level reached",
		"Not enough memory",
		"Permission denied",
		"Bad address",
		"Device or resource busy",
		"File exists",
		"Cross-device link",
		"No such device",
		"Not a directory",
		"Is a directory",
		"Invalid argument",
		"Too many files open in system",
		"Too many open files",
		"Inappropriate I/O control operation",
		"File too large",
		"No space left on device",
		"Invalid seek",
		"Read-only file system",
		"Too many links",
		"Broken pipe",
		"Math argument",
		"Result too large",
		"Resource deadlock would occur",
		"Same as EDEADLK for compatibility with older Microsoft C versions",
		"Filename too long",
		"No locks available",
		"Function not supported",
		"Directory not empty",
		"Directory not empty",
		"Illegal byte sequence",
		"String was truncated"
	};

	struct ScopeFileHandle
	{
		ScopeFileHandle(const DKString& path)
		{
			errno_t err = fopen_s(&_fp, path.c_str(), "rb");
			if (err != 0)
			{
				DK_ASSERT_LOG(false, "파일을 Open하지 못했습니다.\n%s\npath: %s", errorString[err], path.c_str());
				_fp = nullptr;
			}
		}
		~ScopeFileHandle()
		{
			if(_fp != nullptr)
				fclose(_fp);
		}

		FILE* _fp = nullptr;
	};

	static bool checkResourcePathEmpty(const DKString& resourcePath)
	{
		if (resourcePath.empty() == false)
			return false;

		DK_ASSERT_LOG(false, "ResourcePath가 비어있습니다. ResourceLoading에 실패합니다.");
		return true;
	}

	DKVector<std::string> split(const std::string& str, const std::string& delim)
	{
		DKVector<std::string> tokens;
		size_t prev = 0, pos = 0;
		do
		{
			pos = str.find(delim, prev);
			if (pos == std::string::npos) pos = str.length();
			std::string token = str.substr(prev, pos - prev);
			if (!token.empty()) tokens.push_back(token);
			prev = pos + delim.length();
		} while (pos < str.length() && prev < str.length());
		return tokens;
	}
	float2 parseFloat2FromString(const std::string& str)
	{
		DKVector<std::string> tokens = split(str, " ");
		return float2(
			StringUtil::atof(tokens[0].c_str()),
			StringUtil::atof(tokens[1].c_str())
		);
	}
	float3 parseFloat3FromString(const std::string& str)
	{
		DKVector<std::string> tokens = split(str, " ");
		return float3(
			StringUtil::atof(tokens[0].c_str()),
			StringUtil::atof(tokens[1].c_str()),
			StringUtil::atof(tokens[2].c_str())
		);
	}
	Quaternion parseQuaternionFromString(const std::string& str)
	{
		DKVector<std::string> tokens = split(str, " ");
		return Quaternion(
			StringUtil::atof(tokens[0].c_str()),
			StringUtil::atof(tokens[1].c_str()),
			StringUtil::atof(tokens[2].c_str()),
			StringUtil::atof(tokens[3].c_str())
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

		uint boneCount = atoi(boneCountAttr->Value());
		if (boneCount == 0)
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

			float3 position = parseFloat3FromString(positionStr);
			float3 scale = parseFloat3FromString(scaleStr);
			Quaternion rotation = parseQuaternionFromString(rotationStr);

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
	const bool LoadAnimationInternal(const DKString& animationPath, const SkeletonRef& skeleton, AnimationRef& outAnimation)
	{
		ScopeString<DK_MAX_PATH> animationFullPath = GlobalPath::makeResourceFullPath(animationPath);

		TiXmlDocument doc;
		doc.LoadFile(animationFullPath.c_str());
		TiXmlElement* rootNode = doc.FirstChildElement("Animation");
		if (rootNode == nullptr)
		{
			DK_ASSERT_LOG(false, "Animation XML의 RootNode가 Animation이 아닙니다.");
			return false;
		}

		TiXmlAttribute* frameCountAttr = rootNode->FirstAttribute();
		if (frameCountAttr == nullptr)
		{
			DK_ASSERT_LOG(false, "Animation Data의 FrameCount Attribute를 찾을 수 없습니다.");
			return false;
		}

		uint frameCount = atoi(frameCountAttr->Value());
		outAnimation->SetFrameCount(frameCount);

		const DKVector<Bone>& bones = skeleton->getBoneArr();
		DKVector<Animation::BoneAnimation> boneAnimations;
		boneAnimations.resize(bones.size());

		uint boneCount = static_cast<uint>(bones.size());
		DKVector<bool> boneAnimationFound;
		boneAnimationFound.resize(bones.size());

		for (TiXmlNode* boneNode = rootNode->FirstChild(); boneNode != nullptr; boneNode = boneNode->NextSibling())
		{
			DKString boneName = boneNode->ToElement()->FirstAttribute()->Value();

			int boneIndex = -1;
			for (uint32 i = 0; i < boneCount; ++i)
			{
				if (boneName == bones[i]._boneName)
				{
					boneAnimationFound[i] = true;
					boneIndex = static_cast<int>(i);
					break;
				}
			}

			if (boneIndex == -1)
			{
				//DK_ASSERT_LOG(false, "Animation이 Skeleton에 없는 본을 가지고 있습니다. BoneName: %s", boneName.c_str());
				//return false;
			}
			else
			{
				Animation::BoneAnimation& boneAnimation = boneAnimations[boneIndex];
				boneAnimation._boneName = boneName;
				boneAnimation._animation.resize(frameCount);

				uint boneFrameCount = 0;
				for (TiXmlNode* transformNode = boneNode->FirstChild(); transformNode != nullptr; transformNode = transformNode->NextSibling())
				{
					TiXmlNode* positionNode = transformNode->FirstChild("Position");
					TiXmlNode* rotationNode = transformNode->FirstChild("Rotation");
					TiXmlNode* scaleNode = transformNode->FirstChild("Scale");
					if (positionNode == nullptr || rotationNode == nullptr || scaleNode == nullptr)
					{
						DK_ASSERT_LOG(false, "Transform노드를 Decompose할 수 없습니다.");
						return false;
					}

					DKString positionStr = positionNode->FirstChild()->Value();
					DKString rotationStr = rotationNode->FirstChild()->Value();
					DKString scaleStr = scaleNode->FirstChild()->Value();

					float3 position = parseFloat3FromString(positionStr);
					Quaternion rotation = parseQuaternionFromString(rotationStr);
					float3 scale = parseFloat3FromString(scaleStr);

					Transform animation(position, rotation, scale);
					animation.tofloat4x4(boneAnimation._animation[boneFrameCount]);

					++boneFrameCount;
				}

				if (frameCount != boneFrameCount)
				{
					DK_ASSERT_LOG(false, "FrameCount와 BoneFrameCount가 다릅니다. %d / %d", boneFrameCount, frameCount);
					return false;
				}
			}
		}

		uint32 boneFoundCount = static_cast<uint32>(boneAnimationFound.size());
		for (uint32 boneIndex = 0; boneIndex < boneFoundCount; ++boneIndex)
		{
			if (boneAnimationFound[boneIndex] == false)
			{
				DK_ASSERT_LOG(false, "Skeleton에 Animation에는 없는 Bone이 있습니다. BoneName: %s", bones[boneIndex]._boneName.c_str());
				return false;
			}
		}

		outAnimation->SetBoneAnimations(boneAnimations);

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
		modelPropertyDocument.LoadFile(modelPropertyFullPath.c_str());

		const TiXmlElement* modelPropertyNode = modelPropertyDocument.RootElement();

		const TiXmlElement* subMeshArrNode = modelPropertyNode->FirstChildElement("SubMeshArr");
		const uint subMeshCount = atoi(subMeshArrNode->Attribute("Count"));
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

		auto findResult = _staticMeshModelContainer.find(modelPath);
		if (findResult != _staticMeshModelContainer.end())
			return findResult->second;

		ScopeString<DK_MAX_PATH> modelFullPath = GlobalPath::makeResourceFullPath(modelPath);

		ScopeFileHandle fileHandle(modelFullPath.c_str());
		if (fileHandle._fp == nullptr)
			return nullptr;

		fseek(fileHandle._fp, 0, SEEK_END);
		uint32 blockSize = ftell(fileHandle._fp);
		fseek(fileHandle._fp, 0, SEEK_SET);

		DKVector<char> buffer(blockSize);
		fread(buffer.data(), sizeof(char), blockSize, fileHandle._fp);

		uint32 bufferOffset = 0;
		// SubMeshCount
		uint32 subMeshCount = 0;
		DK::memcpy(&subMeshCount, &buffer[bufferOffset], 4);
		bufferOffset += 4;
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
			DK::memcpy(&indexCount, &buffer[bufferOffset], 4);
			bufferOffset += 4;
			// indexBuffer
			DKVector<uint32> indexBuffer;
			indexBuffer.resize(indexCount);
			DK::memcpy(indexBuffer.data(), &buffer[bufferOffset], indexCount * 4);
			bufferOffset += indexCount * 4;

			uint32 vertexCount = 0;
			DK::memcpy(&vertexCount, &buffer[bufferOffset], 4);
			bufferOffset += 4;
			DKVector<StaticMeshModel::SubMeshType::VertexType> vertexBuffer;
			vertexBuffer.resize(vertexCount);
			DK::memcpy(vertexBuffer.data(), &buffer[bufferOffset], vertexCount * sizeof(StaticMeshModel::SubMeshType::VertexType));
			bufferOffset += vertexCount * sizeof(StaticMeshModel::SubMeshType::VertexType);

			VertexBufferViewRef vertexBufferView;
			const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexBuffer.data(), sizeof(StaticMeshModel::SubMeshType::VertexType), vertexCount, vertexBufferView);
			if (vertexBufferSuccess == false)
				return nullptr;

			IndexBufferViewRef indexBufferView;
			const bool indexBufferSuccess = renderModule.createIndexBuffer(indexBuffer.data(), indexCount, indexBufferView);
			if (indexBufferSuccess == false)
				return nullptr;

			Material* newMaterial = Material::createMaterial(materialDefinitionArr[i]);
			if (newMaterial == nullptr)
				return nullptr;

			StaticMeshModel::SubMeshType subMesh(
				DK::move(vertexBuffer), DK::move(indexBuffer), 
				DK::move(vertexBufferView), DK::move(indexBufferView), 
				newMaterial
			);
			subMeshArr.push_back(DK::move(subMesh));
		}

		DK_ASSERT_LOG(feof(fileHandle._fp) == 0, "파일이 끝에 도달하지 못했습니다. %d/%d", ftell(fileHandle._fp), blockSize);

		auto insertResult = _staticMeshModelContainer.insert(DKPair<DKString, StaticMeshModelRef>(modelPath, dk_new StaticMeshModel(DK::move(subMeshArr))));
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

		auto findResult = _skinnedMeshModelContainer.find(modelPath);
		if (findResult != _skinnedMeshModelContainer.end())
			return findResult->second;

		ScopeString<DK_MAX_PATH> modelFullPath = GlobalPath::makeResourceFullPath(modelPath);

		ScopeFileHandle handle(modelFullPath.c_str());
		if (handle._fp == nullptr)
			return nullptr;

		fseek(handle._fp, 0, SEEK_END);
		uint blockSize = ftell(handle._fp);
		fseek(handle._fp, 0, SEEK_SET);

		DKVector<char> buffer(blockSize);
		fread(buffer.data(), sizeof(char), blockSize, handle._fp);

		uint bufferOffset = 0;
		// SubMeshCount
		uint subMeshCount = 0;
		DK::memcpy(&subMeshCount, &buffer[bufferOffset], 4);
		DK_ASSERT_LOG(subMeshCount != 0, "SubMesh 개수는 0개일 수 없습니다!");
		bufferOffset += 4;

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
			DK::memcpy(&indexCount, &buffer[bufferOffset], sizeof(uint32));
			bufferOffset += 4;
			DKVector<uint32> indexBuffer;
			indexBuffer.resize(indexCount);
			DK::memcpy(indexBuffer.data(), &buffer[bufferOffset], indexCount * sizeof(uint32));
			bufferOffset += indexCount * sizeof(uint32);

			uint32 vertexCount = 0;
			DK::memcpy(&vertexCount, &buffer[bufferOffset], sizeof(uint32));
			bufferOffset += 4;
			DKVector<SkinnedMeshModel::SubMeshType::VertexType> vertexBuffer;
			vertexBuffer.resize(vertexCount);
			DK::memcpy(vertexBuffer.data(), &buffer[bufferOffset], vertexCount * sizeof(SkinnedMeshModel::SubMeshType::VertexType));
			bufferOffset += vertexCount * sizeof(SkinnedMeshModel::SubMeshType::VertexType);

#ifdef _DK_DEBUG_
			for (auto vertex : vertexBuffer)
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
			const bool vertexBufferSuccess = renderModule.createVertexBuffer(vertexBuffer.data(), sizeof(SkinnedMeshModel::SubMeshType::VertexType), vertexCount, vertexBufferView);
			if (vertexBufferSuccess == false)
				return nullptr;

			IndexBufferViewRef indexBufferView;
			const bool indexBufferSuccess = renderModule.createIndexBuffer(indexBuffer.data(), indexCount, indexBufferView);
			if (indexBufferSuccess == false)
				return nullptr;

			Material* newMaterial = Material::createMaterial(materialDefinitionArr[i]);
			if (newMaterial == nullptr)
				return nullptr;

			SkinnedMeshModel::SubMeshType subMesh(
				DK::move(vertexBuffer), DK::move(indexBuffer),
				DK::move(vertexBufferView), DK::move(indexBufferView),
				newMaterial
			);
			subMeshArr.push_back(DK::move(subMesh));
		}

		DK_ASSERT_LOG(feof(handle._fp) == 0, "파일이 끝에 도달하지 못했습니다. %d/%d", ftell(handle._fp), blockSize);

		auto insertResult = _skinnedMeshModelContainer.insert(DKPair<DKString, SkinnedMeshModelRef>(modelPath, dk_new SkinnedMeshModel(DK::move(subMeshArr))));
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
			_skinnedMeshModelContainer.erase(skeletonPath);
			return false;
		}

		outSkeleton = success.first->second;
		return true;
	}
	const bool ResourceManager::loadAnimation(const DKString& animationPath, SkeletonRef& skeleton, AnimationRef& outAnimation)
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

		AnimationRef& animation = success.first->second;

		if (LoadAnimationInternal(animationPath, skeleton, animation) == false)
		{
			DK_ASSERT_LOG(true, "Animation Loading Failed. path: %s", animationPath.c_str());
			_skinnedMeshModelContainer.erase(animationPath);
			return false;
		}

		outAnimation = success.first->second;
		return true;
	}
}