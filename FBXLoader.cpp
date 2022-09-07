#include "stdafx.h"
#include "FBXLoader.h"

#pragma region LIB
#include "fbxsdk.h"

#ifdef USE_ASSIMP
#include "assimp/Importer.hpp"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#endif
#pragma endregion

#ifdef USE_TINYXML
#include "tinyxml.h"
#endif

#include "float2.h"
#include "float3.h"
#include "Transform.h"
#include "Matrix4x4.h"

#include "DuckingEngine.h"

#include "Model.h"
#include "Skeleton.h"
#include "Animation.h"

float3 ToEulerAnglesInternal(const float4 quaternion)
{
	float3 angles;

	const float x = quaternion.z;
	const float y = quaternion.x;
	const float z = quaternion.y;
	const float w = quaternion.w;

	// roll (x-axis rotation)
	double sinr_cosp = 2 * (w * x + y * z);
	double cosr_cosp = 1 - 2 * (x * x + y * y);
	angles.z = static_cast<float>(std::atan2(sinr_cosp, cosr_cosp));

	static float math_pi = 3.141592f;

	// pitch (y-axis rotation)
	double sinp = 2 * (w * y - z * x);
	if (std::abs(sinp) >= 1)
		angles.x = static_cast<float>(std::copysign(math_pi / 2, sinp)); // use 90 degrees if out of range
	else
		angles.x = static_cast<float>(std::asin(sinp));

	// yaw (z-axis rotation)
	double siny_cosp = 2 * (w * z + x * y);
	double cosy_cosp = 1 - 2 * (y * y + z * z);
	angles.y = static_cast<float>(std::atan2(siny_cosp, cosy_cosp));
	return angles;
}

float3 ToEulerAngles(const float4 quaternion)
{
	return ToEulerAnglesInternal(quaternion);
}

#ifdef USE_ASSIMP
float3 ToEulerAngles(const aiQuaternion& q) {
	const auto x = q.z;
	const auto y = q.x;
	const auto z = q.y;
	const auto w = q.w;
	return ToEulerAnglesInternal(float4(x, y, z, w));
}

void InitMeshFromAssimpToSubMesh(_IN_ const aiMesh* assimpMesh, _OUT_ DKVector<Vertex>& vertices, _OUT_ DKVector<uint>& indices)
{
	const uint numVertices = assimpMesh->mNumVertices;
	vertices.reserve(numVertices);

	const uint numFaces = assimpMesh->mNumFaces;
	indices.reserve(numFaces * 3);

	for (uint i = 0; i < numVertices; ++i)
	{
		// ���� duckingengine_v0002�� �޼���ǥ�踦 ������Դϴ�.
		// FBX ... Max�� ������ ��ǥ���̹Ƿ� ��ȯ�մϴ�.
		float3 position(
			assimpMesh->mVertices[i].x
			, assimpMesh->mVertices[i].y
			, assimpMesh->mVertices[i].z
		);

		float3 normal(
			assimpMesh->mNormals[i].x
			, assimpMesh->mNormals[i].y
			, assimpMesh->mNormals[i].z
		);

		float2 uv;
		if (assimpMesh->HasTextureCoords(0))
		{
			uv = float2(
				assimpMesh->mTextureCoords[0][i].x
				, assimpMesh->mTextureCoords[0][i].y
			);
		}
		else
		{
			uv = float2(0, 0);
		}

		const Vertex vertex(position, normal, uv);
		vertices.push_back(vertex);
	}

	for (uint i = 0; i < numFaces; ++i)
	{
		const aiFace& face = assimpMesh->mFaces[i];
		indices.push_back(face.mIndices[0]);
		indices.push_back(face.mIndices[1]);
		indices.push_back(face.mIndices[2]);
	}
}
Bone CreateBoneByAIBone(const aiBone* foundBone)
{
	DKString boneName = foundBone->mName.C_Str();
	aiVector3D aiLocation, aiRotation, aiScale;
	foundBone->mOffsetMatrix.Decompose(aiScale, aiRotation, aiLocation);
	float3 location(aiLocation.x, aiLocation.y, aiLocation.z);
	float3 rotation(aiRotation.x, aiRotation.y, aiRotation.z);
	float3 scale(aiScale.x, aiScale.y, aiScale.z);
	Transform boneTransform(location, rotation, scale);

	return Bone(boneName, boneTransform);
}
const bool LoadFBXMeshFromFileByAssimp(_IN_ const char* path, _OUT_ Model& outModel)
{
	const aiScene* scene = aiImportFile(path,
		aiProcess_JoinIdenticalVertices			// ������ ������ ����, �ε��� ����ȭ
		| aiProcess_ValidateDataStructure		// �δ��� ����� ����
		| aiProcess_ImproveCacheLocality		// ��� ������ ĳ����ġ�� ����
		| aiProcess_RemoveRedundantMaterials	// �ߺ��� ���͸��� ����
		| aiProcess_GenUVCoords					// ����, ������, ���� �� ��� ������ ������ UV�� ��ȯ
		| aiProcess_TransformUVCoords			// UV ��ȯ ó���� (�����ϸ�, ��ȯ ...)
		| aiProcess_FindInstances				// �ν��Ͻ��� �Ž��� �˻��Ͽ� �ϳ��� �����Ϳ� ���� ������ ����
		| aiProcess_LimitBoneWeights			// ������ ���� ����ġ�� �ִ� 4���� ����
		| aiProcess_OptimizeMeshes				// ������ ��� ���� �Ž��� ����
		| aiProcess_GenSmoothNormals			// �ε巯�� �븻����(��������) ����
		| aiProcess_SplitLargeMeshes			// �Ŵ��� �ϳ��� �Ž��� �����Ž���� ����(����)
		| aiProcess_Triangulate					// 3�� �̻��� �𼭸��� ���� �ٰ��� ���� �ﰢ������ ����(����)
		| aiProcess_ConvertToLeftHanded			// D3D�� �޼���ǥ��� ��ȯ
		| aiProcess_SortByPType					// ����Ÿ���� ������Ƽ��� ������ '������' �Ž��� ����
	);

	if (scene == nullptr) return false;

	uint numSubMesh = scene->mNumMeshes;		// ����Ž� ����

	RenderModule& rm = DuckingEngine::getInstance().GetRenderModuleWritable();
	DKVector<SubMesh> subMeshes;
	subMeshes.resize(numSubMesh);
	for (uint i = 0; i < numSubMesh; ++i)
	{
		const aiMesh* mesh = scene->mMeshes[i];
		InitMeshFromAssimpToSubMesh(mesh, subMeshes[i]._vertices, subMeshes[i]._indices);
	}
	outModel.MoveSubMeshesFrom(subMeshes);

	return true;
}

