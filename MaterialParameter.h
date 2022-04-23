#pragma once

class ITexture;

class IMaterialParameter
{
public:
	IMaterialParameter() = default;
	virtual ~IMaterialParameter() {}
};

template <typename T>
class MaterialParameter : public IMaterialParameter
{
public:
	typedef T SelfType;

public:
	MaterialParameter(const T& lvalue)
		: _value(lvalue)
	{}
	MaterialParameter(T&& rvalue)
		: _value(std::move(rvalue))
	{}
	virtual ~MaterialParameter() {}

	T GetValue() const noexcept
	{
		return _value;
	}
	void SetValue(const T& value) const noexcept
	{
		_value = value;
	}

private:
	T _value;
};

using MaterialParameterInt = MaterialParameter<int>;
using MaterialParameterTexture = MaterialParameter<ITexture>;