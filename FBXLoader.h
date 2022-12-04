#pragma once

class FbxNode;
class FbxMesh;

namespace DK
{
	class Model;
	class Skeleton;
	class Animation;
}

namespace DK
{
	class FBXLoader
	{
	public:
		const bool LoadFBXMeshFromFile(_IN_ const char* path, _OUT_ Model& outModel) const;
		void CreateMesh(FbxNode* node, int depth, _OUT_ Model* outModel) const;
		const bool CreateSubMesh(FbxMesh* mesh, 
			FbxNode* node, float3& translation, float3& rotation, float3& scale, _OUT_ Model* outModel) const;

		const bool LoadFBXSkeletonFromFile(_IN_ const char* path, _IN_ const ModelRef& model, _OUT_ Skeleton& outSkeleton);
		const bool LoadFBXAnimationFromFile(_IN_ const char* path, const SkeletonRef& skeleton, _OUT_ Animation& outAnimation);

		//#pragma region Assimp
		//#ifdef USE_ASSIMP
		//	const bool LoadFBXMeshFromFileByAssimp(_IN_ const char* path, _OUT_ Model* outModel) const;
		//	void InitMeshFromAssimpToSubMesh(_IN_ const aiMesh* assimpMesh, _OUT_ DKVector<Vertex>& vertices, _OUT_ std::vector<uint>& indices) const;
		//#endif // #ifdef USE_ASSIMP
		//#pragma endregion
	};
}