Bone ConvertAIBoneToBone(const aiBone* sourceBone)
{
	DKString boneName(sourceBone->mName.C_Str());

	aiVector3D aiPosition, aiRotation, aiScale;
	sourceBone->mOffsetMatrix.Decompose(aiScale, aiRotation, aiPosition);
	float3 position(aiPosition.x, aiPosition.y, aiPosition.z);
	float3 rotation(aiRotation.x, aiRotation.y, aiRotation.z);
	float3 scale(aiScale.x, aiScale.y, aiScale.z);

	Transform transform(position * -1, rotation * -1, scale);
	return Bone(boneName, transform);
}
Bone ConvertNodeToBone(const aiNode* sourceNode)
{
	DKString boneName(sourceNode->mName.C_Str());

	aiVector3D aiPosition, aiRotation, aiScale;
	sourceNode->mTransformation.Decompose(aiScale, aiRotation, aiPosition);

	float3 position(aiPosition.x, aiPosition.y, aiPosition.z);
	float3 rotation(aiRotation.x, aiRotation.y, aiRotation.z);
	float3 scale(aiScale.x, aiScale.y, aiScale.z);

	Transform transform(position, rotation, scale);
	return Bone(boneName, transform);
}
void BuildBoneVectorAndIndexHashMapChild(
	const DKHashMap<DKString, const aiBone*>& boneHash,
	const aiNode* node, 
	DKVector<Bone>& boneVector,
	DKHashMap<DKString, uint>& boneIndexHash
)
{
	uint childCount = node->mNumChildren;
	for (uint i = 0; i < childCount; ++i)
	{
		const aiString boneName = node->mChildren[i]->mName;
		const DKHashMap<DKString, const aiBone*>::const_iterator boneIter = boneHash.find(boneName.C_Str());
		if (boneIter == boneHash.end())
		{
			//DK_ASSERT_LOG(false, "%s�� �ش��ϴ� ���� Mesh���� ã�� ���߽��ϴ�.", boneName.C_Str());
			continue;
		}
		DKHashMap<DKString, uint>::iterator indexIter = boneIndexHash.find(boneName.C_Str());
		if (indexIter != boneIndexHash.end())
		{
			DK_ASSERT_LOG(false, "�ߺ��Ǵ� ���� ����� �� �����ϴ�. BoneName: %s", boneName.C_Str());
			return;
		}

		uint boneIndex = static_cast<uint>(boneVector.size());
		//boneVector.push_back(ConvertAIBoneToBone(boneIter->second));
		boneVector.push_back(ConvertNodeToBone(node->mChildren[i]));
		boneIndexHash.insert(std::make_pair(boneName.C_Str(), boneIndex));
	}

	for (uint i = 0; i < childCount; ++i)
	{
		BuildBoneVectorAndIndexHashMapChild(boneHash, node->mChildren[i], boneVector, boneIndexHash);
	}
}
void BuildBoneChildIndex(
	const aiNode* node, 
	const DKHashMap<DKString, uint>& boneIndexHash,
	DKVector<Bone>& boneVector
)
{
	DKHashMap<DKString, uint>::const_iterator parentBoneIter = boneIndexHash.find(node->mName.C_Str());
	if (parentBoneIter == boneIndexHash.end())
	{
		//DK_ASSERT_LOG(false, "�ùٸ��� ���� ���� �����մϴ�. %s", node->mName.C_Str());
		return;
	}

	uint boneIndex = parentBoneIter->second;
	Bone& bone = boneVector[boneIndex];

	uint childCount = node->mNumChildren;
	for (uint i = 0; i < childCount; ++i)
	{
		DKHashMap<DKString, uint>::const_iterator childBoneIter = boneIndexHash.find(node->mChildren[i]->mName.C_Str());
		if (childBoneIter == boneIndexHash.end())
		{
			//DK_ASSERT_LOG(false, "�ùٸ��� ���� ���� �����մϴ�. %s", node->mChildren[i]->mName.C_Str());
			continue;
		}

		bone._childs.push_back(childBoneIter->second);
	}

	for (uint i = 0; i < childCount; ++i)
	{
		BuildBoneChildIndex(node->mChildren[i], boneIndexHash, boneVector);
	}
}
void BuildMeshWeights(
	const ModelRef& model,
	const DKHashMap<DKString, const aiBone*>& boneHash,
	const DKHashMap<DKString, uint>& boneIndexHash
)
{
	DKVector<DKVector<uint>> weightIndices;
	weightIndices.resize(model->GetSubMeshes().size());
	for (uint i = 0; i < weightIndices.size(); ++i)
	{
		weightIndices[i].resize(model->GetSubMeshes()[i]._vertices.size());
	}

	for (auto iter = boneHash.begin(); iter != boneHash.end(); ++iter)
	{
		const aiBone* bone = iter->second;

		if (boneIndexHash.find(bone->mName.C_Str()) == boneIndexHash.end())
		{
			DK_ASSERT_LOG(false, "�������� �ʴ� ���� �����մϴ�. %s", bone->mName.C_Str());
			continue;
		}

		uint weightCount = bone->mNumWeights;
		for (uint i = 0; i < weightCount; ++i)
		{
			uint vertIndex = bone->mWeights[i].mVertexId;
			float weight = bone->mWeights[i].mWeight;

			DKVector<SubMesh>& submeshes = model->GetSubMeshesWritable();
			for (uint submeshIndex = 0; submeshIndex < submeshes.size(); ++submeshIndex)
			{
				if (vertIndex >= submeshes[submeshIndex]._vertices.size())
				{
					continue;
				}

				Vertex& vert = submeshes[submeshIndex]._vertices[vertIndex];
				vert.boneIndices[weightIndices[submeshIndex][vertIndex]] = boneIndexHash.find(bone->mName.C_Str())->second;
				vert.weights[weightIndices[submeshIndex][vertIndex]] = weight;

				weightIndices[submeshIndex][vertIndex] += 1;

				DK_ASSERT_LOG(weightIndices[submeshIndex][vertIndex] <= MAX_SKINNING_COUNT, "Skinning weight�� 4���� ���� �� �����ϴ�.");
			}
		}
	}

#if defined(_DK_DEBUG_)
	DKVector<SubMesh>& submeshes = model->GetSubMeshesWritable();
	for (uint i = 0; i < submeshes.size(); ++i)
	{
		for (uint j = 0; j < submeshes[i]._vertices.size(); ++j)
		{
			Vertex& vert = submeshes[i]._vertices[j];

			float weight = 0.0f;
			for (uint k = 0; k < MAX_SKINNING_COUNT; ++k)
			{
				weight += vert.weights[k];
			}

			static float epsilon = 0.00001f;
			//DK_ASSERT_LOG(abs(weight - 1.0f) <= epsilon, "Vertex�� Weight�� 1.0�� �ƴմϴ�.");
		}
	}
#endif
}
const bool LoadFBXSkeletonFromFileByAssimp(_IN_ const char* path, _IN_ const ModelRef& model, _OUT_ Skeleton& outSkeleton)
{
	const aiScene* scene = aiImportFile(path,
		aiProcess_JoinIdenticalVertices			// ������ ������ ����, �ε��� ����ȭ
		| aiProcess_ValidateDataStructure		// �δ��� ����� ����
		| aiProcess_ImproveCacheLocality		// ��� ������ ĳ����ġ�� ����
		| aiProcess_RemoveRedundantMaterials	// �ߺ��� ���͸��� ����
		| aiProcess_GenUVCoords					// ����, ������, ���� �� ��� ������ ������ UV�� ��ȯ
		| aiProcess_TransformUVCoords			// UV ��ȯ ó���� (�����ϸ�, ��ȯ ...)
		| aiProcess_FindInstances				// �ν��Ͻ��� �Ž��� �˻��Ͽ� �ϳ��� �����Ϳ� ���� ������ ����
		| aiProcess_LimitBoneWeights			// ������ ���� ����ġ�� �ִ� 4���� ����
		| aiProcess_OptimizeMeshes				// ������ ��� ���� �Ž��� ����
		| aiProcess_GenSmoothNormals			// �ε巯�� �븻����(��������) ����
		| aiProcess_SplitLargeMeshes			// �Ŵ��� �ϳ��� �Ž��� �����Ž���� ����(����)
		| aiProcess_Triangulate					// 3�� �̻��� �𼭸��� ���� �ٰ��� ���� �ﰢ������ ����(����)
		| aiProcess_ConvertToLeftHanded			// D3D�� �޼���ǥ��� ��ȯ
		| aiProcess_SortByPType					// ����Ÿ���� ������Ƽ��� ������ '������' �Ž��� ����
	);

	if (scene == nullptr)
	{
		DK_ASSERT_LOG(false, "scene�� �ʱ�ȭ���� �ʾҽ��ϴ�.");
		return false;
	}

	// ��� Submesh�� �ִ� Bone���� Name�� Key�� ������ hashmap�� ����ϴ�. (�Ʒ��� �� �������� ���� ����)
	DKHashMap<DKString, const aiBone* > boneHash;
	for (uint i = 0; i < scene->mNumMeshes; ++i)
	{
		const aiMesh* mesh = scene->mMeshes[i];
		if (mesh == nullptr || mesh->HasBones() == false)
		{
			continue;
		}

		const uint boneCount = mesh->mNumBones;

		// ��� �� hashing
		boneHash.reserve(boneCount);
		for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			aiBone* aiBone = mesh->mBones[boneIndex];
			const aiString& boneName = aiBone->mName;

			auto foundResult = boneHash.find(boneName.C_Str());
			if (foundResult != boneHash.end())
			{
				continue;
			}

			boneHash.insert(std::make_pair(boneName.C_Str(), aiBone));
		}
	}

