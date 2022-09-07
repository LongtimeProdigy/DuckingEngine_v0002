#include "stdafx.h"
#include "SkinnedMeshComponent.h"

#include "DuckingEngine.h"
#include "ResourceManager.h"
#include "RenderModule.h"

#include "Model.h"
#include "Material.h"
#include "MaterialParameter.h"
#include "Skeleton.h"
#include "Animation.h"

const char* ConvertModelPathToModelPropertyPath(const char* modelPath)
{
	DK_ASSERT_LOG(false, "아직 ModelProperty를 구현하지 않았는데 해당 함수를 사용합니다. 꼭 확인!!!!!");
	return nullptr;
}

bool SkinnedMeshComponent::LoadResource(const char* modelPath, const char* skeletonPath, const char* animationSetPath, const char* modelPropertyPath)
{
	ResourceManager& resourceManager = DuckingEngine::getInstance().GetResourceManagerWritable();

	// Load Mesh
	if (resourceManager.LoadMesh(modelPath, _model) == false) return false;
	// Load Skeleton
	if (resourceManager.LoadSkeleton(skeletonPath, _model, _skeleton) == false) return false;
	// Load Animation
	if (resourceManager.LoadAnimation(animationSetPath, _skeleton, _animation) == false) return false;

	// Load ModelProperty (Model Property는 Character 별로 존재한다. 따라서 ResourceManager를 써선안된다)
	DKVector<MaterialDefinition> modelProperties;
#if defined(USE_TINYXML)
	TiXmlDocument doc;
	doc.LoadFile(modelPropertyPath);

	const TiXmlElement* rootNode = doc.FirstChildElement("ModelProperty");

	const TiXmlElement* subMeshNode = rootNode->FirstChildElement("SubMesh");
	const uint subMeshCount = atoi(subMeshNode->Attribute("Count"));
	modelProperties.reserve(subMeshCount);

#if defined(_DK_DEBUG_)
	const uint32 modelSubMeshCount = _model->GetSubMeshes().size();
	DK_ASSERT_LOG(subMeshCount == modelSubMeshCount, "Load된 SubMesh개수와 Total SubMesh개수가 다릅니다.");
#endif	// _DK_DEBUG_
	for (const TiXmlElement* childNode = subMeshNode->FirstChildElement(); childNode != nullptr; childNode = childNode->NextSiblingElement())
	{
		const DKString materialName = subMeshNode->Attribute("Name");

		MaterialDefinition modelPropertyRaw;
		modelPropertyRaw._materialName = materialName;

		const TiXmlElement* parametersNode = subMeshNode->FirstChildElement("Parameters");
		for (const TiXmlElement* parameterNode = parametersNode->FirstChildElement(); parameterNode != nullptr; parameterNode = parameterNode->NextSiblingElement())
		{
			MaterialParameterDefinition parameterRaw;
			parameterRaw._name = parameterNode->Attribute("Name");
			parameterRaw._value = parameterNode->Value();

			modelPropertyRaw._parameters.push_back(std::move(parameterRaw));
		}

		modelProperties.push_back(std::move(modelPropertyRaw));
	}
#else	// USE_TINYXML
	static_assert(false, "XML Loader가 없습니다.");
#endif	// USE_TINYXML

	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	DKVector<SubMesh>& subMeshes = _model->GetSubMeshesWritable();
	const uint32 subMeshCount = subMeshes.size();

	for (uint subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
	{
		SubMesh& submesh = subMeshes[subMeshIndex];
		submesh._material = Material::createMaterial(modelProperties[subMeshIndex]);
		if (submesh._material == nullptr) return false;
	}

	for (uint subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
	{
		SubMesh& submesh = subMeshes[subMeshIndex];
		if (submesh._vertexBufferView.get() == nullptr)
		{
			const bool vertexBufferSuccess = renderModule.loadVertexBuffer(
				modelPath, subMeshIndex, &submesh._vertices[0], sizeof(Vertex), sizeof(Vertex) * static_cast<uint>(submesh._vertices.size()), submesh._vertexBufferView
			);
			if (vertexBufferSuccess == false) return false;
		}

		if (submesh._indexBufferView.get() == nullptr)
		{
			const bool indexBufferSuccess = renderModule.loadIndexBuffer(
				&submesh._indices[0], sizeof(uint) * static_cast<uint>(submesh._indices.size()), submesh._indexBufferView
			);
			if (indexBufferSuccess == false) return false;
		}
	}

	return true;
}

void SkinnedMeshComponent::Update(float deltaTime)
{
	const DKVector<Bone>& bones = _skeleton->GetBones();
	const uint boneCount = static_cast<uint>(bones.size());

	const DKVector<Animation::BoneAnimation>& boneAnimations = _animation->GetBoneAnimation();
	const float currentAnimationTime = static_cast<const float>(fmodf(_animation->GetAnimationTime() + deltaTime * 30, _animation->GetFrameCount()));
	_animation->SetAnimationTime(currentAnimationTime);

	DK_ASSERT_LOG(boneCount == boneAnimations.size(), "Skeleton과 Animation의 Bone Count가 다릅니다.");

	// build CharacterSpace DressPose Bone InvertMatrix
	DKVector<Matrix4x4> characterSpaceBoneMatrix;
	characterSpaceBoneMatrix.resize(boneCount);
	DKVector<Matrix4x4> characterSpaceInvertBoneMatrix;
	characterSpaceInvertBoneMatrix.resize(boneCount);
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		Matrix4x4 outMatrix;
		bones[boneIndex]._transform.ToMatrix4x4(outMatrix);
		characterSpaceBoneMatrix[boneIndex] = outMatrix * characterSpaceBoneMatrix[boneIndex];

		Transform tempTransform = bones[boneIndex]._transform;
		tempTransform.Invert();
		Matrix4x4 outInvertMatrix;
		tempTransform.ToMatrix4x4(outInvertMatrix);
		characterSpaceInvertBoneMatrix[boneIndex] = characterSpaceInvertBoneMatrix[boneIndex] * outInvertMatrix;

		const Bone& bone = bones[boneIndex];
		for (uint childBoneIndex = 0; childBoneIndex < bone._childs.size(); ++childBoneIndex)
		{
			characterSpaceBoneMatrix[bone._childs[childBoneIndex]] = characterSpaceBoneMatrix[boneIndex];
			characterSpaceInvertBoneMatrix[bone._childs[childBoneIndex]] = characterSpaceInvertBoneMatrix[boneIndex];
		}
	}

	// build CharacterSpace Animated Bone Matrix
	DKVector<Matrix4x4> currentCharacterSpaceBoneAnimation;
	currentCharacterSpaceBoneAnimation.clear();
	currentCharacterSpaceBoneAnimation.resize(boneCount);
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		Matrix4x4 outMatrix = boneAnimations[boneIndex]._animation[static_cast<size_t>(currentAnimationTime)];
		//Matrix4x4 outMatrix = boneAnimations[boneIndex]._animation[static_cast<size_t>(0)];

		currentCharacterSpaceBoneAnimation[boneIndex] = outMatrix * currentCharacterSpaceBoneAnimation[boneIndex];

		const Bone& bone = bones[boneIndex];
		for (uint childBoneIndex = 0; childBoneIndex < bone._childs.size(); ++childBoneIndex)
		{
			currentCharacterSpaceBoneAnimation[bone._childs[childBoneIndex]] = currentCharacterSpaceBoneAnimation[boneIndex];
		}
	}

	// skinning
	_currentCharacterSpaceBoneAnimation.clear();
	_currentCharacterSpaceBoneAnimation.resize(boneCount);
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		Matrix4x4 boneSpaceAnimationMatrix = 
			//characterSpaceBoneMatrix[boneIndex];
			characterSpaceInvertBoneMatrix[boneIndex]
			* currentCharacterSpaceBoneAnimation[boneIndex];

		_currentCharacterSpaceBoneAnimation[boneIndex] = boneSpaceAnimationMatrix;
		_currentCharacterSpaceBoneAnimation[boneIndex].Transpose();
	}
}