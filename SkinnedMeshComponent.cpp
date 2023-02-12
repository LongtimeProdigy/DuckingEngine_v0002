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
	bool SkinnedMeshComponent::loadResource()
	{
		ResourceManager& resourceManager = DuckingEngine::getInstance().GetResourceManagerWritable();

		const ModelPropertyRef modelProperty = resourceManager.loadModelProperty(_modelPropertyPath);
		if (modelProperty == nullptr)
			return false;

		_model = resourceManager.loadSkinnedMesh(_modelPath, modelProperty);
		if (_model == nullptr)
			return false;
		if (resourceManager.loadSkeleton(_skeletonPath, _model, _skeleton) == false) 
			return false;
		if (resourceManager.loadAnimation(_animationPath, _skeleton, _animation) == false) 
			return false;

		const uint32 boneCount = static_cast<uint32>(_skeleton->GetBones().size());
		DKVector<float4x4> animationMatrices;
		animationMatrices.resize(boneCount);
		for (uint32 i = 0; i < boneCount; ++i)
		{
			const Bone& bone = _skeleton->GetBones()[i];
			bone._transform.tofloat4x4(animationMatrices[i]);
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		_skeletonConstantBuffer = renderModule.createUploadBuffer(sizeof(decltype(animationMatrices[0])) * static_cast<uint32>(animationMatrices.size()));
		if (_skeletonConstantBuffer.get() == nullptr)
			return false;

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

		// Build Skinning Matrix(Bone)
		_currentCharacterSpaceBoneAnimation.clear();
		_currentCharacterSpaceBoneAnimation.resize(boneCount);
		for (uint boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			_currentCharacterSpaceBoneAnimation[boneIndex] = characterSpaceInvertBoneMatrix[boneIndex] * currentCharacterSpaceBoneAnimation[boneIndex];
		}
	}
}