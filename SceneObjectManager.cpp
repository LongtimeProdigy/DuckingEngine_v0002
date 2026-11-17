#include "stdafx.h"
#include "SceneObjectManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

#include "SceneObject.h"
#include "FileUtil.h"
#include "StaticMeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "Skeleton.h"

#define USE_TINY_GLTF
#if defined(USE_TINY_GLTF)
#define _CRT_SECURE_NO_WARNINGS
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/tinyGLTF/tiny_gltf.h"
#include "Model.h"
#include "SceneRenderer.h"
#endif

namespace DK
{
	const bool createSceneObjectConstantBuffer(SceneObject& sceneObject)
	{
		// for GPU resource
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		SceneObjectConstantBuffer sceneObjectConstantBufferData;
		sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
		sceneObject._sceneObjectConstantBuffer = renderModule.createConstantBuffer(sizeof(sceneObjectConstantBufferData), DK_DEBUG_NAME(L"SceneObject_Cbuffer"));
		if (sceneObject._sceneObjectConstantBuffer == nullptr)
		{
			DK_ASSERT_LOG(false, "SceneObjectConstantBuffer 생성에 실패");
			return false;
		}

		return true;
	}

	template <typename T>
	static T ReadUnaligned(const uint8_t* src)
	{
		T value{};
		DK::memcpy(&value, src, sizeof(T));
		return value;
	}
	template <typename T>
	struct CpuPrimitive
	{
#if defined(_DK_DEBUG_)
		DKString _name;
#endif

		// 실질적인 VertexBuffer / IndexBuffer 데이터.
		DKVector<T> _vertices;
		DKVector<uint32_t> _indices;

		// CpuModel::materials 인덱스.
		// 0은 glTF primitive에 material이 없을 때 사용하는 기본 material.
		uint32_t _materialIndex = 0;

		// 원래 glTF mesh / primitive 위치
		uint32_t _sourceMeshIndex = 0;
		uint32_t _sourcePrimitiveIndex = 0;
	};
	SceneObject* SceneObjectManager::loadGLTF(const char* path)
	{
		DKVector<StaticMeshModelRef> models;
		if (loadStaticMeshModels(path, models) == false)
			return nullptr;

		SceneObject object;
		for (const StaticMeshModelRef& model : models)
		{
			StaticMeshComponent* component = dk_new StaticMeshComponent;
			object.addComponent(component);
			component->set_modelPath(path);
			component->set_modelPropertyPath("GLTF_ModelProperty");
			component->set_model(model);
		}
		return publish(DK::move(object), _sceneObjectContainer);
	}

	bool SceneObjectManager::loadStaticMeshPlacements(const char* path)
	{
		const ScopeString<DK_MAX_PATH> fullPath = GlobalPath::makeResourceFullPath(path);
		FileReader reader;
		DKString text;
		if (reader.open(fullPath.c_str()) == false || reader.readText(text) == false)
		{
			DK_ASSERT_LOG(false, "Static mesh placement file load failed: %s", fullPath.c_str());
			return false;
		}

		TiXmlDocument document;
		document.Parse(text.c_str());
		const TiXmlElement* root = document.RootElement();
		if (document.Error() || root == nullptr || StringUtil::strcmp(root->Value(), "StaticMeshPlacements") != 0)
		{
			DK_ASSERT_LOG(false, "Invalid static mesh placement file: %s", fullPath.c_str());
			return false;
		}

		for (const TiXmlElement* model = root->FirstChildElement("Model"); model != nullptr; model = model->NextSiblingElement("Model"))
		{
			const char* modelPath = model->Attribute("Path");
			DKVector<Transform> transforms;
			for (const TiXmlElement* instance = model->FirstChildElement("Instance"); instance != nullptr; instance = instance->NextSiblingElement("Instance"))
			{
				float3 position;
				float yaw = 0.0f;
				float scale = 1.0f;
				if (instance->QueryFloatAttribute("X", &position.x) != TIXML_SUCCESS || instance->QueryFloatAttribute("Y", &position.y) != TIXML_SUCCESS || instance->QueryFloatAttribute("Z", &position.z) != TIXML_SUCCESS ||
				    instance->QueryFloatAttribute("Yaw", &yaw) != TIXML_SUCCESS || instance->QueryFloatAttribute("Scale", &scale) != TIXML_SUCCESS ||
				    std::isfinite(position.x) == false || std::isfinite(position.y) == false || std::isfinite(position.z) == false || std::isfinite(yaw) == false || std::isfinite(scale) == false || scale <= 0.0f)
				{
					DK_ASSERT_LOG(false, "Invalid static mesh instance transform: %s", fullPath.c_str());
					return false;
				}
				transforms.emplace_back(position, Quaternion(0.0f, 0.0f, yaw), float3(scale, scale, scale));
			}
			if (modelPath == nullptr || transforms.empty())
			{
				DK_ASSERT_LOG(false, "Static mesh placement needs a model and instances: %s", fullPath.c_str());
				return false;
			}
			const ScopeString<DK_MAX_PATH> modelFullPath = GlobalPath::makeResourceFullPath(modelPath);
			DKVector<StaticMeshModelRef> models;
			if (loadStaticMeshModels(modelFullPath.c_str(), models) == false || createStaticMeshInstances(models, transforms) == false)
				return false;
		}
		return true;
	}

