#pragma once

enum class MaterialParameterType : uint8
{
	INT, 
	UINT, 
	FLOAT,
	TEXTURE,
	COUNT
};
constexpr static const char* gMaterialParameterTypeName[static_cast<uint32>(MaterialParameterType::COUNT)]
{
	"int", 
	"uint", 
	"float", 
	"Texture",
};
static_assert(static_cast<uint32>(MaterialParameterType::COUNT) != (ARRAYSIZE(gMaterialParameterTypeName) - 1), "Type�� TypeName ������ ��ġ�Ǿ���մϴ�.");
dk_inline MaterialParameterType convertNameToEnum(const char* name)
{
	const uint32 count = ARRAYSIZE(gMaterialParameterTypeName);
	for (uint i = 0; i < count; ++i)
	{
		if (strcmp(name, gMaterialParameterTypeName[i]) == 0)
		{
			return static_cast<MaterialParameterType>(i);
		}
	}

	return MaterialParameterType::COUNT;
}

struct MaterialParameterDefinition
{
	DKString _name;
	MaterialParameterType _type;
	uint32 _register;	// RenderPass������ ���̴� Value�Դϴ�.
	Ptr<void> _value;	// RenderPass������ DefaultValue�Դϴ�.	// ModelProperty������ ���� Value�Դϴ�.
};

class MaterialParameterFloat;
class MaterialParameterTexture;
class ITexture;

class MaterialParameter
{
public:
	static MaterialParameter* createMaterialParameter(const MaterialParameterDefinition& parameterDefinition);

public:
	MaterialParameter(const DKString& name)
		: _name(name)
	{}
	virtual ~MaterialParameter() = default;

	dk_inline const DKString& getName() const noexcept
	{
		return _name;
	}

	dk_inline virtual void setValuePtr(void* valuePtr) noexcept
	{
		_valuePtr = valuePtr;
	}

	dk_inline virtual void setValue(void* value) noexcept = 0;
	dk_inline virtual uint32 getSize() const noexcept = 0;

protected:
	DKString _name;
	void* _valuePtr;
};

template <typename T>
class MaterialParameterTemplate : public MaterialParameter
{
public:
	MaterialParameterTemplate(const DKstring& name, const uint32 reg, const T& defaultValue)
		: MaterialParameter(name, reg)
		, _defaultValue(defaultValue)
	{}

	dk_inline virtual void setValue(void* value) noexcept override
	{
		DK_ASSERT_LOG(value != nullptr, "value�� nullptr�Դϴ�. ũ���ð� �� ���Դϴ�.");
		memcpy(setValuePtr, value, sizeof(T));
	}

	dk_inline virtual uint32 getSize() const noexcept
	{
		return sizeof(T);
	}

private:
	const T _defaultValue;
	T _value;				// ITexture�� RefCount ������ ���ؼ� ��� �־���մϴ�.
};

using MaterialParameterFloat = MaterialParameterTemplate<float>;
using MaterialParameterTexture = MaterialParameterTemplate<ITextureRef>;

template <>
void MaterialParameterTexture::setValue(void* value) noexcept;
template <>
uint32 MaterialParameterTexture::getSize() const noexcept;