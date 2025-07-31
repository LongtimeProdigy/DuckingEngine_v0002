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

		const uint32 boneCount = static_cast<uint32>(_skeleton->getBoneArr().size());
		DKVector<float4x4> animationMatrices;
		animationMatrices.resize(boneCount);
		for (uint32 i = 0; i < boneCount; ++i)
		{
			const Bone& bone = _skeleton->getBoneArr()[i];
			bone._transform.tofloat4x4(animationMatrices[i]);
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		_skeletonConstantBuffer = renderModule.createUploadBuffer(sizeof(decltype(animationMatrices[0])) * static_cast<uint32>(animationMatrices.size()), L"SkinnedMesh_Skeleton_Cbuffer");
		if (_skeletonConstantBuffer.get() == nullptr)
			return false;

		return true;
	}

	void SkinnedMeshComponent::update(float deltaTime)
	{
		const DKVector<Bone>& boneArr = _skeleton->getBoneArr();
		const uint32 boneCount = static_cast<uint32>(boneArr.size());

		const float totalAnimationTime = _animation->getFrameCount() * (Animation::kAnimationTimePerFrame);
		const float currentAnimationTime = fmodf(_animation->getCurrentAnimationTime() + deltaTime, totalAnimationTime);
		_animation->setCurrentAnimationTime(currentAnimationTime);

		const DKVector<Animation::BoneAnimation>& boneAnimations = _animation->getBoneAnimation();
		DK_ASSERT_LOG(boneCount == boneAnimations.size(), "Skeleton과 Animation의 BoneCount는 일치해야합니다.");

		// build InvertMatrix by CharacterSpace Bone DressPose
		DKVector<float4x4> characterSpaceBoneMatrix;
		DKVector<float4x4> characterSpaceInvertBoneMatrix;
		characterSpaceBoneMatrix.resize(boneCount);
		characterSpaceInvertBoneMatrix.resize(boneCount);
		for (uint32 boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			float4x4 outMatrix;
			boneArr[boneIndex]._transform.tofloat4x4(outMatrix);
			characterSpaceBoneMatrix[boneIndex] = outMatrix;
			if (boneArr[boneIndex]._parentBoneIndex != Bone::kInvalidBoneIndex)
				characterSpaceBoneMatrix[boneIndex] = characterSpaceBoneMatrix[boneIndex] * characterSpaceBoneMatrix[boneArr[boneIndex]._parentBoneIndex];

			characterSpaceBoneMatrix[boneIndex].inverse_copy(characterSpaceInvertBoneMatrix[boneIndex]);
		}

		const uint32 frameIndex = static_cast<uint32>(currentAnimationTime / Animation::kAnimationTimePerFrame);
		const uint32 nextFrameIndex = frameIndex == (_animation->getFrameCount() - 1) ? 0 : frameIndex + 1;
		const float lerp = (currentAnimationTime - frameIndex * Animation::kAnimationTimePerFrame) / Animation::kAnimationTimePerFrame;

		// build CharacterSpace Animated Bone Matrix
		DKVector<float4x4> currentCharacterSpaceBoneAnimation;
		currentCharacterSpaceBoneAnimation.clear();
		currentCharacterSpaceBoneAnimation.resize(boneCount);
		for (uint32 boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			float4x4 animationMatrix1 = boneAnimations[boneIndex]._animation[frameIndex];
			float4x4 animationMatrix2 = boneAnimations[boneIndex]._animation[nextFrameIndex];
			currentCharacterSpaceBoneAnimation[boneIndex] = animationMatrix1; // float4x4::slerp(animationMatrix1, animationMatrix2, lerp);
			if (boneArr[boneIndex]._parentBoneIndex != Bone::kInvalidBoneIndex)
				currentCharacterSpaceBoneAnimation[boneIndex] = currentCharacterSpaceBoneAnimation[boneIndex] * currentCharacterSpaceBoneAnimation[boneArr[boneIndex]._parentBoneIndex];
		}

		// Build Skinning Matrix(Bone)
		_currentCharacterSpaceBoneAnimation.clear();
		_currentCharacterSpaceBoneAnimation.resize(boneCount);
		for (uint32 boneIndex = 0; boneIndex < boneCount; ++boneIndex)
		{
			_currentCharacterSpaceBoneAnimation[boneIndex] = characterSpaceInvertBoneMatrix[boneIndex] * currentCharacterSpaceBoneAnimation[boneIndex];
		}
	}
}
