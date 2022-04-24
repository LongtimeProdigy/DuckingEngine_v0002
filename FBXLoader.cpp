#include "stdafx.h"
#include "FBXLoader.h"

#pragma region LIB
#include "fbxsdk.h"

#define USE_ASSIMP
#ifdef USE_ASSIMP
#include "assimp/Importer.hpp"
#include "assimp/cimport.h"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#endif
#pragma endregion

#include "float2.h"
#include "float3.h"
#include "Transform.h"

#include "DuckingEngine.h"

#include "Model.h"
#include "Skeleton.h"
#include "Animation.h"

#ifdef USE_ASSIMP
void InitMeshFromAssimpToSubMesh(_IN_ const aiMesh* assimpMesh, _OUT_ std::vector<Vertex>& vertices, _OUT_ std::vector<uint>& indices)
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
	std::string boneName = foundBone->mName.C_Str();
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

	CHECK_BOOL_AND_RETURN(scene != nullptr);

	uint numSubMesh = scene->mNumMeshes;		// ����Ž� ����

	RenderModule* rm = DuckingEngine::_duckingEngine->GetRenderModuleWritable();
	std::vector<SubMesh> subMeshes;
	subMeshes.resize(numSubMesh);
	for (uint i = 0; i < numSubMesh; ++i)
	{
		const aiMesh* mesh = scene->mMeshes[i];
		InitMeshFromAssimpToSubMesh(mesh, subMeshes[i]._vertices, subMeshes[i]._indices);
	}
	outModel.SetSubMeshes(subMeshes);

	return true;
}

Bone ConvertAIBoneToBone(const aiBone* sourceBone)
{
	std::string boneName(sourceBone->mName.C_Str());

	aiVector3D aiPosition, aiRotation, aiScale;
	sourceBone->mOffsetMatrix.Decompose(aiScale, aiRotation, aiPosition);
	float3 position(aiPosition.x, aiPosition.y, aiPosition.z);
	float3 rotation(aiRotation.x, aiRotation.y, aiRotation.z);
	float3 scale(aiScale.x, aiScale.y, aiScale.z);

	return Bone(boneName, Transform(position, rotation, scale));
}
//void BuildBoneVectorAndIndexHashMapChild(
//	const std::unordered_map<const char*, const aiBone*>& boneHash,
//	const aiNode* node,
//	std::vector<Bone>& boneVector,
//	std::unordered_map<const char*, uint>& boneIndexHash
//);
//void BuildBoneVectorAndIndexHashMap(
//	const std::unordered_map<const char*, const aiBone*>& boneHash,
//	const aiNode* node,
//	std::vector<Bone>& boneVector,
//	std::unordered_map<const char*, uint>& boneIndexHash
//)
//{
//	const aiString& boneName = node->mName;
//	const std::unordered_map<const char*, const aiBone*>::const_iterator boneIter = boneHash.find(boneName.C_Str());
//	if (boneIter == boneHash.end())
//	{
//		DK_ASSERT_LOG(false, "%s�� �ش��ϴ� ���� Mesh���� ã�� ���߽��ϴ�.", boneName.C_Str());
//		return;
//	}
//	std::unordered_map<const char*, uint>::iterator indexIter = boneIndexHash.find(boneName.C_Str());
//	if (boneIter != boneHash.end())
//	{
//		DK_ASSERT_LOG(false, "�ߺ��Ǵ� ���� ����� �� �����ϴ�. BoneName: %s", boneName.C_Str());
//		return;
//	}
//
//	uint boneIndex = static_cast<uint>(boneVector.size());
//	boneVector.push_back(ConvertAIBoneToBone(boneIter->second));
//	boneIndexHash.insert(std::make_pair(boneName.C_Str(), boneIndex));
//
//	BuildBoneVectorAndIndexHashMapChild(boneHash, node, boneVector, boneIndexHash);
//}
void BuildBoneVectorAndIndexHashMapChild(
	const std::unordered_map<std::string, const aiBone*>& boneHash,
	const aiNode* node, 
	std::vector<Bone>& boneVector, 
	std::unordered_map<std::string, uint>& boneIndexHash
)
{
	uint childCount = node->mNumChildren;
	for (uint i = 0; i < childCount; ++i)
	{
		const aiString boneName = node->mChildren[i]->mName;
		const std::unordered_map<std::string, const aiBone*>::const_iterator boneIter = boneHash.find(boneName.C_Str());
		if (boneIter == boneHash.end())
		{
			//DK_ASSERT_LOG(false, "%s�� �ش��ϴ� ���� Mesh���� ã�� ���߽��ϴ�.", boneName.C_Str());
			continue;
		}
		//std::unordered_map<aiString, uint>::iterator indexIter = boneIndexHash.find(boneName);
		//if (indexIter != boneIndexHash.end())
		//{
		//	DK_ASSERT_LOG(false, "�ߺ��Ǵ� ���� ����� �� �����ϴ�. BoneName: %s", boneName.C_Str());
		//	return;
		//}

		uint boneIndex = static_cast<uint>(boneVector.size());
		boneVector.push_back(ConvertAIBoneToBone(boneIter->second));
		boneIndexHash.insert(std::make_pair(boneName.C_Str(), boneIndex));
	}

	for (uint i = 0; i < childCount; ++i)
	{
		BuildBoneVectorAndIndexHashMapChild(boneHash, node->mChildren[i], boneVector, boneIndexHash);
	}
}
void BuildBoneChildIndex(
	const aiNode* node, 
	const std::unordered_map<std::string, uint>& boneIndexHash,
	std::vector<Bone>& boneVector
)
{
	std::unordered_map<std::string, uint>::const_iterator parentBoneIter = boneIndexHash.find(node->mName.C_Str());
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
		std::unordered_map<std::string, uint>::const_iterator childBoneIter = boneIndexHash.find(node->mChildren[i]->mName.C_Str());
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
	const std::unordered_map<std::string, const aiBone*>& boneHash,
	const std::unordered_map<std::string, uint>& boneIndexHash
)
{
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

			std::vector<SubMesh>& submeshes = model->GetSubMeshesWritable();
			for (uint j = 0; j < submeshes.size(); ++j)
			{
				if (vertIndex >= submeshes[j]._vertices.size())
				{
					continue;
				}

				Vertex& vert = submeshes[j]._vertices[vertIndex];
				vert.boneIndices.push_back(boneIndexHash.find(bone->mName.C_Str())->second);
				vert.weights.push_back(weight);

				DK_ASSERT_LOG(vert.boneIndices.size() < 5, "Skinning weight�� 4���� ���� �� �����ϴ�.");
			}
		}
	}
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

	CHECK_BOOL_AND_RETURN(scene != nullptr);

	RenderModule* rm = DuckingEngine::_duckingEngine->GetRenderModuleWritable();

	std::unordered_map<std::string, const aiBone* > boneHash;
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
	const uint boneCount = boneHash.size();
	const aiNode* rootNode = scene->mRootNode;
	std::vector<Bone> bones;
	std::unordered_map<std::string, uint> boneIndexHash;
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

			std::string boneName = aiBone->mName.C_Str();
			aiVector3D translation, rotation, scale;
			aiBone->mOffsetMatrix.Decompose(scale, rotation, translation);
			float3 translationF(translation.x, translation.y, translation.z);
			float3 rotationF(rotation.x, rotation.y, rotation.z);
			float3 scaleF(scale.x, scale.y, scale.z);
			Transform boneTransform(translationF, rotationF, scaleF);

			Bone newBone;
