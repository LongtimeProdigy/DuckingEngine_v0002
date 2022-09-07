#include "stdafx.h"
#include "ResourceManager.h"

#pragma region Lib
#include "FBXLoader.h"

#ifdef USE_TINYXML
#include "tinyxml.h"
#endif
#pragma endregion

#include "Model.h"
#include "Skeleton.h"
#include "Animation.h"

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
		static_cast<float>(atof(tokens[0].c_str())),
		static_cast<float>(atof(tokens[1].c_str()))
	);
}
float3 parseFloat3FromString(const std::string& str)
{
	DKVector<std::string> tokens = split(str, " ");
	return float3(
		static_cast<float>(atof(tokens[0].c_str())),
		static_cast<float>(atof(tokens[1].c_str())),
		static_cast<float>(atof(tokens[2].c_str()))
	);
}
float4 parseFloat4FromString(const std::string& str)
{
	DKVector<std::string> tokens = split(str, " ");
	return float4(
		static_cast<float>(atof(tokens[0].c_str())),
		static_cast<float>(atof(tokens[1].c_str())),
		static_cast<float>(atof(tokens[2].c_str())),
		static_cast<float>(atof(tokens[3].c_str()))
	);
}
const bool LoadMeshInternal(const char* modelPath, ModelRef& outModel)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, modelPath, "rb");
	if (err != 0)
	{
		DK_ASSERT_LOG(false, "파일을 Open하지 못했습니다. 이미 실행중인 파일이거나 관리자 문제일 수 있습니다. path: %s", modelPath);
		return false;
	}

	fseek(fp, 0, SEEK_END);
	uint blockSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char* buffer = dk_new char[blockSize];
	fread(buffer, sizeof(char), blockSize, fp);

	uint bufferOffset = 0;
	// SubMeshCount
	uint subMeshCount = 0;
	std::memcpy(&subMeshCount, &buffer[bufferOffset], 4);
	bufferOffset += 4;

	DKVector<SubMesh> submeshes;
	submeshes.reserve(subMeshCount);
	for (uint i = 0; i < subMeshCount; ++i)
	{
		// indexCount
		uint indexCount = 0;
		std::memcpy(&indexCount, &buffer[bufferOffset], 4);
		bufferOffset += 4;
		// indexBuffer
		DKVector<uint> indexBuffer;
		indexBuffer.resize(indexCount);
		std::memcpy(indexBuffer.data(), &buffer[bufferOffset], indexCount * 4);
		bufferOffset += indexCount * 4;

		// vertexCount
		uint vertexCount = 0;
		std::memcpy(&vertexCount, &buffer[bufferOffset], 4);
		bufferOffset += 4;
		// vertexBuffer
		DKVector<Vertex> vertexBuffer;
		vertexBuffer.resize(vertexCount);
		std::memcpy(vertexBuffer.data(), &buffer[bufferOffset], vertexCount * sizeof(Vertex));
		bufferOffset += vertexCount * sizeof(Vertex);

		submeshes.push_back(std::move(SubMesh(vertexBuffer, indexBuffer)));
	}

	DK_ASSERT_LOG(feof(fp) == 0, "파일이 끝에 도달하지 못했습니다. %d/%d", ftell(fp), blockSize);

	fclose(fp);
	dk_delete_array(buffer);

	outModel->MoveSubMeshesFrom(submeshes);

	return true;
}

