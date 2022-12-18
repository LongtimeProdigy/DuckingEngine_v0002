#pragma once

namespace DK
{
	struct IBuffer;
}

namespace DK
{
#pragma region Definition
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

	static_assert(static_cast<uint32>(MaterialParameterType::COUNT) != (DK_COUNT_OF(gMaterialParameterTypeName) - 1), "Type과 TypeName 개수가 일치되어야합니다.");
	dk_inline MaterialParameterType convertStringToEnum(const char* str)
	{
		const uint32 count = static_cast<uint32>(MaterialParameterType::COUNT);
		for (uint32 i = 0; i < count; ++i)
		{
			if (_stricmp(str, gMaterialParameterTypeName[i]) == 0)
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
		DKString _value;
	};

	struct MaterialDefinition
	{
		DKString _materialName;
		DKVector<MaterialParameterDefinition> _parameters;
	};
#pragma endregion

#pragma region Parameter
	class MaterialParameter
	{
	public:
		static MaterialParameter* createMaterialParameter(const MaterialParameterDefinition& parameterDefinition);

	public:
		MaterialParameter(const DKString& name)
			: _name(name)
		{}
		virtual ~MaterialParameter() {}

		dk_inline const DKString& getName() const noexcept
		{
			return _name;
		}

		dk_inline virtual void setValuePtr(void* valuePtr) noexcept
		{
			_valuePtr = valuePtr;
		}

		dk_inline virtual uint32 getSize() const noexcept = 0;

	protected:
		DKString _name;
		void* _valuePtr;
	};

	template <typename T>
	class MaterialParameterTemplate : public MaterialParameter
	{
	public:
		MaterialParameterTemplate(const DKString& name, const T& defaultValue)
			: MaterialParameter(name)
			, _defaultValue(defaultValue)
		{}

		virtual void setValue(const T& value) noexcept;

		dk_inline virtual uint32 getSize() const noexcept
		{
			return sizeof(T);
		}

	private:
		const T _defaultValue;
		T _value;				// ITexture의 RefCount 관리를 위해서 들고 있어야합니다.
	};

	using MaterialParameterFloat = MaterialParameterTemplate<float>;
	using MaterialParameterTexture = MaterialParameterTemplate<ITextureRef>;

	void MaterialParameterTexture::setValue(const ITextureRef& value) noexcept;
	template <>
	uint32 MaterialParameterTexture::getSize() const noexcept;
#pragma endregion

#pragma region Material
	class Material
	{
	public:
		static bool createMaterial(const MaterialDefinition& modelProperty, Material& outMaterial);

	public:
		Material() {};
		Material(const DKString& materialname)
			: _materialName(materialname)
		{};
		Material(Material&& rhs)
		{
			_materialName = rhs._materialName;
			_parameters.swap(rhs._parameters);
			_parameterBufferForCPU.swap(rhs._parameterBufferForCPU);
			_parameterBufferForGPU.swap(rhs._parameterBufferForGPU);
		}
		~Material() {}

	public:
		bool setModelProperty(const MaterialDefinition& modelProperty);

	private:
		DK_REFLECTION_DECLARE(DKString, _materialName);
		DKVector<Ptr<MaterialParameter>> _parameters;

		DKVector<char> _parameterBufferForCPU;
		DK_REFLECTION_PTR_DECLARE_FLAG(IBuffer, _parameterBufferForGPU, ReflectionFlag::NoSave);
	};
#pragma endregion
}