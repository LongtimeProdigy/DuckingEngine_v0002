#pragma once

#ifdef _DK_DEBUG_
struct IBuffer;

struct float3;

class EditorDebugDrawManager
{
public:
	struct SpherePrimitiveInfo
	{
		static VertexBufferViewRef kVertexBufferView;
		static IndexBufferViewRef kIndexBufferView;
		static uint32 indexCount;

		SpherePrimitiveInfo()
			: _worldPosition(float3::Zero)
			, _color(float3::Zero)
			, _radius(1.0f)
		{}
		SpherePrimitiveInfo(const float3& worldPosition, const float3& color, const float& radius)
			: _worldPosition(worldPosition)
			, _color(color)
			, _radius(radius)
		{}

		float3 _worldPosition;
		float _radius = 1.0f;	// _radius를 _color 밑으로내리면 class alignment로 인해서 data upload시에 값이 틀어짐
		float3 _color;
		float _padding = 1.0f;
	};

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

private:
	bool initialize_Sphere();

public:
#define MAX_ELEMENT_COUNT 1024
	dk_inline void addSphere(const float3& worldPosition, const float3& color, const float radius)
	{
		DK_ASSERT_LOG(_spherePrimitiveInfoArr.size() < MAX_ELEMENT_COUNT, "최대 개수를 넘어갑니다. 랜더링 버퍼가 충분하지 않습니다.");
		_spherePrimitiveInfoArr.push_back(SpherePrimitiveInfo(worldPosition, color, radius));
	}
	dk_inline const DKVector<SpherePrimitiveInfo>& getSpherePrimitiveInfo() const noexcept
	{
		return _spherePrimitiveInfoArr;
	}
	dk_inline const Ptr<IBuffer>& getSphereDebugDrawElementBuffer() const noexcept
	{
		return _sphereDebugDrawElementBuffer;
	}
	dk_inline Ptr<IBuffer>& getSphereDebugDrawElementBufferWritable() noexcept
	{
		return _sphereDebugDrawElementBuffer;
	}

private:
	DKVector<SpherePrimitiveInfo> _spherePrimitiveInfoArr;
	Ptr<IBuffer> _sphereDebugDrawElementBuffer;
};
#endif