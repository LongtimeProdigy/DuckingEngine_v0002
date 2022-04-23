#include "stdafx.h"
#include "SkinnedMeshComponent.h"

#include "DuckingEngine.h"
#include "ResourceManager.h"
#include "RenderModule.h"

#include "Model.h"
#include "Material.h"

const char* ConvertModelPathToModelPropertyPath(const char* modelPath)
{
	DK_ASSERT_LOG(false, "아직 ModelProperty를 구현하지 않았는데 해당 함수를 사용합니다. 꼭 확인!!!!!");
	return nullptr;
}

bool SkinnedMeshComponent::LoadResource(const char* modelPath, const char* skeletonPath, const char* animationSetPath, const char* modelPropertyPath, SceneObject* sceneObject)
{
	if (strcmp(modelPath, "Plane") == 0)
	{
		static float testPlaneLength = 100000.0f;
		Model planeModel;
		SubMesh planeSubMesh;
		std::vector<Vertex> vertices = {
			Vertex(float3(-testPlaneLength, 0.0f, -testPlaneLength)), 
			Vertex(float3(testPlaneLength, 0.0f, -testPlaneLength)), 
			Vertex(float3(-testPlaneLength, 0.0f, testPlaneLength)), 
			Vertex(float3(testPlaneLength, 0.0f, testPlaneLength))
		};
		planeSubMesh._vertices = std::move(vertices);
		planeSubMesh._indices = {
			0, 1, 2, 
			2, 1, 3
		};
		std::vector<SubMesh> subMeshes = { planeSubMesh };

		_model = ModelRef(dk_new Model);
		_model->SetSubMeshes(subMeshes);
	}
	else
	{
		ResourceManager* resourceManager = DuckingEngine::GetResourceManagerWritable();

		// Load Mesh
		bool success = resourceManager->LoadMesh(modelPath, _model);
		CHECK_BOOL_AND_RETURN(success == true);

		// Load Skeleton
		success = resourceManager->LoadSkeleton(skeletonPath, _model, _skeleton);
		CHECK_BOOL_AND_RETURN(success == true);

		// Load Animation
		success = resourceManager->LoadAnimation(animationSetPath, _animation);
		CHECK_BOOL_AND_RETURN(success == true);
	}

	// GPU resource
	{
		RenderModule* rm = DuckingEngine::GetRenderModuleWritable();
		std::vector<SubMesh>& subMeshes = _model->GetSubMeshesWritable();
		for (uint i = 0; i < subMeshes.size(); ++i)
		{
			SubMesh& submesh = subMeshes[i];

			const bool vertexBufferSuccess = rm->LoadVertexBuffer(
				&submesh._vertices[0], 
				sizeof(Vertex), 
				sizeof(Vertex) * static_cast<uint>(submesh._vertices.size()),
				submesh._vertexBufferView
			);
			CHECK_BOOL_AND_RETURN(vertexBufferSuccess);

			const bool indexBufferSuccess = rm->LoadIndexBuffer(
				&submesh._indices[0], 
				sizeof(uint) * static_cast<uint>(submesh._indices.size()), 
				submesh._indexBufferView
			);
			CHECK_BOOL_AND_RETURN(indexBufferSuccess);

			submesh._material = rm->CreateMaterial(MaterialType::SKINNEDMESH, sceneObject);
			CHECK_BOOL_AND_RETURN(submesh._material != nullptr);

			// setting parameters

			// loadtexutre..
			// create texture srv
			// craete descriptorheap
		}
	}

	return true;
}