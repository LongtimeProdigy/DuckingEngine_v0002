#pragma once

struct float3;

class EditorDebugDrawManager
{
private:
	static EditorDebugDrawManager* _this;
public:
	static EditorDebugDrawManager& getSingleton() noexcept
	{
		if (_this == nullptr)
			_this = dk_new EditorDebugDrawManager;

		return *_this;
	}

public:
	void initialize() const;

	void drawCube(const float3& worldPosition, const float radius) const;
};