#if 1
	// ��� �� vector�� build
	const uint boneCount = static_cast<const uint>(boneHash.size());
	const aiNode* rootNode = scene->mRootNode;
	DKVector<Bone> bones;
	DKHashMap<DKString, uint> boneIndexHash;
	BuildBoneVectorAndIndexHashMapChild(boneHash, rootNode, bones, boneIndexHash);
	DK_ASSERT_LOG(boneCount == bones.size(), "Bone ������ �� �̷������ �ʾҽ��ϴ�.");
	DK_ASSERT_LOG(boneCount == boneIndexHash.size(), "Bone ������ �� �̷������ �ʾҽ��ϴ�.");

	// rootNode�� ���� �ƴϱ� ������ rootNode�� Child���� �����ؾ���
	uint childCount = rootNode->mNumChildren;
	for (uint i = 0; i < childCount; ++i)
	{
		BuildBoneChildIndex(rootNode->mChildren[i], boneIndexHash, bones);
	}

	BuildMeshWeights(model, boneHash, boneIndexHash);
#else
		uint max = 0;
		for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			const aiBone* aiBone = mesh->mBones[boneIndex];

			DKString boneName = aiBone->mName.C_Str();
			aiVector3D translation, rotation, scale;
			aiBone->mOffsetMatrix.Decompose(scale, rotation, translation);
			float3 translationF(translation.x, translation.y, translation.z);
			float3 rotationF(rotation.x, rotation.y, rotation.z);
			float3 scaleF(scale.x, scale.y, scale.z);
			Transform boneTransform(translationF, rotationF, scaleF);

			Bone newBone;