#ifdef USE_TINYXML
const bool LoadSkeletonInternal(const char* skeletonPath, SkeletonRef& outSkeleton)
{
	TiXmlDocument doc;
	if (doc.LoadFile(skeletonPath) == false)
		return false;

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
	for (TiXmlNode* boneNode = rootNode->FirstChild(); boneNode != nullptr; boneNode = boneNode->NextSibling())
	{
		std::string boneName = boneNode->ToElement()->FirstAttribute()->Value();

		TiXmlNode* positionNode = boneNode->FirstChild("Position");
		TiXmlNode* rotationNode = boneNode->FirstChild("Rotation");
		TiXmlNode* scaleNode = boneNode->FirstChild("Scale");
		if (positionNode == nullptr || rotationNode == nullptr || scaleNode == nullptr)
		{
			DK_ASSERT_LOG(false, "Transform노드를 Decompose할 수 없습니다.");
			return false;
		}

		std::string positionStr = positionNode->FirstChild()->Value();
		std::string rotationStr = rotationNode->FirstChild()->Value();
		std::string scaleStr = scaleNode->FirstChild()->Value();

		float3 position = parseFloat3FromString(positionStr);
		//float4 rotation = parseFloat4FromString(rotationStr);
		float3 rotation = parseFloat3FromString(rotationStr);
		float3 scale = parseFloat3FromString(scaleStr);

		Bone bone(boneName, Transform(position, rotation, scale));

		bones.push_back(bone);
	}

	outSkeleton->SetBones(bones);

	return true;
}
const bool LoadAnimationInternal(const char* animationPath, const SkeletonRef& skeleton, AnimationRef& outAnimation)
{
	TiXmlDocument doc;
	doc.LoadFile(animationPath);
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

	const DKVector<Bone>& bones = skeleton->GetBones();
	DKVector<Animation::BoneAnimation> boneAnimations;
	boneAnimations.resize(bones.size());

	uint boneCount = static_cast<uint>(bones.size());
	DKVector<bool> boneAnimationFound;
	boneAnimationFound.resize(bones.size());

	for (TiXmlNode* boneNode = rootNode->FirstChild(); boneNode != nullptr; boneNode = boneNode->NextSibling())
	{
		std::string boneName = boneNode->ToElement()->FirstAttribute()->Value();

		int boneIndex = -1;
		for (uint i = 0; i < boneCount; ++i)
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

				std::string positionStr = positionNode->FirstChild()->Value();
				std::string rotationStr = rotationNode->FirstChild()->Value();
				std::string scaleStr = scaleNode->FirstChild()->Value();

				float3 position = parseFloat3FromString(positionStr);
				//float4 quaternion = parseFloat4FromString(rotationStr);
				float3 rotation = parseFloat3FromString(rotationStr);
				float3 scale = parseFloat3FromString(scaleStr);

				Transform animation(position, rotation, scale);
				animation.ToMatrix4x4(boneAnimation._animation[boneFrameCount]);

				++boneFrameCount;
			}

			if (frameCount != boneFrameCount)
			{
				DK_ASSERT_LOG(false, "FrameCount와 BoneFrameCount가 다릅니다. %d / %d", boneFrameCount, frameCount);
				return false;
			}
		}
	}

	uint boneFoundCount = static_cast<uint>(boneAnimationFound.size());
	for (uint boneIndex = 0; boneIndex < boneFoundCount; ++boneIndex)
	{
		if (boneAnimationFound[boneIndex] == false)
		{
#if defined(_DK_DEBUG_)
			DK_ASSERT_LOG(false, "Skeleton에 Animation에는 없는 Bone이 있습니다. BoneName: %s", bones[boneIndex]._boneName.c_str());
#endif
			return false;
		}

		//for (uint frameIndex = 0; frameIndex < frameCount; ++frameIndex)
		//{
		//	bones[boneIndex]._transform.ToMatrix4x4(boneAnimation._animation[frameIndex]);
		//}
	}

	outAnimation->SetBoneAnimations(boneAnimations);

	return true;
}
#endif

const bool ResourceManager::LoadMesh(const char* modelPath, ModelRef& outModel)
{
	typedef DKPair<DKHashMap<const char*, ModelRef>::iterator, bool> InsertResult;
	InsertResult success = _modelContainer.insert(DKPair<const char*, ModelRef>(modelPath, dk_new Model));
	if (success.second == false)
	{
		outModel = success.first->second;
		return true;
	}

	ModelRef& model = success.first->second;
#if 0 // load From FBX
	FBXLoader* loader = new FBXLoader;
	if (loader->LoadFBXMeshFromFile(modelPath, model) == false)
#else
	if (LoadMeshInternal(modelPath, model) == false)
#endif
	{
		DK_ASSERT_LOG(true, "Model Loading Failed. path: %s", modelPath);
		_modelContainer.erase(modelPath);
		return false;
	}

	outModel = success.first->second;
	return true;
}
const bool ResourceManager::LoadSkeleton(const char* skeletonPath, const ModelRef& model, SkeletonRef& outSkeleton)
{
	typedef DKPair<DKHashMap<const char*, SkeletonRef>::iterator, bool> InsertResult;
	InsertResult success = _skeletonContainer.insert(DKPair<const char*, SkeletonRef>(skeletonPath, dk_new Skeleton));
	if (success.second == false)
	{
		outSkeleton = success.first->second;
		return true;
	}

	FBXLoader* loader = new FBXLoader;
	SkeletonRef& skeleton = success.first->second;
#if 0
	if (loader->LoadFBXSkeletonFromFile(skeletonPath, model, *skeleton.get()) == false)
#else
	if(LoadSkeletonInternal(skeletonPath, skeleton) == false)
#endif
	{
		DK_ASSERT_LOG(true, "Skeleton Loading Failed. path: %s", skeletonPath);
		_modelContainer.erase(skeletonPath);
		return false;
	}

	outSkeleton = success.first->second;
	return true;
}
const bool ResourceManager::LoadAnimation(const char* animationPath, SkeletonRef& skeleton, AnimationRef& outAnimation)
{
	typedef DKPair<DKHashMap<const char*, AnimationRef>::iterator, bool> InsertResult;
	InsertResult success = _animationContainer.insert(DKPair<const char*, AnimationRef>(animationPath, dk_new Animation));
	if (success.second == false)
	{
		outAnimation = success.first->second;
		return true;
	}

	AnimationRef& animation = success.first->second;

#if 0
	FBXLoader* loader = new FBXLoader;
	if (loader->LoadFBXAnimationFromFile(animationPath, skeleton, *animation.get()) == false)
#else
	if(LoadAnimationInternal(animationPath, skeleton, animation) == false)
#endif
	{
		DK_ASSERT_LOG(true, "Animation Loading Failed. path: %s", animationPath);
		_modelContainer.erase(animationPath);
		return false;
	}

	outAnimation = success.first->second;
	return true;
}