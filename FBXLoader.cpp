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
		// 현재 duckingengine_v0002는 왼손좌표계를 사용중입니다.
		// FBX ... Max는 오른손 좌표계이므로 변환합니다.
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
		aiProcess_JoinIdenticalVertices			// 동일한 꼭지점 결합, 인덱싱 최적화
		| aiProcess_ValidateDataStructure		// 로더의 출력을 검증
		| aiProcess_ImproveCacheLocality		// 출력 정점의 캐쉬위치를 개선
		| aiProcess_RemoveRedundantMaterials	// 중복된 매터리얼 제거
		| aiProcess_GenUVCoords					// 구형, 원통형, 상자 및 평면 매핑을 적절한 UV로 변환
		| aiProcess_TransformUVCoords			// UV 변환 처리기 (스케일링, 변환 ...)
		| aiProcess_FindInstances				// 인스턴스된 매쉬를 검색하여 하나의 마스터에 대한 참조로 제거
		| aiProcess_LimitBoneWeights			// 정점당 뼈의 가중치를 최대 4개로 제한
		| aiProcess_OptimizeMeshes				// 가능한 경우 작은 매쉬를 조인
		| aiProcess_GenSmoothNormals			// 부드러운 노말벡터(법선벡터) 생성
		| aiProcess_SplitLargeMeshes			// 거대한 하나의 매쉬를 하위매쉬들로 분할(나눔)
		| aiProcess_Triangulate					// 3개 이상의 모서리를 가진 다각형 면을 삼각형으로 만듦(나눔)
		| aiProcess_ConvertToLeftHanded			// D3D의 왼손좌표계로 변환
		| aiProcess_SortByPType					// 단일타입의 프리미티브로 구성된 '깨끗한' 매쉬를 만듦
	);

	CHECK_BOOL_AND_RETURN(scene != nullptr);

	uint numSubMesh = scene->mNumMeshes;		// 서브매쉬 개수

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
//		DK_ASSERT_LOG(false, "%s에 해당하는 본을 Mesh에서 찾지 못했습니다.", boneName.C_Str());
//		return;
//	}
//	std::unordered_map<const char*, uint>::iterator indexIter = boneIndexHash.find(boneName.C_Str());
//	if (boneIter != boneHash.end())
//	{
//		DK_ASSERT_LOG(false, "중복되는 본이 검출된 것 같습니다. BoneName: %s", boneName.C_Str());
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
			//DK_ASSERT_LOG(false, "%s에 해당하는 본을 Mesh에서 찾지 못했습니다.", boneName.C_Str());
			continue;
		}
		//std::unordered_map<aiString, uint>::iterator indexIter = boneIndexHash.find(boneName);
		//if (indexIter != boneIndexHash.end())
		//{
		//	DK_ASSERT_LOG(false, "중복되는 본이 검출된 것 같습니다. BoneName: %s", boneName.C_Str());
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
		//DK_ASSERT_LOG(false, "올바르지 않으 본을 참조합니다. %s", node->mName.C_Str());
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
			//DK_ASSERT_LOG(false, "올바르지 않으 본을 참조합니다. %s", node->mChildren[i]->mName.C_Str());
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
			DK_ASSERT_LOG(false, "존재하지 않는 본을 참조합니다. %s", bone->mName.C_Str());
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

				DK_ASSERT_LOG(vert.boneIndices.size() < 5, "Skinning weight는 4개를 넘을 수 없습니다.");
			}
		}
	}
}
const bool LoadFBXSkeletonFromFileByAssimp(_IN_ const char* path, _IN_ const ModelRef& model, _OUT_ Skeleton& outSkeleton)
{
	const aiScene* scene = aiImportFile(path,
		aiProcess_JoinIdenticalVertices			// 동일한 꼭지점 결합, 인덱싱 최적화
		| aiProcess_ValidateDataStructure		// 로더의 출력을 검증
		| aiProcess_ImproveCacheLocality		// 출력 정점의 캐쉬위치를 개선
		| aiProcess_RemoveRedundantMaterials	// 중복된 매터리얼 제거
		| aiProcess_GenUVCoords					// 구형, 원통형, 상자 및 평면 매핑을 적절한 UV로 변환
		| aiProcess_TransformUVCoords			// UV 변환 처리기 (스케일링, 변환 ...)
		| aiProcess_FindInstances				// 인스턴스된 매쉬를 검색하여 하나의 마스터에 대한 참조로 제거
		| aiProcess_LimitBoneWeights			// 정점당 뼈의 가중치를 최대 4개로 제한
		| aiProcess_OptimizeMeshes				// 가능한 경우 작은 매쉬를 조인
		| aiProcess_GenSmoothNormals			// 부드러운 노말벡터(법선벡터) 생성
		| aiProcess_SplitLargeMeshes			// 거대한 하나의 매쉬를 하위매쉬들로 분할(나눔)
		| aiProcess_Triangulate					// 3개 이상의 모서리를 가진 다각형 면을 삼각형으로 만듦(나눔)
		| aiProcess_ConvertToLeftHanded			// D3D의 왼손좌표계로 변환
		| aiProcess_SortByPType					// 단일타입의 프리미티브로 구성된 '깨끗한' 매쉬를 만듦
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

		// 모든 본 hashing
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
	// 모든 본 vector로 build
	const uint boneCount = boneHash.size();
	const aiNode* rootNode = scene->mRootNode;
	std::vector<Bone> bones;
	std::unordered_map<std::string, uint> boneIndexHash;
	BuildBoneVectorAndIndexHashMapChild(boneHash, rootNode, bones, boneIndexHash);
	DK_ASSERT_LOG(boneCount == bones.size(), "Bone 구성이 잘 이루어지지 않았습니다.");
	DK_ASSERT_LOG(boneCount == boneIndexHash.size(), "Bone 구성이 잘 이루어지지 않았습니다.");

	// rootNode는 본이 아니기 떄문에 rootNode의 Child에서 시작해야함
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
					DK_ASSERT_LOG(false, "Skinning VertexIndex가 Vertex Count를 넘어섭니다. %d/%d", vertexIndex, model->GetSubMeshes()[0]._vertices.size());
					continue;
				}
				const float weight = aiWeight.mWeight;

				std::vector<SubMesh>& submeshes = model->GetSubMeshesWritable();
				Vertex& vertex = submeshes[0]._vertices[vertexIndex];
				vertex.boneIndices.push_back(boneIndex);
				vertex.weights.push_back(weight);
				if (vertex.boneIndices.size() > 4)
				{
					DK_ASSERT_LOG(false, "Skinning 한계개수를 넘어섰습니다. %d/%d", vertex.boneIndices.size(), 4);
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
		aiProcess_JoinIdenticalVertices			// 동일한 꼭지점 결합, 인덱싱 최적화
		| aiProcess_ValidateDataStructure		// 로더의 출력을 검증
		| aiProcess_ImproveCacheLocality		// 출력 정점의 캐쉬위치를 개선
		| aiProcess_RemoveRedundantMaterials	// 중복된 매터리얼 제거
		| aiProcess_GenUVCoords					// 구형, 원통형, 상자 및 평면 매핑을 적절한 UV로 변환
		| aiProcess_TransformUVCoords			// UV 변환 처리기 (스케일링, 변환 ...)
		| aiProcess_FindInstances				// 인스턴스된 매쉬를 검색하여 하나의 마스터에 대한 참조로 제거
		| aiProcess_LimitBoneWeights			// 정점당 뼈의 가중치를 최대 4개로 제한
		| aiProcess_OptimizeMeshes				// 가능한 경우 작은 매쉬를 조인
		| aiProcess_GenSmoothNormals			// 부드러운 노말벡터(법선벡터) 생성
		| aiProcess_SplitLargeMeshes			// 거대한 하나의 매쉬를 하위매쉬들로 분할(나눔)
		| aiProcess_Triangulate					// 3개 이상의 모서리를 가진 다각형 면을 삼각형으로 만듦(나눔)
		| aiProcess_ConvertToLeftHanded			// D3D의 왼손좌표계로 변환
		| aiProcess_SortByPType					// 단일타입의 프리미티브로 구성된 '깨끗한' 매쉬를 만듦
	);

	CHECK_BOOL_AND_RETURN(scene != nullptr);

	//uint numAnimation = scene->mNumAnimations;	// 애니메이션 개수	

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
	* todo- path 검증 먼저 해야함 (무조건 fbx파일을 가리켜야합니다)
	*/
	// 일단은 무조건 fbx라고 합시다. (string helper 제작 후에는 검증할 것!)
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
		//WLOG(L"Normal의 개수가 1개 이하입니다\n");
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
				int index = vertexNormal->GetIndexArray().GetAt(controlPointIndex); // 인덱스를 얻어온다. 
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
				int index = vertexUV->GetIndexArray().GetAt(controlPointIndex); // 인덱스를 얻어온다. 
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
			DK_ASSERT_WLOG(success == false, L"SubMesh를 만들기에 실패했습니다. 상위 로그를 확인해주세요.");
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
			DK_ASSERT_LOG(3 == polygonCount, "Submeh의 polygon이 3개로 이루어지지 않았습니다. polygon vertex count: %d", polygonCount);
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

			//if (버텍스가 이미 버텍스 리스트에 있다면 ? )
			//{
			//	해당 인덱스를 가져오고
			//	indices에 인덱스만 추가
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

	// move시켜서 성능향상
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