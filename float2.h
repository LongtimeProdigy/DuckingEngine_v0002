#pragma once
struct float2
{
	static const float2 Identity;
	static const float2 Zero;

	float2()
		: m{ 0, 0 }
	{}

	float2(float _x, float _y)
		: m{ _x, _y }
	{}

	float2 operator-(const float2& rhs)
	{
		return float2(x - rhs.x, y - rhs.y);
	}

public:
	union
	{
		float m[2];
		struct
		{
			float x, y;
		};
	};
};