#if defined(_DK_DEBUG_)
			newBone._boneName = boneName;
#endif
			newBone._transform = boneTransform;
			bones.push_back(newBone);

			const uint weightCount = aiBone->mNumWeights;
			for (uint weightIndex = 0; weightIndex < weightCount; ++weightIndex)
			{
				const aiVertexWeight& aiWeight = aiBone->mWeights[weightIndex];
				const uint vertexIndex = aiWeight.mVertexId;
				if (vertexIndex >= model->GetSubMeshes()[0]._vertices.size())
				{
					DK_ASSERT_LOG(false, "Skinning VertexIndex�� Vertex Count�� �Ѿ�ϴ�. %d/%d", vertexIndex, model->GetSubMeshes()[0]._vertices.size());
					continue;
				}
				const float weight = aiWeight.mWeight;

				DKVector<SubMesh>& submeshes = model->GetSubMeshesWritable();
				Vertex& vertex = submeshes[0]._vertices[vertexIndex];
				vertex.boneIndices.push_back(boneIndex);
				vertex.weights.push_back(weight);
				if (vertex.boneIndices.size() > 4)
				{
					DK_ASSERT_LOG(false, "Skinning �Ѱ谳���� �Ѿ���ϴ�. %d/%d", vertex.boneIndices.size(), 4);
					continue;
				}
			}
}
#endif

	outSkeleton.SetBones(bones);

	return true;
}

