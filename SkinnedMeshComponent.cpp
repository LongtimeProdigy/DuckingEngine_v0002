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

namespace DK
{
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
		DKVector<float4x4> animationMatrices;
		animationMatrices.resize(boneCount);
		for (uint32 i = 0; i < boneCount; ++i)
		{
			const Bone& bone = _skeleton->GetBones()[i];
			bone._transform.tofloat4x4(animationMatrices[i]);
		}

		_skeletonConstantBuffer = renderModule.createUploadBuffer(sizeof(decltype(animationMatrices[0])) * static_cast<uint32>(animationMatrices.size()));

		return true;
	}

	void SkinnedMeshComponent::update(float deltaTime)
	{
		const DKVector<Bone>& bones = _skeleton->GetBones();
		const uint boneCount = static_cast<uint>(bones.size());

		const DKVector<Animation::BoneAnimation>& boneAnimations = _animation->GetBoneAnimation();
		const float currentAnimationTime = fmodf(_animation->GetAnimationTime() + deltaTime * 30, static_cast<const float>(_animation->GetFrameCount()));
		_animation->SetAnimationTime(currentAnimationTime);

		DK_ASSERT_LOG(boneCount == boneAnimations.size(), "Skeleton과 Animation의 Bone Count가 다릅니다.");

		// build InvertMatrix by CharacterSpace Bone DressPose
		DKVector<float4x4> characterSpaceBoneMatrix;
		DKVector<float4x4> characterSpaceInvertBoneMatrix;
		characterSpaceBoneMatrix.resize(boneCount);
		characterSpaceInvertBoneMatrix.resize(boneCount);
		for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			float4x4 outMatrix;
			bones[boneIndex]._transform.tofloat4x4(outMatrix);
			characterSpaceBoneMatrix[boneIndex] = outMatrix;
			if (bones[boneIndex]._parentBoneIndex != 0xffffffff)
				characterSpaceBoneMatrix[boneIndex] = characterSpaceBoneMatrix[boneIndex] * characterSpaceBoneMatrix[bones[boneIndex]._parentBoneIndex];

			characterSpaceBoneMatrix[boneIndex].inverse_copy(characterSpaceInvertBoneMatrix[boneIndex]);
		}

		// build CharacterSpace Animated Bone Matrix
		DKVector<float4x4> currentCharacterSpaceBoneAnimation;
		currentCharacterSpaceBoneAnimation.clear();
		currentCharacterSpaceBoneAnimation.resize(boneCount);
		for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			float4x4 outMatrix = boneAnimations[boneIndex]._animation[static_cast<size_t>(currentAnimationTime)];
			currentCharacterSpaceBoneAnimation[boneIndex] = outMatrix;
			if (bones[boneIndex]._parentBoneIndex != 0xffffffff)
				currentCharacterSpaceBoneAnimation[boneIndex] = currentCharacterSpaceBoneAnimation[boneIndex] * currentCharacterSpaceBoneAnimation[bones[boneIndex]._parentBoneIndex];
		}

#ifdef _DK_DEBUG_
		// T-POSE
		{
			static const float scale = 0.05f;
			EditorDebugDrawManager::getSingleton().addAxis(float4x4::Identity, float3(scale, scale, scale));

			uint32 boneIndex = 0;
			for (const float4x4& characterSpacePosition : characterSpaceBoneMatrix)
			{
				float colorR = static_cast<float>(boneIndex) / characterSpaceBoneMatrix.size();
				EditorDebugDrawManager::getSingleton().addSphere(characterSpacePosition.getTranslation(), float3(colorR, 0, 0), 0.01f);
				EditorDebugDrawManager::getSingleton().addAxis(characterSpacePosition, float3(scale, scale, scale));

				if (bones[boneIndex]._parentBoneIndex != 0xffffffff)
					EditorDebugDrawManager::getSingleton().addLine(
						characterSpacePosition.getTranslation(), 
						characterSpaceBoneMatrix[bones[boneIndex]._parentBoneIndex].getTranslation(), 
						float3(colorR, 0, 0)
					);

				++boneIndex;
			}
		}

		// Inverse Matrix
		{
			//static const float scale = 0.05f;
			//EditorDebugDrawManager::getSingleton().addAxis(float4x4::Identity, float3(scale, scale, scale));

			uint32 boneIndex = 0;
			for (const float4x4& inverseCharacterSpacePosition : characterSpaceInvertBoneMatrix)
			{
				float4x4 characterSpacePosition;
				inverseCharacterSpacePosition.inverse_copy(characterSpacePosition);

				float colorG = static_cast<float>(boneIndex) / characterSpaceInvertBoneMatrix.size();
				EditorDebugDrawManager::getSingleton().addSphere(characterSpacePosition.getTranslation(), float3(0, colorG, 0), 0.01f);
				//EditorDebugDrawManager::getSingleton().addAxis(characterSpacePosition, float3(scale, scale, scale));

				if (bones[boneIndex]._parentBoneIndex != 0xffffffff)
				{
					float4x4 parentCharacterSpacePosition;
					characterSpaceInvertBoneMatrix[bones[boneIndex]._parentBoneIndex].inverse_copy(parentCharacterSpacePosition);

					EditorDebugDrawManager::getSingleton().addLine(
						characterSpacePosition.getTranslation(), 
						parentCharacterSpacePosition.getTranslation(), 
						float3(0, colorG, 0)
					);
				}

				++boneIndex;
			}
		}

		// ANIMATION
		{
			uint32 boneIndex = 0;
			for (const float4x4& characterSpacePosition : currentCharacterSpaceBoneAnimation)
			{
				float colorB = static_cast<float>(boneIndex) / characterSpaceBoneMatrix.size();
				EditorDebugDrawManager::getSingleton().addSphere(characterSpacePosition.getTranslation(), float3(0, 0, colorB), 0.01f);
				if (bones[boneIndex]._parentBoneIndex != 0xffffffff)
					EditorDebugDrawManager::getSingleton().addLine(characterSpacePosition.getTranslation(), currentCharacterSpaceBoneAnimation[bones[boneIndex]._parentBoneIndex].getTranslation(), float3(0, 0, colorB));

				++boneIndex;
			}
		}
#endif

		/*
		* Skinning Matrix 자체는 Vertex단위로 있어야합니다. 이 곳에서는 bone단위의 Invert * Animation을 구하고
		* 셰이더에서 Vertex단위로 weight를 곱해주도록 합니다.
		*/
		// Build Skinning Matrix(Bone)
		_currentCharacterSpaceBoneAnimation.clear();
		_currentCharacterSpaceBoneAnimation.resize(boneCount);
		for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			float4x4 boneSpaceAnimationMatrix = characterSpaceInvertBoneMatrix[boneIndex] * currentCharacterSpaceBoneAnimation[boneIndex];
			_currentCharacterSpaceBoneAnimation[boneIndex] = boneSpaceAnimationMatrix;
		}
	}
}