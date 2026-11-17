#pragma once

#include "Resource/Material/ShaderStructures.h"

namespace DK
{
#ifdef _DK_DEBUG_
	struct IBuffer;

	class EditorDebugDrawManager
	{
	public:

	private:
		static EditorDebugDrawManager* _this;
	public:
		static EditorDebugDrawManager& getSingleton() noexcept
		{
			if (_this == nullptr)
				_this = dk_new EditorDebugDrawManager;

			return *_this;
		}

		bool initialize();

		void prepareShaderData();
		void endUpdateRender();

#define MAX_ELEMENT_COUNT 32768
		dk_inline void addSphere(const float3& worldPosition, const float3& color, const float radius)
		{
			if (_primitiveInfoSphereArr.size() >= MAX_ELEMENT_COUNT)
			{
				DK_ASSERT_LOG(false, "최대 개수를 넘어갑니다. 랜더링 버퍼가 충분하지 않습니다. 건너뜁니다.");
				return;
			}
			_primitiveInfoSphereArr.push_back(SpherePrimitiveInfo(worldPosition, color, radius));
		}
		dk_inline void addLine(const float3& startWorldPosition, const float3& endWorldPosition, const float3& color)
		{
			if (_primitiveInfoLineArr.size() >= MAX_ELEMENT_COUNT)
			{
				DK_ASSERT_LOG(false, "최대 개수를 넘어갑니다. 랜더링 버퍼가 충분하지 않습니다. 건너뜁니다.");
				return;
			}
			_primitiveInfoLineArr.push_back(LinePrimitiveInfo(startWorldPosition, endWorldPosition, color));
		}
		dk_inline void addAxis(const float4x4& worldMatrix, const float3& scale)
		{
			DK_ASSERT_LOG(_primitiveInfoLineArr.size() + 3 <= MAX_ELEMENT_COUNT, "Debug axis requires three line slots");

			static const float3 up(0, 1, 0);
			static const float3 right(1, 0, 0);
			static const float3 forward(0, 0, 1);

			float3 startPosition = worldMatrix.getTranslation();
			float4 upPosition = float4(up * scale.y) * worldMatrix;
			float4 rightPosition = float4(right * scale.x) * worldMatrix;
			float4 forwardPosition = float4(forward * scale.z) * worldMatrix;

			_primitiveInfoLineArr.push_back(LinePrimitiveInfo(startPosition, upPosition.xyz(), float3(0, 1, 0)));
			_primitiveInfoLineArr.push_back(LinePrimitiveInfo(startPosition, rightPosition.xyz(), float3(1, 0, 0)));
			_primitiveInfoLineArr.push_back(LinePrimitiveInfo(startPosition, forwardPosition.xyz(), float3(0, 0, 1)));
		}

	private:
		bool initialize_Sphere();
		bool initialize_Line();

#define DEFINE_ELEMENT_TYPE(primitiveName) \
	DK_REFLECTION_PROPERTY(DKVector<##primitiveName##PrimitiveInfo>, _primitiveInfo##primitiveName##Arr); \
	DK_REFLECTION_PROPERTY(IBufferRef, _primitiveInfo##primitiveName##Buffer);

		DEFINE_ELEMENT_TYPE(Sphere);
		DEFINE_ELEMENT_TYPE(Line);
	};
#endif
}