const bool LoadFBXAnimationFromFileByAssimp(_IN_ const char* path, const SkeletonRef& skeleton, _OUT_ Animation& outAnimation)
{
	const aiScene* scene = aiImportFile(path,
		aiProcess_JoinIdenticalVertices			// ������ ������ ����, �ε��� ����ȭ
		| aiProcess_ValidateDataStructure		// �δ��� ����� ����
		| aiProcess_ImproveCacheLocality		// ��� ������ ĳ����ġ�� ����
		| aiProcess_RemoveRedundantMaterials	// �ߺ��� ���͸��� ����
		| aiProcess_GenUVCoords					// ����, ������, ���� �� ��� ������ ������ UV�� ��ȯ
		| aiProcess_TransformUVCoords			// UV ��ȯ ó���� (�����ϸ�, ��ȯ ...)
		| aiProcess_FindInstances				// �ν��Ͻ��� �Ž��� �˻��Ͽ� �ϳ��� �����Ϳ� ���� ������ ����
		| aiProcess_LimitBoneWeights			// ������ ���� ����ġ�� �ִ� 4���� ����
		| aiProcess_OptimizeMeshes				// ������ ��� ���� �Ž��� ����
		| aiProcess_GenSmoothNormals			// �ε巯�� �븻����(��������) ����
		| aiProcess_SplitLargeMeshes			// �Ŵ��� �ϳ��� �Ž��� �����Ž���� ����(����)
		| aiProcess_Triangulate					// 3�� �̻��� �𼭸��� ���� �ٰ��� ���� �ﰢ������ ����(����)
		| aiProcess_ConvertToLeftHanded			// D3D�� �޼���ǥ��� ��ȯ
		| aiProcess_SortByPType					// ����Ÿ���� ������Ƽ��� ������ '������' �Ž��� ����
	);

	if (scene == nullptr)
	{
		DK_ASSERT_LOG(false, "scene�� �ʱ�ȭ���� �ʾҽ��ϴ�.");
		return false;
	}

	// �ִϸ��̼� ���ϴ� �ִϸ��̼��� �ϳ��� �ִٴ� �����Դϴ�.
	aiAnimation* animation = scene->mAnimations[0];
	DKString animationName = animation->mName.C_Str();
	float duration = static_cast<float>(animation->mDuration);
	float tick = static_cast<float>(animation->mTicksPerSecond);
	static float epsilon = 0.00001f;
	uint animationFrameCount = static_cast<uint>(duration - epsilon) + 1;

	aiMatrix4x4 aiGlobalMatrix = scene->mRootNode->mTransformation;
	Matrix4x4 globalMatrix(
		aiGlobalMatrix.a1, aiGlobalMatrix.a2, aiGlobalMatrix.a3, aiGlobalMatrix.a4, 
		aiGlobalMatrix.b1, aiGlobalMatrix.b2, aiGlobalMatrix.b3, aiGlobalMatrix.b4, 
		aiGlobalMatrix.c1, aiGlobalMatrix.c2, aiGlobalMatrix.c3, aiGlobalMatrix.c4, 
		aiGlobalMatrix.d1, aiGlobalMatrix.d2, aiGlobalMatrix.d3, aiGlobalMatrix.d4
	);
	Matrix4x4 fbxConvertingMatrix(
		1, 0, 0, 0, 
		0, 0, 1, 0, 
		0, 1, 0, 0, 
		0, 0, 0, 1
	);

	std::unordered_set<DKString> boneNames;
	const DKVector<Bone>& bones = skeleton->GetBones();
	DKVector<Animation::BoneAnimation> boneAnimation;
	boneAnimation.resize(bones.size());
	for (uint i = 0; i < bones.size(); ++i)
	{
		const DKString boneName = bones[i]._boneName;
		std::unordered_set<DKString>::iterator iter = boneNames.find(boneName.c_str());
		if (iter != boneNames.end())
		{
			DK_ASSERT_LOG(false, "%s ���� ���� �ߺ� �ִϸ��̼��� �߰ߵǾ����ϴ�. �߸��� �ִϸ��̼� �����Դϴ�.", boneName.c_str());
			continue;
		}

		boneNames.insert(boneName);

		aiNodeAnim* foundBoneAnimationNode = nullptr;
		uint channelCount = animation->mNumChannels;
		for (uint j = 0; j < channelCount; ++j)
		{
			const aiString& animationBoneName = animation->mChannels[j]->mNodeName;
			if (strcmp(animationBoneName.C_Str(), boneName.c_str()) != 0)
			{
				continue;
			}

			foundBoneAnimationNode = animation->mChannels[j];

			break;
		}

		Animation::BoneAnimation& newBoneAnimation = boneAnimation[i];
		newBoneAnimation._boneName = boneName.c_str();

		DKVector<Matrix4x4> newAnimation;
		newAnimation.resize(animationFrameCount);

		if (foundBoneAnimationNode == nullptr)
		{
			DK_ASSERT_LOG(foundBoneAnimationNode != nullptr, "Bone[%s] Animation�� ã�� ���߽��ϴ�. �ִϸ��̼��� ����� �۵����� �ʽ��ϴ�.", boneName.c_str());
			for (uint j = 0; j < animationFrameCount; ++j)
			{
				Matrix4x4 temp;
				bones[i]._transform.ToMatrix4x4(temp);
				newAnimation[j] = temp;
			}
		}
		else
		{
			uint positionCount = foundBoneAnimationNode->mNumPositionKeys;
			uint rotationCount = foundBoneAnimationNode->mNumRotationKeys;
			uint scaleCount = foundBoneAnimationNode->mNumScalingKeys;
			DK_ASSERT_LOG((positionCount == rotationCount) && (rotationCount == scaleCount), "�ִϸ��̼� �������� �ٸ��ϴ�. %d, %d, %d", positionCount, rotationCount, scaleCount);

			uint positionIndex = 0;
			uint rotationIndex = 0;
			uint scaleIndex = 0;
			float3 prevPosition, prevRotation, prevScale;
			uint maxCount = positionCount < rotationCount ? (rotationCount < scaleCount ? scaleCount : rotationCount) : (positionCount < scaleCount ? scaleCount : positionCount);
			for (uint j = 0; j < animationFrameCount; ++j)
			{
				// animation channel�� position, rotation, scale ��� time�� �����ϴ�.
				// ���� rotation�� �޶����� �ִϸ��̼� �̶� position, scale��� channel�� ���� �ð��� ����ֽ��ϴ�.
				if (j > static_cast<uint>(foundBoneAnimationNode->mPositionKeys[positionIndex].mTime))
				{
					positionIndex = positionIndex + 1 < maxCount ? positionIndex + 1 : positionIndex;
				}
				if (j > static_cast<uint>(foundBoneAnimationNode->mRotationKeys[rotationIndex].mTime))
				{
					rotationIndex = rotationIndex + 1 < maxCount ? rotationIndex + 1 : positionIndex;
				}
				if (j > static_cast<uint>(foundBoneAnimationNode->mScalingKeys[scaleIndex].mTime))
				{
					scaleIndex = scaleIndex + 1 < maxCount ? scaleIndex + 1 : positionIndex;
				}
				aiVector3D position = foundBoneAnimationNode->mPositionKeys[positionIndex].mValue;
				float3 newPosition(position.x, position.y, position.z);

				aiVector3D scale = foundBoneAnimationNode->mScalingKeys[scaleIndex].mValue;
				float3 newScale(scale.x, scale.y, scale.z);

				aiQuaternion& rotation = foundBoneAnimationNode->mRotationKeys[rotationIndex].mValue;
				aiMatrix3x3 aiRotationMat = rotation.GetMatrix();
				Matrix4x4 animationMatrix(
					aiRotationMat.a1 * scale.x,	aiRotationMat.a2,			aiRotationMat.a3,			0, 
					aiRotationMat.b1,			aiRotationMat.b2 * scale.y, aiRotationMat.b3,			0, 
					aiRotationMat.c1,			aiRotationMat.c2,			aiRotationMat.c3 * scale.z, 0, 
					newPosition.x, newPosition.y, newPosition.z, 1
				);

				//Transform boneTransform = bones[i]._transform;
				//Matrix4x4 temp;
				//Transform(newPosition, newRotation, newScale).ToMatrix4x4(temp);
				newAnimation[j] = animationMatrix;
			}
		}

		newBoneAnimation._animation = std::move(newAnimation);
	}

	outAnimation.SetFrameCount(animationFrameCount);
	outAnimation.SetBoneAnimations(boneAnimation);

	return true;
}
#endif