	bool SceneObjectManager::loadStaticMeshModels(const char* path, DKVector<StaticMeshModelRef>& models)
	{
		tinygltf::TinyGLTF loader;
		tinygltf::Model gltfModel;

		DKString error;
		DKString warning;

		DKString extension = StringUtil::extension(path);
		StringUtil::lower(extension);

		bool loaded = false;
		if (extension == ".glb")
			loaded = loader.LoadBinaryFromFile(&gltfModel, &error, &warning, path);
		else if (extension == ".gltf")
			loaded = loader.LoadASCIIFromFile(&gltfModel, &error, &warning, path);
		else
		{
			DK_ASSERT_LOG(false, "Unsupported glTF file extension: %s", extension.c_str());
			return false;
		}
		if (loaded == false)
		{
			DK_ASSERT_LOG(false, "glTF file load failed: %s", error.c_str());
			return false;
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

		DKVector<ITextureRef> textures;
		textures.resize(gltfModel.textures.size());
		DKVector<bool> baseColorTextures(gltfModel.textures.size(), false);
		for (const tinygltf::Material& material : gltfModel.materials)
		{
			const int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
			if (textureIndex >= 0 && static_cast<size_t>(textureIndex) < baseColorTextures.size())
				baseColorTextures[textureIndex] = true;
		}
		for (size_t i = 0; i < gltfModel.textures.size(); ++i)
		{
			const tinygltf::Texture& srcTexture = gltfModel.textures[i];
			const int imageIndex = srcTexture.source;
			if (imageIndex < 0 || imageIndex >= static_cast<int>(gltfModel.images.size()))
			{
				DK_ASSERT_LOG(false, "Invalid image index.");
				return false;
			}

			const tinygltf::Image& src = gltfModel.images[imageIndex];
			if (src.width <= 0 || src.height <= 0)
			{
				DK_ASSERT_LOG(false, "Invalid image size.");
				return false;
			}
			if (src.bits != 8)
			{
				DK_ASSERT_LOG(false, "Only 8-bit images are supported.");
				return false;
			}
			if (src.component < 1 || src.component > 4)
			{
				DK_ASSERT_LOG(false, "Invalid image channel count.");
				return false;
			}

			DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
			switch (src.component)
			{
			case 1:
				format = DXGI_FORMAT_R8_UNORM;
				break;
			case 2:
				format = DXGI_FORMAT_R8G8_UNORM;
				break;
			case 4:
				format = baseColorTextures[i] ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			default:
			{
				DK_ASSERT_LOG(false, "Unsupported image channel count.");
				return false;
			}
			}

			textures[i] = renderModule.createTexture(DK_DEBUG_NAME(src.uri.c_str()), src.width, src.height, src.image.data(), format, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, false);
			if (textures[i] == nullptr)
				return false;
		}

		// Material 추출
		struct TextureBinding
		{
			// CpuModel::textures 인덱스. 없으면 -1.
			int _textureIndex = -1;
			// glTF material textureInfo.texCoord 값. 이 샘플 Vertex는 uv0, uv1만 보관합니다.
			int _texCoord = 0;
		};
		struct CpuMaterial
		{
#if defined(_DK_DEBUG_)
			DKString _name;
#endif

			// glTF metallic-roughness PBR
			float4 _baseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
			float _metallicFactor = 1.0f;
			float _roughnessFactor = 1.0f;
			float4 _emissiveFactor{ 0.0f, 0.0f, 0.0f };

			TextureBinding _baseColorTexture;
			TextureBinding _metallicRoughnessTexture;
			TextureBinding _normalTexture;
			TextureBinding _occlusionTexture;
			TextureBinding _emissiveTexture;

			float _normalScale = 1.0f;
			float _occlusionStrength = 1.0f;

			bool _doubleSided = false;
			uint64 _alphaModeHash = StringUtil::hash("OPAQUE"); // OPAQUE / MASK / BLEND
			float _alphaCutoff = 0.5f;
		};

		DKVector<Ptr<Material>> materials;
		materials.reserve(gltfModel.materials.size());

		TextureBinding (*const MakeTextureBinding)(const tinygltf::Model&, int, int) = [](const tinygltf::Model & model, int textureIndex, int texCoord)->TextureBinding
		{
			TextureBinding result;
			result._textureIndex = textureIndex;
			result._texCoord = texCoord;
			return result;
		};
		const DKFunction<Material*(const tinygltf::Model&, const tinygltf::Material&)> ConvertMaterial = [&MakeTextureBinding, &textures](const tinygltf::Model& model, const tinygltf::Material& src)->Material*
		{
			CpuMaterial dst;
#if defined(_DK_DEBUG_)
			dst._name = src.name;
#endif

			//{
			//	DKArray<float, 4> factor{ dst._baseColorFactor.x, dst._baseColorFactor.y, dst._baseColorFactor.z, dst._baseColorFactor.w };
			//	CopyFactor(src.pbrMetallicRoughness.baseColorFactor, factor);
			//	dst._baseColorFactor = { factor[0], factor[1], factor[2], factor[3] };
			//}

			dst._metallicFactor = static_cast<float>(src.pbrMetallicRoughness.metallicFactor);
			dst._roughnessFactor = static_cast<float>(src.pbrMetallicRoughness.roughnessFactor);

			//{
			//	DKArray<float, 3> factor{ dst._emissiveFactor.x, dst._emissiveFactor.y, dst._emissiveFactor.z };
			//	CopyFactor(src.emissiveFactor, factor);
			//	dst._emissiveFactor = { factor[0], factor[1], factor[2] };
			//}

			const int textureCount = static_cast<int>(model.textures.size());
			if (src.pbrMetallicRoughness.baseColorTexture.index < 0 || src.pbrMetallicRoughness.baseColorTexture.index >= textureCount || src.pbrMetallicRoughness.metallicRoughnessTexture.index < -1 || src.pbrMetallicRoughness.metallicRoughnessTexture.index >= textureCount || src.normalTexture.index < -1 || src.normalTexture.index >= textureCount || src.occlusionTexture.index < -1 || src.occlusionTexture.index >= textureCount || src.emissiveTexture.index < -1 || src.emissiveTexture.index >= textureCount)
			{
				DK_ASSERT_LOG(false, "Unsupported or invalid glTF material texture indices");
				return nullptr;
			}

			dst._baseColorTexture = MakeTextureBinding(model, src.pbrMetallicRoughness.baseColorTexture.index, src.pbrMetallicRoughness.baseColorTexture.texCoord);
			dst._metallicRoughnessTexture = MakeTextureBinding(model, src.pbrMetallicRoughness.metallicRoughnessTexture.index, src.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);
			dst._normalTexture = MakeTextureBinding(model, src.normalTexture.index, src.normalTexture.texCoord);
			dst._occlusionTexture = MakeTextureBinding(model, src.occlusionTexture.index, src.occlusionTexture.texCoord);
			dst._emissiveTexture = MakeTextureBinding(model, src.emissiveTexture.index, src.emissiveTexture.texCoord);
			dst._normalScale = static_cast<float>(src.normalTexture.scale);
			dst._occlusionStrength = static_cast<float>(src.occlusionTexture.strength);
			dst._doubleSided = src.doubleSided;
			dst._alphaModeHash = StringUtil::registerHash(src.alphaMode.empty() ? "OPAQUE" : src.alphaMode.c_str());
			dst._alphaCutoff = static_cast<float>(src.alphaCutoff);

			SceneRenderer& sceneRenderer = DuckingEngine::getInstance().getSceneRenderWritable();
			const MaterialDefinition* materialDefinition = sceneRenderer.getMaterialDefinition("StaticMeshStandard");
			if (materialDefinition == nullptr)
			{
				DK_ASSERT_LOG(false, "RenderPass에 없는 Material(%s)을 만들려합니다! 절대 발생하면 안됩니다!", "StaticMeshStandard");
				return nullptr;
			}

			// RenderPass로부터 Parameter를 세팅
			const uint32 parameterCount = static_cast<uint32>(materialDefinition->_parameters.size());
			Ptr<Material> outMaterial = Material::createMaterial("StaticMeshStandard");
			if (outMaterial.get() == nullptr)
				return nullptr;

			const ITextureRef& baseColorTexture = textures[dst._baseColorTexture._textureIndex];
			for (uint32 i = 0; i < parameterCount; ++i)
			{
				MaterialParameter* parameter = outMaterial->_parameterArr[i].get();
				const uint64 nameHash = parameter->getParameterNameHash();
				if (nameHash == StringUtil::hash("_diffuseTexture"))
					static_cast<MaterialParameterTexture*>(parameter)->setParameterValue(baseColorTexture);
				if (nameHash == StringUtil::hash("_metallic"))
					static_cast<MaterialParameterFloat*>(parameter)->setParameterValue(dst._metallicFactor);
				if (nameHash == StringUtil::hash("_roughness"))
					static_cast<MaterialParameterFloat*>(parameter)->setParameterValue(dst._roughnessFactor);
				if (nameHash == StringUtil::hash("_alphaCutoff") && dst._alphaModeHash == StringUtil::hash("MASK"))
					static_cast<MaterialParameterFloat*>(parameter)->setParameterValue(dst._alphaCutoff);
			}

			outMaterial->_parameterBufferForGPU->upload(outMaterial->_parameterBufferForCPU.data());

			return outMaterial.relocate();
		};
		for (const tinygltf::Material& src : gltfModel.materials)
		{
			Ptr<Material> material(ConvertMaterial(gltfModel, src));
			if (material.get() == nullptr)
				return false;

			materials.push_back(DK::move(material));
		}

		// Mesh Primitive -> VertexBuffer / IndexBuffer 변환
		int (*const FindAttributeAccessor)(const tinygltf::Primitive&, const char*) = [](const tinygltf::Primitive& primitive, const char* semantic)->int
		{
			const decltype(primitive.attributes)::const_iterator it = primitive.attributes.find(semantic);
			if (it == primitive.attributes.end())
				return -1;

			return it->second;
		};
		struct AccessorSpan
		{
			const uint8_t* _data = nullptr;

			size_t _count = 0;
			size_t _stride = 0;
			size_t _elementSize = 0;
			size_t _componentCount = 0;

			int _componentType = 0;
			bool _normalized = false;
		};
		AccessorSpan (*const GetAccessorSpan)(const tinygltf::Model&, int) = [](const tinygltf::Model& model, int accessorIndex)->AccessorSpan
		{
			if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
			{
				DK_ASSERT_LOG(false, "잘못된 accessor 인덱스");
				return AccessorSpan();
			}

			const tinygltf::Accessor& accessor = model.accessors[accessorIndex];

			// Sparse accessor는 base buffer data 위에 patch를 적용해야 합니다.
			// 이 예제는 명시적으로 거부합니다.
			if (accessor.sparse.isSparse)
			{
				DK_ASSERT_LOG(false, "Sparse accessor는 이 샘플에서 지원하지 않습니다.");
				return AccessorSpan();
			}
			if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
			{
				DK_ASSERT_LOG(false, "accessor에 유효한 bufferView가 없습니다.");
				return AccessorSpan();
			}

			const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
			if (view.buffer < 0 || view.buffer >= static_cast<int>(model.buffers.size()))
			{
				DK_ASSERT_LOG(false, "bufferView가 잘못된 buffer를 참조합니다.");
				return AccessorSpan();
			}
			if (view.byteStride < 0)
			{
				DK_ASSERT_LOG(false, "음수 byteStride는 허용되지 않습니다.");
				return AccessorSpan();
			}
			if (accessor.byteOffset > view.byteLength)
			{
				DK_ASSERT_LOG(false, "accessor byteOffset이 bufferView 범위를 벗어났습니다.");
				return AccessorSpan();
			}

			const tinygltf::Buffer& buffer = model.buffers[view.buffer];
			if (view.byteOffset > buffer.data.size() || view.byteLength > buffer.data.size() - view.byteOffset)
			{
				DK_ASSERT_LOG(false, "bufferView 범위가 buffer 범위를 벗어났습니다.");
				return AccessorSpan();
			}

			size_t componentSize, componentCount;
			switch (accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				componentSize = 1;
				break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				componentSize = 2;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				componentSize = 4;
				break;
			default:
			{
				DK_ASSERT_LOG(false, "지원하지 않는 accessor componentType");
				return AccessorSpan();
			}
			}
			switch (accessor.type)
			{
			case TINYGLTF_TYPE_SCALAR: componentCount = 1; break;
			case TINYGLTF_TYPE_VEC2:   componentCount = 2; break;
			case TINYGLTF_TYPE_VEC3:   componentCount = 3; break;
			case TINYGLTF_TYPE_VEC4:   componentCount = 4; break;
			default:
			{
				DK_ASSERT_LOG(false, "SCALAR/VEC2/VEC3/VEC4 이외 accessor는 이 로더에서 지원하지 않습니다.");
				return AccessorSpan();
			}
			}

			const size_t elementSize = componentSize * componentCount;
			const size_t stride = (view.byteStride != 0) ? static_cast<size_t>(view.byteStride) : elementSize;
			if (stride < elementSize)
			{
				DK_ASSERT_LOG(false, "byteStride가 element size보다 작습니다.");
				return AccessorSpan();
			}

			const size_t availableBytes = view.byteLength - accessor.byteOffset;
			if (accessor.count > 0)
			{
				if (elementSize > availableBytes)
				{
					DK_ASSERT_LOG(false, "accessor element가 bufferView 범위를 벗어났습니다.");
					return AccessorSpan();
				}

				const size_t remainingBytes = availableBytes - elementSize;
				const size_t elementCountAfterFirst = accessor.count - 1;
				if (elementCountAfterFirst > remainingBytes / stride)
				{
					DK_ASSERT_LOG(false, "accessor 데이터가 bufferView 범위를 벗어났습니다.");
					return AccessorSpan();
				}
			}

			AccessorSpan result;
			result._count = accessor.count;
			result._stride = stride;
			result._elementSize = elementSize;
			result._componentCount = componentCount;
			result._componentType = accessor.componentType;
			result._normalized = accessor.normalized;

			if (accessor.count > 0)
				result._data = buffer.data.data() + view.byteOffset + accessor.byteOffset;

			return result;
		};
		float (*const ReadFloatComponent)(const AccessorSpan&, size_t, size_t) = [](const AccessorSpan& accessor, size_t elementIndex, size_t componentIndex)->float
		{
			if (elementIndex >= accessor._count || componentIndex >= accessor._componentCount)
			{
				DK_ASSERT_LOG(false, "accessor read 범위 초과");
				return 0;
			}

			size_t componentSize = 0;
			switch (accessor._componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				componentSize = 1;
				break;
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				componentSize = 2;
				break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				componentSize = 4;
				break;
			default:
			{
				DK_ASSERT_LOG(false, "지원하지 않는 accessor componentType");
				return 0;
			}
			}

			const uint8_t* ptr = accessor._data + elementIndex * accessor._stride + componentIndex * componentSize;

			switch (accessor._componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			{
				const float value = static_cast<float>(ReadUnaligned<int8_t>(ptr));
				return accessor._normalized ? Math::max(-1.0f, value / 127.0f) : value;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			{
				const float value = static_cast<float>(ReadUnaligned<uint8_t>(ptr));
				return accessor._normalized ? value / 255.0f : value;
			}
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			{
				const float value = static_cast<float>(ReadUnaligned<int16_t>(ptr));
				return accessor._normalized ? Math::max(-1.0f, value / 32767.0f) : value;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			{
				const float value = static_cast<float>(ReadUnaligned<uint16_t>(ptr));
				return accessor._normalized ? value / 65535.0f : value;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			{
				const float value = static_cast<float>(ReadUnaligned<uint32_t>(ptr));
				return accessor._normalized ? value / 4294967295.0f : value;
			}
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return ReadUnaligned<float>(ptr);
			default:
			{
				DK_ASSERT_LOG(false, "float으로 변환할 수 없는 componentType");
				return 0;
			}
			}
		};
		uint32_t (*const ReadIndex)(const AccessorSpan&, size_t) = [](const AccessorSpan& accessor, size_t index)->uint32_t
		{
			if (accessor._componentCount != 1)
			{
				DK_ASSERT_LOG(false, "Index accessor는 SCALAR여야 합니다.");
				return UINT32_MAX;
			}
			if (index >= accessor._count)
			{
				DK_ASSERT_LOG(false, "Index accessor 범위 초과");
				return UINT32_MAX;
			}

			const uint8_t* ptr = accessor._data + index * accessor._stride;
			switch (accessor._componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return ReadUnaligned<uint8_t>(ptr);
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return ReadUnaligned<uint16_t>(ptr);
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return ReadUnaligned<uint32_t>(ptr);
			default:
			{
				DK_ASSERT_LOG(false, "Index accessor는 UNSIGNED_BYTE/SHORT/INT만 지원합니다.");
				return UINT32_MAX;
			}
			}
		};
		bool (*const ConvertToTriangleList)(const DKVector<uint32_t>&, int, DKVector<uint32_t>&) = [](const DKVector<uint32_t>& sourceIndices, int primitiveMode, DKVector<uint32_t>& result)->bool
		{
			switch (primitiveMode)
			{
			case TINYGLTF_MODE_TRIANGLES:
			{
				if ((sourceIndices.size() % 3) != 0)
				{
					DK_ASSERT_LOG(false, "TRIANGLES index 개수는 3의 배수여야 합니다.");
					return false;
				}

				result = sourceIndices;
				return true;
			}
			case TINYGLTF_MODE_TRIANGLE_STRIP:
			{
				if (sourceIndices.size() < 3)
				{
					DK_ASSERT_LOG(false, "TRIANGLES index 개수는 3의 배수여야 합니다.");
					return false;
				}

				result.reserve((sourceIndices.size() - 2) * 3);
				for (size_t i = 0; i + 2 < sourceIndices.size(); ++i)
				{
					// Triangle strip은 triangle마다 winding이 바뀝니다.
					if ((i & 1) == 0)
					{
						result.push_back(sourceIndices[i + 0]);
						result.push_back(sourceIndices[i + 1]);
						result.push_back(sourceIndices[i + 2]);
					}
					else
					{
						result.push_back(sourceIndices[i + 1]);
						result.push_back(sourceIndices[i + 0]);
						result.push_back(sourceIndices[i + 2]);
					}
				}

				return true;
			}
			case TINYGLTF_MODE_TRIANGLE_FAN:
			{
				if (sourceIndices.size() < 3)
				{
					DK_ASSERT_LOG(false, "TRIANGLES index 개수는 3의 배수여야 합니다.");
					return false;
				}

				result.reserve((sourceIndices.size() - 2) * 3);
				for (size_t i = 1; i + 1 < sourceIndices.size(); ++i)
				{
					result.push_back(sourceIndices[0]);
					result.push_back(sourceIndices[i]);
					result.push_back(sourceIndices[i + 1]);
				}

				return true;
			}
			default:
			{
				DK_ASSERT_LOG(false, "POINTS/LINES 계열 primitive는 이 로더에서 지원하지 않습니다.");
				return false;
			}
			}
		};
		const DKFunction<bool(const tinygltf::Model&, const tinygltf::Primitive&, uint32_t, uint32_t, CpuPrimitive<StaticMeshModel::SubMeshType::VertexType>&)> ConvertPrimitive = [&FindAttributeAccessor, &GetAccessorSpan, &ReadFloatComponent, &ReadIndex, &ConvertToTriangleList](const tinygltf::Model& model, const tinygltf::Primitive& primitive, uint32_t meshIndex, uint32_t primitiveIndex, CpuPrimitive<StaticMeshModel::SubMeshType::VertexType>& result)->bool
		{
			const int positionAccessorIndex = FindAttributeAccessor(primitive, "POSITION");
			if (positionAccessorIndex < 0)
			{
				DK_ASSERT_LOG(false, "glTF primitive에 POSITION attribute가 없습니다.");
				return false;
			}

			const AccessorSpan positions = GetAccessorSpan(model, positionAccessorIndex);
			if (positions._componentCount != 3)
			{
				DK_ASSERT_LOG(false, "POSITION은 VEC3여야 합니다.");
				return false;
			}

			const size_t vertexCount = positions._count;

			DKOptional<AccessorSpan> normals;
			DKOptional<AccessorSpan> uv0;
			DKOptional<AccessorSpan> uv1;

			const int normalAccessorIndex = FindAttributeAccessor(primitive, "NORMAL");
			const int uv0AccessorIndex = FindAttributeAccessor(primitive, "TEXCOORD_0");
			const int uv1AccessorIndex = FindAttributeAccessor(primitive, "TEXCOORD_1");
			if (normalAccessorIndex >= 0)
			{
				normals = GetAccessorSpan(model, normalAccessorIndex);
				if (normals->_componentCount != 3 || normals->_count != vertexCount)
				{
					DK_ASSERT_LOG(false, "NORMAL accessor 형식 또는 vertex count가 POSITION과 다릅니다.");
					return false;
				}
			}
			if (uv0AccessorIndex >= 0)
			{
				uv0 = GetAccessorSpan(model, uv0AccessorIndex);
				if (uv0->_componentCount != 2 || uv0->_count != vertexCount)
				{
					DK_ASSERT_LOG(false, "TEXCOORD_0 accessor 형식 또는 vertex count가 POSITION과 다릅니다.");
					return false;
				}
			}
			if (uv1AccessorIndex >= 0)
			{
				uv1 = GetAccessorSpan(model, uv1AccessorIndex);
				if (uv1->_componentCount != 2 || uv1->_count != vertexCount)
				{
					DK_ASSERT_LOG(false, "TEXCOORD_1 accessor 형식 또는 vertex count가 POSITION과 다릅니다.");
					return false;
				}
			}

			//result._name = primitive.name;
			result._sourceMeshIndex = meshIndex;
			result._sourcePrimitiveIndex = primitiveIndex;

			result._vertices.reserve(vertexCount);
			for (size_t i = 0; i < vertexCount; ++i)
			{
				float3 position = { ReadFloatComponent(positions, i, 0), ReadFloatComponent(positions, i, 1), ReadFloatComponent(positions, i, 2) };
				float3 normal = float3::Zero;
				float2 uv = float2::Zero;
				if (normals.has_value())
					normal = { ReadFloatComponent(*normals, i, 0), ReadFloatComponent(*normals, i, 1), ReadFloatComponent(*normals, i, 2) };
				if (uv0.has_value())
					uv = { ReadFloatComponent(*uv0, i, 0), ReadFloatComponent(*uv0, i, 1) };
				//if (uv1.has_value())
				//	uv2 = { ReadFloatComponent(*uv1, i, 0), ReadFloatComponent(*uv1, i, 1) };

				result._vertices.push_back(StaticMeshModel::SubMeshType::VertexType{ position, normal, uv });
			}

			DKVector<uint32_t> sourceIndices;
			if (primitive.indices >= 0)
			{
				const AccessorSpan indexAccessor = GetAccessorSpan(model, primitive.indices);
				if (indexAccessor._componentCount != 1)
				{
					DK_ASSERT_LOG(false, "Index accessor는 SCALAR 타입이어야 합니다.");
					return false;
				}

				sourceIndices.resize(indexAccessor._count);
				for (size_t i = 0; i < indexAccessor._count; ++i)
				{
					const uint32_t index = ReadIndex(indexAccessor, i);
					if (index >= vertexCount)
					{
						DK_ASSERT_LOG(false, "IndexBuffer가 VertexBuffer 범위를 벗어났습니다.");
						return false;
					}

					sourceIndices[i] = index;
				}
			}
			else
			{
				//// index accessor가 없는 glTF primitive도 유효합니다.
				//sourceIndices.resize(vertexCount);
				//std::iota(sourceIndices.begin(), sourceIndices.end(), 0u);
				DK_ASSERT_LOG(false, "");
				return false;
			}

			if (ConvertToTriangleList(sourceIndices, primitive.mode, result._indices) == false)
				return false;

			if (primitive.material >= 0)
			{
				if (primitive.material >= static_cast<int>(model.materials.size()))
				{
					DK_ASSERT_LOG(false, "primitive가 존재하지 않는 material을 참조합니다.");
					return false;
				}

				// CpuModel::materials[0]은 기본 material.
				result._materialIndex = static_cast<uint32_t>(primitive.material);
			}
			else
			{
				//result._materialIndex = 0;
				DK_ASSERT_LOG(false, "");
				return false;
			}

			return true;
		};

		models.reserve(gltfModel.meshes.size());

		for (uint32_t meshIndex = 0; meshIndex < static_cast<uint32_t>(gltfModel.meshes.size()); ++meshIndex)
		{
			const tinygltf::Mesh& mesh = gltfModel.meshes[meshIndex];

			DKVector<StaticMeshModel::SubMeshType> subMeshArr;
			subMeshArr.reserve(static_cast<uint32_t>(mesh.primitives.size()));
			for (uint32_t primitiveIndex = 0; primitiveIndex < static_cast<uint32_t>(mesh.primitives.size()); ++primitiveIndex)
			{
				const tinygltf::Primitive& primitive = mesh.primitives[primitiveIndex];
				CpuPrimitive<StaticMeshModel::SubMeshType::VertexType> cpuPrimitive;
				if (ConvertPrimitive(gltfModel, primitive, meshIndex, primitiveIndex, cpuPrimitive) == false)
					return false;

				//result.model.primitives.push_back();

				VertexBufferViewRef vertexBufferView;
				IBufferRef vertexBuffer = renderModule.createVertexBuffer(cpuPrimitive._vertices.data(), sizeof(StaticMeshModel::SubMeshType::VertexType), cpuPrimitive._vertices.size(), vertexBufferView, DK_DEBUG_NAME(L"GLTF_VertexBuffer"));
				if (vertexBuffer == nullptr)
					return false;

				IndexBufferViewRef indexBufferView;
				IBufferRef indexBuffer = renderModule.createIndexBuffer(cpuPrimitive._indices.data(), cpuPrimitive._indices.size(), indexBufferView, DK_DEBUG_NAME(L"GLTF_IndexBuffer"));
				if (indexBuffer == nullptr)
					return false;

				Material* newMaterial = materials[cpuPrimitive._materialIndex]->clone();
				if (newMaterial == nullptr)
					return false;

				StaticMeshModel::SubMeshType subMesh(
					DK::move(cpuPrimitive._vertices), DK::move(cpuPrimitive._indices),
					DK::move(vertexBuffer), DK::move(indexBuffer),
					DK::move(vertexBufferView), DK::move(indexBufferView),
					newMaterial
				);
				subMeshArr.push_back(DK::move(subMesh));
			}

			models.push_back(DK::makeRef<StaticMeshModel>(DK::move(subMeshArr)));
		}
		DK_ASSERT_LOG(models.empty() == false, "glTF scene needs meshes: %s", path);
		return models.empty() == false;
	}

	bool SceneObjectManager::createStaticMeshInstances(const DKVector<StaticMeshModelRef>& models, const DKVector<Transform>& transforms)
	{
		constexpr float chunkMeters = 32.0f;
		DKHashMap<uint64, DKVector<StaticMeshInstance>> chunkInstances;
		for (const Transform& transform : transforms)
		{
			const float3& position = transform.get_translation();
			const int chunkX = static_cast<int>(Math::floor(position.x / chunkMeters));
			const int chunkZ = static_cast<int>(Math::floor(position.z / chunkMeters));
			const uint64 chunkKey = (uint64(static_cast<uint32>(chunkX)) << 32) | static_cast<uint32>(chunkZ);
			StaticMeshInstance instance;
			transform.tofloat4x4(instance._worldMatrix);
			chunkInstances[chunkKey].push_back(instance);
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		for (const StaticMeshModelRef& model : models)
		{
			float3 minimum(FLT_MAX, FLT_MAX, FLT_MAX);
			float3 maximum(-FLT_MAX, -FLT_MAX, -FLT_MAX);
			for (const StaticMeshModel::SubMeshType& subMesh : model->get_subMeshArr())
			{
				for (const StaticMeshVertex& vertex : subMesh._vertices)
				{
					minimum = float3(Math::min(minimum.x, vertex._position.x), Math::min(minimum.y, vertex._position.y), Math::min(minimum.z, vertex._position.z));
					maximum = float3(Math::max(maximum.x, vertex._position.x), Math::max(maximum.y, vertex._position.y), Math::max(maximum.z, vertex._position.z));
				}
			}
			DK_ASSERT_LOG(minimum.x <= maximum.x && minimum.y <= maximum.y && minimum.z <= maximum.z, "Static mesh needs valid bounds");

			StaticMeshInstanceBatch batch;
			batch._model = model;
			DKVector<StaticMeshInstance> instances;
			instances.reserve(transforms.size());
			for (const DKHashMap<uint64, DKVector<StaticMeshInstance>>::value_type& entry : chunkInstances)
			{
				StaticMeshInstanceChunk chunk;
				chunk._firstInstance = static_cast<uint32>(instances.size());
				chunk._instanceCount = static_cast<uint32>(entry.second.size());
				for (const StaticMeshInstance& instance : entry.second)
				{
					for (uint32 cornerIndex = 0; cornerIndex < 8; ++cornerIndex)
					{
						const float4 corner((cornerIndex & 1) == 0 ? minimum.x : maximum.x, (cornerIndex & 2) == 0 ? minimum.y : maximum.y, (cornerIndex & 4) == 0 ? minimum.z : maximum.z, 1.0f);
						const float4 worldCorner = corner * instance._worldMatrix;
						chunk._minimum = float3(Math::min(chunk._minimum.x, worldCorner.x), Math::min(chunk._minimum.y, worldCorner.y), Math::min(chunk._minimum.z, worldCorner.z));
						chunk._maximum = float3(Math::max(chunk._maximum.x, worldCorner.x), Math::max(chunk._maximum.y, worldCorner.y), Math::max(chunk._maximum.z, worldCorner.z));
					}
					instances.push_back(instance);
				}
				batch._chunks.push_back(chunk);
			}

			batch._staticMeshInstances = renderModule.createStructuredBuffer(instances.data(), sizeof(StaticMeshInstance), static_cast<uint32>(instances.size()), DK_DEBUG_NAME(L"StaticMeshInstances"));
			if (batch._staticMeshInstances == nullptr)
				return false;

			batch._instances = DK::move(instances);
			_staticMeshInstanceBatches.push_back(DK::move(batch));
		}

		return true;
	}

	SceneObject* SceneObjectManager::publish(SceneObject&& object, DKVector<SceneObject>& objects)
	{
		if (createSceneObjectConstantBuffer(object) == false)
			return nullptr;
		return &objects.emplace_back(DK::move(object));
	}

	SceneObject* SceneObjectManager::createStaticMeshSceneObject(const DKString& modelPath, const DKString& modelPropertyPath)
	{
		SceneObject newSceneObject;
		StaticMeshComponent* component = dk_new StaticMeshComponent;
		newSceneObject.addComponent(component);
		component->set_modelPath(modelPath);
		component->set_modelPropertyPath(modelPropertyPath);
		if (component->loadResource() == false)
			return nullptr;

		return publish(DK::move(newSceneObject), _sceneObjectContainer);
	}

	const AppearanceDataRef SceneObjectManager::loadCharacter_LoadAppearanceFile(const char* appearancePath)
	{
		using FindResult = DKHashMap<uint64, AppearanceDataRef>::iterator;
		FindResult findResult = _appearanceRawContainers.find(StringUtil::hash(appearancePath));
		if (findResult != _appearanceRawContainers.end())
			return findResult->second;

		ScopeString<DK_MAX_PATH> appearanceFullPath = GlobalPath::makeResourceFullPath(appearancePath);

		TiXmlDocument appearanceDocument;
		if (appearanceDocument.LoadFile(appearanceFullPath.c_str()) == false)
		{
			DK_ASSERT_LOG(false, "Appearance File LoadError: %s", appearanceDocument.ErrorDesc());
			return nullptr;
		}

		TiXmlNode* rootNode = appearanceDocument.RootElement();
		TiXmlNode* skeletonNode = rootNode->FirstChild("Skeleton");
		const DKString skeletonPath = skeletonNode->ToElement()->GetText();
		TiXmlNode* animationSetNode = rootNode->FirstChild("AnimationSet");
		const DKString animationSetPath = animationSetNode->ToElement()->GetText();

		DKVector<AppearanceData::ModelData> modelDataArr;
		TiXmlNode* modelDataArrNode = rootNode->FirstChild("ModelDataArr");
		const int modelDataCount = StringUtil::atoi(modelDataArrNode->ToElement()->Attribute("Count"));
		modelDataArr.reserve(modelDataCount);
		for (TiXmlNode* modelDataNode = modelDataArrNode->FirstChild(); modelDataNode != nullptr; modelDataNode = modelDataNode->NextSiblingElement())
		{
			const TiXmlElement* modelNode = modelDataNode->FirstChildElement("Model");
			const char* modelPath = modelNode->GetText();
			const TiXmlElement* modelPropertyNode = modelDataNode->FirstChildElement("ModelProperty");
			const char* modelPropertyPath = modelPropertyNode->GetText();

			modelDataArr.push_back(AppearanceData::ModelData(modelPath, modelPropertyPath));
		}

		const DKPair<DKHashMap<uint64, AppearanceDataRef>::iterator, bool> insertResult = _appearanceRawContainers.insert(DKPair<uint64, AppearanceDataRef>(StringUtil::registerHash(appearancePath), dk_new AppearanceData(skeletonPath, animationSetPath, DK::move(modelDataArr))));
		return insertResult.first->second;
	}
	SceneObject* SceneObjectManager::createCharacter(const char* appearancePath)
	{
		const AppearanceDataRef& appearanceData = loadCharacter_LoadAppearanceFile(appearancePath);
		if (appearanceData == nullptr)
			return nullptr;

		SceneObject newSceneObject;
		const uint32 skinnedMeshCount = static_cast<const uint32>(appearanceData->_modelDataArr.size());
		for (uint32 i = 0; i < skinnedMeshCount; ++i)
		{
			SkinnedMeshComponent* skinnedMeshComponent = dk_new SkinnedMeshComponent;
			skinnedMeshComponent->set_modelPath(appearanceData->_modelDataArr[i]._modelPath);
			skinnedMeshComponent->set_modelPropertyPath(appearanceData->_modelDataArr[i]._modelPropertyPath);
			if (i == 0)	// MainSkinnedMesh
			{
				skinnedMeshComponent->set_skeletonPath(appearanceData->_skeletonPath);
				skinnedMeshComponent->_animationPath = appearanceData->_animationSetPath;	// #todo- 나중에 AnimationSet으로 변경(AnimationController 만든 후에)
			}
			if (skinnedMeshComponent->loadResource() == false)
				return nullptr;

			newSceneObject.addComponent(skinnedMeshComponent);
		}

		return publish(DK::move(newSceneObject), _characterSceneObjectContainer);
	}

	void SceneObjectManager::update(float deltaTime)
	{
		for (SceneObject& entry : _sceneObjectContainer)
			entry.update(deltaTime);
		for (SceneObject& entry : _characterSceneObjectContainer)
			entry.update(deltaTime);
	}
}
