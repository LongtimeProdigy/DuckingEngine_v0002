#pragma once

#include "Object.h"
#include <limits>

namespace DK
{
	struct Transform;
	class InputModule;
}

namespace DK
{
	class Camera : public Object
	{
	public:
		static Camera* gMainCamera;

	public:
		Camera(const float fov, const int width, const int height)
			: _halfFov((fov / 2.0f)* Math::kToRadian)
			, _width(width)
			, _height(height)
		{}

		virtual void update(float deltaTime) override final;

#if defined(_DK_DEBUG_)
		bool loadViewState();
		bool saveViewState();
#endif

		void setView(const float3& position, const float3& direction)
		{
			float3 forward = direction;
			forward.normalize();
			_yaw = Math::atan2(forward.x, forward.z);
			_pitch = -Math::asin(Math::clamp(forward.y, -1.0f, 1.0f));
			set_worldTransform(Transform(position, Quaternion(0.0f, _pitch, _yaw), float3::Identity));
			setWorldPosition(position.x, position.y, position.z);
		}

		void setLevelView(const float3& position, const float3& direction)
		{
			setView(position, direction);
#if defined(_DK_DEBUG_)
			_saveViewState = false;
#endif
		}

		void setWorldPosition(double x, double y, double z);
		const DKArray<double, 3>& getWorldPosition();

		void setMoveSpeed(float speed)
		{
			DK_ASSERT_LOG(std::isfinite(speed) && speed >= 0.0f, "Invalid camera move speed");
			_moveSpeed = speed;
		}

		void setClipPlaneDistances(float nearDistance)
		{
			DK_ASSERT_LOG(std::isfinite(nearDistance) && nearDistance > 0.0f, "Invalid camera near plane distance");
			_nearPlaneDistance = nearDistance;
		}

		void setViewportSize(int width, int height)
		{
			DK_ASSERT_LOG(width > 0 && height > 0, "Invalid camera viewport size");
			_width = width;
			_height = height;
		}

		void getCameraWorldMatrix(float4x4& outMatrix) const
		{
			Transform invertTransform = get_worldTransform();
			invertTransform.invert();
			invertTransform.tofloat4x4(outMatrix);

			float4x4 translationMatrix(
				1, 0, 0, 0,	0, 1, 0, 0, 0, 0, 1, 0,
				invertTransform.get_translation().x, invertTransform.get_translation().y, invertTransform.get_translation().z, 1
			);
			float4x4 rotationMatrix;
			invertTransform.get_rotation().toFloat4x4(rotationMatrix);

			outMatrix = translationMatrix * rotationMatrix;
		}
		void getCameraProjectionMatrix(float4x4& outMatrix) const
		{
			outMatrix = float4x4::Identity;

			// https://marmelo12.tistory.com/338 (h == 1 이라는 가정하게 설명하지만 더 자세하게 설명해줌)
			// https://jw910911.tistory.com/19
			// DX에서 투영행렬은 (-1~1, -1~1, 0~1) 까지의 매핑작업을 의미함
			// 결과는 w가 z인 동차좌표계가 나오도록 행렬을 구성해야함 (진짜 z위치를 알기 위해서는 z/w를 해야한다는 소리)
#if 0	// noOptimize
			const float aspect = static_cast<float>(_width) / static_cast<float>(_height);
			const float h = _nearPlaneDistance * tanf(_halfFov);
			const float w = h * aspect;
			outMatrix._11 = _nearPlaneDistance / w;
			outMatrix._22 = _nearPlaneDistance / h;
#else	// optimized
			const float h = 1.0f / tanf(_halfFov);
			const float w = h * (static_cast<float>(_height) / static_cast<float>(_width));
			outMatrix._11 = w;
			outMatrix._22 = h;
#endif

			outMatrix._33 = 0.0f;
			outMatrix._43 = _nearPlaneDistance;

			outMatrix._34 = 1.0f;
			outMatrix._44 = 0.0f;
		}

		dk_inline float getNearPlaneDistance() const { return _nearPlaneDistance; }

	private:
#if defined(_DK_DEBUG_)
		bool _saveViewState = true;
#endif
		DKArray<double, 3> _worldPosition = {};
		float3 _lastPosition = float3::Zero;
		float _halfFov = 45.0f * Math::kToRadian;
		int _width = 1920;
		int _height = 1080;

		float _nearPlaneDistance = 0.1f;

		/*	Component의 Transform을 직접 쓰지 않는 이유
		*	- Transform.get_rotation으로 얻어온 quaternion이 pitch가 90도 근처에 있을 때, toEuler를 하는 순간 짐벌락같이 Yaw, Roll이 특정하기 어려운 순간이 온다.
		*	- 따라서 카메라의 경우 yaw, pitch를 직접 관리하여 clamp등에 사용하기 용이하도록 한다
		*/
		float _yaw = 0.0f;
		float _moveSpeed = 1.0f;
		float _pitch = 0.0f;
	};
}