const bool FBXLoader::LoadFBXMeshFromFile(_IN_ const char* path, _OUT_ Model& outModel) const
{
#ifdef USE_ASSIMP
	return LoadFBXMeshFromFileByAssimp(path, outModel);
#else
	/*
	* todo- path ���� ���� �ؾ��� (������ fbx������ �����Ѿ��մϴ�)
	*/
	// �ϴ��� ������ fbx��� �սô�. (DKString helper ���� �Ŀ��� ������ ��!)
	CHECK_BOOL_AND_RETURN(IsFBXExtension(path));

	static FbxManager* fbxManager = nullptr;
	static FbxIOSettings* ios = nullptr;
	static FbxImporter* fbxImporter;
	if (nullptr == fbxManager)
	{
		fbxManager = FbxManager::Create();

		ios = FbxIOSettings::Create(fbxManager, IOSROOT);
		fbxManager->SetIOSettings(ios);

		fbxImporter = FbxImporter::Create(fbxManager, "MyImporter");
	}
	fbxImporter->Initialize(path, -1, fbxManager->GetIOSettings());

	FbxScene* fbxScene = FbxScene::Create(fbxManager, "MyScene");
	fbxImporter->Import(fbxScene);

	FbxGeometryConverter geometryConverter(fbxManager);
	geometryConverter.Triangulate(fbxScene, true);

	FbxNode* rootNode = fbxScene->GetRootNode();
	CreateMesh(rootNode, 0, outModel);
#endif

	return true;
}

