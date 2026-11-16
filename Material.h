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
		uint64 _nameHash = 0;
#if defined(_DK_DEBUG_)
		DKString _name;
#endif
		MaterialParameterType _type;
		DKString _value;
		void setName(const char* name)
		{
			_nameHash = StringUtil::registerHash(name);
#if defined(_DK_DEBUG_)
			_name = name;
#endif
		}
	};

	struct MaterialDefinition
	{
		uint64 _materialNameHash = 0;
#if defined(_DK_DEBUG_)
		DKString _materialName;
#endif
		DKVector<MaterialParameterDefinition> _parameters;
		void setMaterialName(const char* name)
		{
			_materialNameHash = StringUtil::registerHash(name);
#if defined(_DK_DEBUG_)
			_materialName = name;
#endif
		}
	};
#pragma endregion

#pragma region Parameter
	class MaterialParameter
	{
	public:
		static MaterialParameter* createMaterialParameter(const MaterialParameterDefinition& parameterDefinition);

	public:
		MaterialParameter(const MaterialParameterDefinition& definition)
			: _nameHash(definition._nameHash)
		{
#if defined(_DK_DEBUG_)
			_name = definition._name;
#endif
		}
		virtual ~MaterialParameter() {}

		dk_inline uint64 getParameterNameHash() const noexcept { return _nameHash; }
#if defined(_DK_DEBUG_)
		dk_inline const DKString& getParameterName() const noexcept
		{
			return _name;
		}
#endif

		dk_inline void setParameterValuePtr(void* valuePtr) noexcept
		{
			_valuePtr = valuePtr;
		}

		virtual MaterialParameterType getType() const noexcept = 0;
		virtual uint32 getParameterSize() const noexcept = 0;

	protected:
		uint64 _nameHash = 0;
#if defined(_DK_DEBUG_)
		DKString _name;
#endif
		void* _valuePtr;
	};

	template <typename T>
	class MaterialParameterTemplate : public MaterialParameter
	{
	public:
		MaterialParameterTemplate(const MaterialParameterDefinition& definition, const T& defaultValue)
			: MaterialParameter(definition)
			, _value(defaultValue)
			, _defaultValue(defaultValue)
		{}
		virtual ~MaterialParameterTemplate() override final
		{}

		dk_inline virtual MaterialParameterType getType() const noexcept override;
		dk_inline virtual uint32 getParameterSize() const noexcept override
		{
			return sizeof(T);
		}

		void setParameterValue(const T& value) noexcept;

		T _value;				// ITexture의 RefCount 관리를 위해서 들고 있어야합니다.
	private:
		const T _defaultValue;
	};

	using MaterialParameterFloat = MaterialParameterTemplate<float>;
	using MaterialParameterTexture = MaterialParameterTemplate<ITextureRef>;
#pragma endregion

#pragma region Material
	class Material
	{
	public:
		static Material* createMaterial(uint64 materialNameHash);
		static Material* createMaterial(const DKString& materialName) { return createMaterial(StringUtil::registerHash(materialName)); }
		static Material* createMaterial(const char* materialName) { return createMaterial(StringUtil::registerHash(materialName)); }
		static Material* createMaterial(const MaterialDefinition& modelProperty);

	public:
		Material() {};
		Material(Material&& rhs)
		{
			_materialNameHash = rhs._materialNameHash;
#if defined(_DK_DEBUG_)
			_materialName = DK::move(rhs._materialName);
#endif
			_parameterArr.swap(rhs._parameterArr);
			_parameterBufferForCPU.swap(rhs._parameterBufferForCPU);
			_parameterBufferForGPU.swap(rhs._parameterBufferForGPU);
		}

	public:
		bool setModelProperty(const MaterialDefinition& modelProperty);

		Material* clone() const;

		// TODO: private 로 내릴것
		DK_REFLECTION_PROPERTY(uint64, _materialNameHash);
#if defined(_DK_DEBUG_)
		DK_REFLECTION_PROPERTY(DKString, _materialName);
#endif
		DKVector<Ptr<MaterialParameter>> _parameterArr;
		DKVector<char> _parameterBufferForCPU;
		DK_REFLECTION_PROPERTY_FLAG(IBufferRef, _parameterBufferForGPU, ReflectionFlag::NoSave);
	private:

	};
#pragma endregion
}
