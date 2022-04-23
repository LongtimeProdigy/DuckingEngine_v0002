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
void PushBoneHirarchy(
	const std::unordered_map<const char*, const aiBone*>& bones, 
	std::vector<Bone>& boneVec, 
	const aiNode* aiNode
)
{
	// #todo- �̰���  rootBone�� �� �Ѱ���� �����ϰ� �ۼ��Ǿ����ϴ�.
	const aiBone* foundBone = nullptr;
	if (bones.find(aiNode->mName.C_Str()) != bones.end())
	{
		foundBone = bones.find(aiNode->mName.C_Str())->second;
	}
	else
	{
		DK_ASSERT_LOG(false, "�������� �ʴ� Bone�� �����մϴ�. �ݵ�� Ȯ���� �ʿ��մϴ�.");
		return;
	}

	std::string boneName = foundBone->mName.C_Str();
	aiVector3D aiLocation, aiRotation, aiScale;
	foundBone->mOffsetMatrix.Decompose(aiScale, aiRotation, aiLocation);
	float3 location(aiLocation.x, aiLocation.y, aiLocation.z);
	float3 rotation(aiRotation.x, aiRotation.y, aiRotation.z);
	float3 scale(aiScale.x, aiScale.y, aiScale.z);
	Transform boneTransform(location, rotation, scale);

	uint childCount = aiNode->mNumChildren;
	std::vector<uint> childIndices;
	childIndices.reserve(childCount);
	uint boneIndex = boneVec.size();
	for (uint i = 0; i < childCount; ++i)
	{
		childIndices.push_back(boneIndex + i);
	}

	boneVec.push_back(Bone(boneName, boneTransform, childIndices));
	for (uint i = 0; i < childCount; ++i)
	{
		boneVec.push_back(Bone(boneName, boneTransform, childIndices))
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
	const aiMesh* mesh = scene->mMeshes[0];
	if (mesh == nullptr || mesh->HasBones() == false)
	{
		return false;
	}

	const uint boneCount = mesh->mNumBones;
	std::vector<Bone> bones;
	bones.reserve(boneCount);
#if 1
	// ��� �� hashing
	std::unordered_map<const char*, aiBone*> boneIndexMapper;
	boneIndexMapper.reserve(boneCount);
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		aiBone* aiBone = mesh->mBones[boneIndex];
		const char* boneName = aiBone->mName.C_Str();

		auto foundResult = boneIndexMapper.find(boneName);
		if (foundResult != boneIndexMapper.end())
		{
			continue;
		}

		boneIndexMapper.insert(std::make_pair(boneName, aiBone));
	}

	// skeleton ����
	aiNode* rootNode = scene->mRootNode;
	std::vector<int> rootBoneIndices;
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		const aiBone* aiBone = mesh->mBones[boneIndex];
	}
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
const bool LoadFBXAnimationFromFileByAssimp(_IN_ const char* path, _OUT_ Animation& outAnimation)
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
	float duration = animation->mDuration;
	float tick = animation->mTicksPerSecond;
	uint channelCount = animation->mNumChannels;


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