#if !defined(USE_ASSIMP)
void ReadNormal(FbxMesh* mesh, uint controlPointIndex, int vertexCounter, float3& normal)
{
	if (1 > mesh->GetElementNormalCount())
	{
		//WLOG(L"Normal�� ������ 1�� �����Դϴ�\n");
		normal = float3(0, 0, 0);
	}
	else
	{
		const FbxGeometryElementNormal* vertexNormal = mesh->GetElementNormal(0);
		switch (vertexNormal->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
		{
			switch (vertexNormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				normal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(controlPointIndex).mData[0]);
				normal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(controlPointIndex).mData[1]);
				normal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(controlPointIndex).mData[2]);
			}
			break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int index = vertexNormal->GetIndexArray().GetAt(controlPointIndex); // �ε����� ���´�. 
				normal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
				normal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
				normal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
			}
			break;
			}
		}
		break;
		case FbxGeometryElement::eByPolygonVertex:
		{
			switch (vertexNormal->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				normal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(vertexCounter).mData[0]);
				normal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(vertexCounter).mData[1]);
				normal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(vertexCounter).mData[2]);
			}
			break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int index = vertexNormal->GetIndexArray().GetAt(vertexCounter);
				normal.x = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[0]);
				normal.z = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[1]);
				normal.y = static_cast<float>(vertexNormal->GetDirectArray().GetAt(index).mData[2]);
			}
			break;
			}
		}
		break;
		}
	}
}

void ReadUV(FbxMesh* mesh, uint controlPointIndex, int vertexCounter, float2& uv)
{
	int test = mesh->GetElementUVCount();
	if (1 > mesh->GetElementUVCount())
	{
		uv = float2(0, 0);
	}
	else
	{
		const FbxGeometryElementUV* vertexUV = mesh->GetElementUV(0);
		switch (vertexUV->GetMappingMode())
		{
		case FbxGeometryElement::eByControlPoint:
		{
			switch (vertexUV->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(controlPointIndex).mData[0]);
				uv.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(controlPointIndex).mData[1]);
			}
			break;
			case FbxGeometryElement::eIndexToDirect:
			{
				int index = vertexUV->GetIndexArray().GetAt(controlPointIndex); // �ε����� ���´�. 
				uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[0]);
				uv.y = static_cast<float>(vertexUV->GetDirectArray().GetAt(index).mData[1]);
			}
			break;
			}
		}
		break;
		case FbxGeometryElement::eByPolygonVertex:
		{
			switch (vertexUV->GetReferenceMode())
			{
			case FbxGeometryElement::eDirect:
			{
				uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[0]);
				uv.y = 1 - static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[1]);
			}
			break;
			case FbxGeometryElement::eIndexToDirect:
			{
				//int index = vertexUV->GetIndexArray().GetAt(vertexCounter);
				uv.x = static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[0]);
				uv.y = 1 - static_cast<float>(vertexUV->GetDirectArray().GetAt(vertexCounter).mData[1]);
			}
			break;
			}
			}
		break;
		}
		}
	}

