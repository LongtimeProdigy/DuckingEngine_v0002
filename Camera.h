#pragma once

#include "Object.h"

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

		void getCameraWorldMatrix(float4x4& outMatrix)
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
		void getCameraProjectionMatrix(float4x4& outMaterix)
		{
			//outMaterix.Identity;

			// https://marmelo12.tistory.com/338 (h == 1 이라는 가정하게 설명하지만 더 자세하게 설명해줌)
			// https://jw910911.tistory.com/19
			// DX에서 투영행렬은 (-1~1, -1~1, 0~1) 까지의 매핑작업을 의미함
			// 결과는 w가 z인 동차좌표계가 나오도록 행렬을 구성해야함 (진짜 z위치를 알기 위해서는 z/w를 해야한다는 소리)
#if 0	// noOptimize
			const float aspect = static_cast<float>(_width) / static_cast<float>(_height);
			const float h = _nearPlaneDistance * tanf(_halfFov);
			const float w = h * aspect;
			outMaterix._11 = _nearPlaneDistance / w;
			outMaterix._22 = _nearPlaneDistance / h;
#else	// optimized
			const float h = 1.0f / tanf(_halfFov);
			const float w = h * (static_cast<float>(_height) / static_cast<float>(_width));
			outMaterix._11 = w;
			outMaterix._22 = h;
#endif

			const float a = _farPlaneDistance / (_farPlaneDistance - _nearPlaneDistance);
			const float b = -a * _nearPlaneDistance;
			outMaterix._33 = a;
			outMaterix._43 = b;

			outMaterix._34 = 1.0f;
			outMaterix._44 = 0.0f;
		}

		dk_inline float getNearPlaneDistance() const { return _nearPlaneDistance; }
		dk_inline float getFarPlaneDistance() const { return _farPlaneDistance; }

	private:
		float _halfFov = 45.0f * Math::kToRadian;
		int _width = 1920;
		int _height = 1080;

		float _nearPlaneDistance = 0.1f;
		float _farPlaneDistance = 1000.0f;
	};
}
