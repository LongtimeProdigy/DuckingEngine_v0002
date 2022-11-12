#include "stdafx.h"
#include "SkinnedMeshComponent.h"

#include "DuckingEngine.h"
#include "ResourceManager.h"
#include "RenderModule.h"

#include "Model.h"
#include "Material.h"
#include "Skeleton.h"
#include "Animation.h"

#include "EditorDebugDrawManager.h"

bool SkinnedMeshComponent::LoadResource(const DKString& modelPath, const DKString& skeletonPath, const DKString& animationSetPath, const DKString& modelPropertyPath)
{
	ResourceManager& resourceManager = DuckingEngine::getInstance().GetResourceManagerWritable();

	// Load Mesh
	if (resourceManager.LoadMesh(modelPath, _model) == false) return false;
	// Load Skeleton
	if (resourceManager.LoadSkeleton(skeletonPath, _model, _skeleton) == false) return false;
	// Load Animation
	if (resourceManager.LoadAnimation(animationSetPath, _skeleton, _animation) == false) return false;

#if defined(USE_TINYXML)
	TiXmlDocument doc;
	doc.LoadFile(modelPropertyPath.c_str());

	const TiXmlElement* rootNode = doc.FirstChildElement("ModelProperty");

	RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
	DKVector<SubMesh>& subMeshes = _model->GetSubMeshesWritable();
	const TiXmlElement* subMeshesNode = rootNode->FirstChildElement("SubMeshes");
	const uint subMeshCount = atoi(subMeshesNode->Attribute("Count"));
	DK_ASSERT_LOG(subMeshCount == _model->GetSubMeshes().size(), "Load된 SubMesh개수와 Total SubMesh개수가 다릅니다.");
	DKVector<MaterialDefinition> modelProperties;
	const TiXmlElement* subMeshNode = subMeshesNode->ToElement()->FirstChildElement();
	for (uint32 i = 0; i < subMeshCount; ++i)
	{
		MaterialDefinition modelProperty;
		modelProperty._materialName = subMeshNode->Attribute("Name");
		for (const TiXmlElement* childNode = subMeshNode->FirstChildElement(); childNode != nullptr; childNode = childNode->NextSiblingElement())
		{
			const TiXmlElement* parametersNode = subMeshNode->FirstChildElement("Parameters");
			for (const TiXmlElement* parameterNode = parametersNode->FirstChildElement(); parameterNode != nullptr; parameterNode = parameterNode->NextSiblingElement())
			{
				MaterialParameterDefinition parameterRaw;
				parameterRaw._name = parameterNode->Attribute("Name");
				parameterRaw._type = convertStringToEnum(parameterNode->Attribute("Type"));
				parameterRaw._value = parameterNode->GetText();
				modelProperty._parameters.push_back(std::move(parameterRaw));
			}

			modelProperties.push_back(std::move(modelProperty));
		}

		subMeshNode = subMeshNode->NextSiblingElement();
	}
#else	// USE_TINYXML
	static_assert(false, "XML Loader가 없습니다.");
#endif	// USE_TINYXML

	for (uint subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
	{
		SubMesh& submesh = subMeshes[subMeshIndex];
		if (Material::createMaterial(modelProperties[subMeshIndex], submesh._material) == false) return false;
	}

	for (uint subMeshIndex = 0; subMeshIndex < subMeshCount; ++subMeshIndex)
	{
		SubMesh& submesh = subMeshes[subMeshIndex];
		if (submesh._vertexBufferView.get() == nullptr)
		{
			const bool vertexBufferSuccess = renderModule.createVertexBuffer(
				&submesh._vertices[0], sizeof(Vertex), static_cast<uint32>(submesh._vertices.size()), submesh._vertexBufferView
			);
			if (vertexBufferSuccess == false) return false;
		}

		if (submesh._indexBufferView.get() == nullptr)
		{
			const bool indexBufferSuccess = renderModule.createIndexBuffer(
				&submesh._indices[0], static_cast<uint32>(submesh._indices.size()), submesh._indexBufferView
			);
			if (indexBufferSuccess == false) return false;
		}
	}

	const uint32 boneCount = static_cast<uint32>(_skeleton->GetBones().size());
	DKVector<Matrix4x4> animationMatrices;
	animationMatrices.resize(boneCount);
	for (uint32 i = 0; i < boneCount; ++i)
	{
		const Bone& bone = _skeleton->GetBones()[i];
		bone._transform.ToMatrix4x4(animationMatrices[i]);
	}

	_skeletonConstantBuffer = renderModule.createUploadBuffer(animationMatrices.data(), sizeof(Matrix4x4) * static_cast<uint32>(animationMatrices.size()));

	return true;
}

void SkinnedMeshComponent::update(float deltaTime)
{
	const DKVector<Bone>& bones = _skeleton->GetBones();
	const uint boneCount = static_cast<uint>(bones.size());

	const DKVector<Animation::BoneAnimation>& boneAnimations = _animation->GetBoneAnimation();
	const float currentAnimationTime = fmodf(_animation->GetAnimationTime() + deltaTime, static_cast<const float>(_animation->GetFrameCount()));
	_animation->SetAnimationTime(currentAnimationTime);

	DK_ASSERT_LOG(boneCount == boneAnimations.size(), "Skeleton과 Animation의 Bone Count가 다릅니다.");

	// build InvertMatrix by CharacterSpace Bone DressPose
	DKVector<Matrix4x4> characterSpaceBoneMatrix;
	DKVector<Matrix4x4> characterSpaceInvertBoneMatrix;
	characterSpaceBoneMatrix.resize(boneCount);
	characterSpaceInvertBoneMatrix.resize(boneCount);
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		Matrix4x4 outMatrix;
		bones[boneIndex]._transform.ToMatrix4x4(outMatrix);
		if(bones[boneIndex]._parentBoneIndex != 0xffffffff)
			characterSpaceBoneMatrix[boneIndex] = characterSpaceBoneMatrix[bones[boneIndex]._parentBoneIndex];	// parent * current(child)
		characterSpaceBoneMatrix[boneIndex] = characterSpaceBoneMatrix[boneIndex] * outMatrix;

		Transform tempTransform = bones[boneIndex]._transform;
		tempTransform.Invert();
		Matrix4x4 outInvertMatrix;
		tempTransform.ToMatrix4x4(outInvertMatrix);
		characterSpaceInvertBoneMatrix[boneIndex] = characterSpaceInvertBoneMatrix[boneIndex] * outInvertMatrix;
	}

	for (const Matrix4x4& characterSpacePosition : characterSpaceBoneMatrix)
	{
		EditorDebugDrawManager::getSingleton().addSphere(characterSpacePosition.getTranslation(), float3(1, 1, 1), 0.02f);
	}

	// build CharacterSpace Animated Bone Matrix
	DKVector<Matrix4x4> currentCharacterSpaceBoneAnimation;
	currentCharacterSpaceBoneAnimation.clear();
	currentCharacterSpaceBoneAnimation.resize(boneCount);
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		Matrix4x4 outMatrix = boneAnimations[boneIndex]._animation[static_cast<size_t>(currentAnimationTime)];
		if (bones[boneIndex]._parentBoneIndex != 0xffffffff)
			currentCharacterSpaceBoneAnimation[boneIndex] = currentCharacterSpaceBoneAnimation[bones[boneIndex]._parentBoneIndex];	// parent * current(child)
		currentCharacterSpaceBoneAnimation[boneIndex] = currentCharacterSpaceBoneAnimation[boneIndex] * outMatrix;	// parent * current(child)
	}

	/*
	* Skinning Matrix 자체는 Vertex단위로 있어야합니다. 이 곳에서는 bone단위의 Invert * Animation을 구하고
	* 셰이더에서 Vertex단위로 weight를 곱해주도록 합니다.
	*/
	// Build Skinning Matrix(Bone)
	_currentCharacterSpaceBoneAnimation.clear();
	_currentCharacterSpaceBoneAnimation.resize(boneCount);
	for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
	{
		// Skinning : (Invert * Animation) * weight
		Matrix4x4 boneSpaceAnimationMatrix = characterSpaceInvertBoneMatrix[boneIndex] * currentCharacterSpaceBoneAnimation[boneIndex];
		_currentCharacterSpaceBoneAnimation[boneIndex] = boneSpaceAnimationMatrix;

		_currentCharacterSpaceBoneAnimation[boneIndex].Transpose();

		//_currentCharacterSpaceBoneAnimation[boneIndex] = Matrix4x4::Identity;
	}
}