void FBXLoader::CreateMesh(FbxNode* node, int depth, _OUT_ Model* outModel) const
{
	int childCount = node->GetChildCount();

	FbxNodeAttribute* nodeAttribute = node->GetNodeAttribute();
	if (nullptr != nodeAttribute)
	{
		if (FbxNodeAttribute::eMesh == nodeAttribute->GetAttributeType())
		{
			FbxMesh* mesh = node->GetMesh();
			const FbxVector4 T = node->GetGeometricTranslation(FbxNode::eSourcePivot);
			const FbxVector4 R = node->GetGeometricRotation(FbxNode::eSourcePivot);
			const FbxVector4 S = node->GetGeometricScaling(FbxNode::eSourcePivot);
			float3 translation = float3(
				static_cast<float>(T.mData[0]), 
				static_cast<float>(T.mData[2]), 
				static_cast<float>(T.mData[1])
			);
			float3 rotation = float3(
				static_cast<float>(R.mData[0]), 
				static_cast<float>(R.mData[2]), 
				static_cast<float>(R.mData[1])
			);
			float3 scale = float3(
				static_cast<float>(S.mData[0]), 
				static_cast<float>(S.mData[2]), 
				static_cast<float>(S.mData[1])
			);

			const bool success = CreateSubMesh(mesh, node, translation, rotation, scale, outModel);
			DK_ASSERT_WLOG(success == false, L"SubMesh�� ����⿡ �����߽��ϴ�. ���� �α׸� Ȯ�����ּ���.");
		}
		else if (FbxNodeAttribute::eSkeleton == nodeAttribute->GetAttributeType())
		{
			DK_LOG("FBXFile has skeleton: %d - %s", depth, node->GetName());
		}
		else
		{
			DK_LOG("FBXFile has something: %d - %s", depth, node->GetName());
		}
	}

	for (int i = 0; i < childCount; ++i)
	{
		CreateMesh(node->GetChild(i), depth + 1, outModel);
	}
}

const bool FBXLoader::CreateSubMesh(FbxMesh* mesh, FbxNode* node, float3& translation, float3& rotation, float3& scale, _OUT_ Model* outModel) const
{
	const uint pointCount = mesh->GetControlPointsCount();
	float3* positions = new float3[pointCount];
	for (uint i = 0; i < pointCount; ++i)
	{
		float3 position;
		position.x = static_cast<float>(mesh->GetControlPointAt(i).mData[0]);
		position.z = static_cast<float>(mesh->GetControlPointAt(i).mData[1]);
		position.y = static_cast<float>(mesh->GetControlPointAt(i).mData[2]);

		positions[i] = position;
	}

	const uint triCount = mesh->GetPolygonCount();
	DKVector<Vertex> vertices;
	DKVector<uint> indices;
	uint vertexCounter = 0;
	for (uint i = 0; i < triCount; ++i)
	{
		const uint polygonCount = mesh->GetPolygonSize(i);
		if (3 != polygonCount)
		{
			DK_ASSERT_LOG(3 == polygonCount, "Submeh�� polygon�� 3���� �̷������ �ʾҽ��ϴ�. polygon vertex count: %d", polygonCount);
			return false;
		}
		for (uint j = 0; j < polygonCount; ++j)
		{
			int controlPointIndex = mesh->GetPolygonVertex(i, j);
			float3& position = positions[controlPointIndex];

			float3 normal;
			ReadNormal(mesh, controlPointIndex, vertexCounter, normal);

			float2 uv;
			ReadUV(mesh, controlPointIndex, mesh->GetTextureUVIndex(i, j), uv);

			++vertexCounter;

			Vertex vertex(position, normal, uv);

			//if (���ؽ��� �̹� ���ؽ� ����Ʈ�� �ִٸ� ? )
			//{
			//	�ش� �ε����� ��������
			//	indices�� �ε����� �߰�
			//}
			//else
			//{
			uint index = static_cast<uint>(vertices.size());
			indices.push_back(index);
			vertices.push_back(vertex);
			//}

		}
	}
	DK_LOG("Submesh points: %d  - triCount: %d - vertexCount: %d - indexCount: %d", 
		pointCount, triCount, 
		static_cast<uint>(vertices.size()), 
		static_cast<uint>(indices.size())
	);

	// move���Ѽ� �������
	outModel->_subMeshes.push_back(Model::SubMeshData(std::move(vertices), std::move(indices)));

	return true;
}
#endif

const bool FBXLoader::LoadFBXSkeletonFromFile(_IN_ const char* path, _IN_ const ModelRef& model, _OUT_ Skeleton& outSkeleton)
{
#ifdef USE_ASSIMP
	LoadFBXSkeletonFromFileByAssimp(path, model, outSkeleton);
#else
#endif
	return true;
}

const bool FBXLoader::LoadFBXAnimationFromFile(_IN_ const char* path, const SkeletonRef& skeleton, _OUT_ Animation& outAnimation)
{
#if defined(USE_ASSIMP)
	LoadFBXAnimationFromFileByAssimp(path, skeleton, outAnimation);
#endif
	return true;
}