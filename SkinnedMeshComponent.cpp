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
		_animation.reset();
		ResourceManager& resourceManager = DuckingEngine::getInstance().GetResourceManagerWritable();

		const ModelPropertyRef modelProperty = resourceManager.loadModelProperty(_modelPropertyPath);
		if (modelProperty == nullptr)
			return false;

		_model = resourceManager.loadSkinnedMesh(_modelPath, modelProperty);
		if (_model == nullptr)
			return false;
		if (resourceManager.loadSkeleton(_skeletonPath, _model, _skeleton) == false) 
			return false;

		AnimationRef clip;
		if (resourceManager.loadAnimation(_animationPath, clip) == false)
			return false;

		const DKVector<Bone>& bones = _skeleton->getBoneArr();
		const DKVector<Animation::BoneAnimation>& tracks = clip->getBoneAnimation();
		if (bones.empty() || bones.size() > Bone::kMaxBoneCount || bones.size() != tracks.size())
			return false;

		_trackIndices.resize(bones.size());
		_inverseBindPose.resize(bones.size());
		_animatedPose.resize(bones.size());
		for (size_t i = 0; i < bones.size(); ++i)
		{
			const Bone& bone = bones[i];
			const auto track = std::find_if(tracks.begin(), tracks.end(), [&](const Animation::BoneAnimation& candidate) {
				return candidate._boneName == bone._boneName;
			});
			if (track == tracks.end())
				return false;

			_trackIndices[i] = static_cast<uint32>(track - tracks.begin());
			bone._transform.tofloat4x4(_animatedPose[i]);
			if (bone._parentBoneIndex != Bone::kInvalidBoneIndex)
			{
				if (bone._parentBoneIndex >= i)
					return false;

				_animatedPose[i] = _animatedPose[i] * _animatedPose[bone._parentBoneIndex];
			}
			_animatedPose[i].inverse_copy(_inverseBindPose[i]);
		}
		_currentCharacterSpaceBoneAnimation.assign(Bone::kMaxBoneCount, float4x4::Identity);

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		_currentAnimationBufferCS = renderModule.createConstantBuffer(sizeof(float4x4) * Bone::kMaxBoneCount, L"SkinnedMesh_Skeleton_Cbuffer");
		if (_currentAnimationBufferCS == nullptr)
			return false;

		_animation = DK::move(clip);
		_animationTime = 0.0;
		update(0.0f);
		return true;
	}

	void SkinnedMeshComponent::update(float deltaTime)
	{
		if (_animation == nullptr || std::isfinite(deltaTime) == false || deltaTime < 0.0f)
			return;

		const DKVector<Bone>& bones = _skeleton->getBoneArr();
		const DKVector<Animation::BoneAnimation>& tracks = _animation->getBoneAnimation();
		const uint32 frameCount = _animation->getFrameCount();
		const double fps = Animation::kAnimationFrameCountPerSecond;
		_animationTime = std::fmod(_animationTime + deltaTime, frameCount / fps);
		const double framePosition = _animationTime * fps;
		const uint32 frame = (std::min)(static_cast<uint32>(framePosition), frameCount - 1);
		const uint32 nextFrame = frame + 1 == frameCount ? 0 : frame + 1;
		const float blend = static_cast<float>((std::min)(framePosition - frame, 1.0));
		for (size_t i = 0; i < bones.size(); ++i)
		{
			const DKVector<float4x4>& frames = tracks[_trackIndices[i]]._animation;
			_animatedPose[i] = float4x4::slerp(frames[frame], frames[nextFrame], blend);
			if (bones[i]._parentBoneIndex != Bone::kInvalidBoneIndex)
				_animatedPose[i] = _animatedPose[i] * _animatedPose[bones[i]._parentBoneIndex];

			_currentCharacterSpaceBoneAnimation[i] = _inverseBindPose[i] * _animatedPose[i];
		}
	}
}