#if defined(DK_DEBUG)
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

				std::vector<SubMesh>& submeshes = model->GetSubMeshesWritable();
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
const bool LoadFBXAnimationFromFileByAssimp(_IN_ const char* path, const Skeleton& skeleton, _OUT_ Animation& outAnimation)
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

	CHECK_BOOL_AND_RETURN(scene != nullptr);

	//uint numAnimation = scene->mNumAnimations;	// �ִϸ��̼� ����	

	RenderModule* rm = DuckingEngine::_duckingEngine->GetRenderModuleWritable();

	aiAnimation* animation = scene->mAnimations[0];
	std::string animationName = animation->mName.C_Str();
	float duration = static_cast<float>(animation->mDuration);
	float tick = static_cast<float>(animation->mTicksPerSecond);

	const std::vector<Bone>& bones = skeleton.GetBones();
	for (uint i = 0; i < bones.size(); ++i)
	{
		const std::string boneName = bones[i]._boneName;

		uint channelCount = animation->mNumChannels;
		for (uint i = 0; i < channelCount; ++i)
		{
			animation->mChannels[i];
		}
	}

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
	// �ϴ��� ������ fbx��� �սô�. (string helper ���� �Ŀ��� ������ ��!)
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
	std::vector<Vertex> vertices;
	std::vector<uint> indices;
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

const bool FBXLoader::LoadFBXAnimationFromFile(_IN_ const char* path, _OUT_ Animation& outAnimation)
{
#ifdef USE_ASSIMP
	LoadFBXAnimationFromFileByAssimp(path, outAnimation);
#else
#endif
	return true;
}