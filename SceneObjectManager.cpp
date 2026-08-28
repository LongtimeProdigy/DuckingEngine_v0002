#include "stdafx.h"
#include "SceneObjectManager.h"

#include "DuckingEngine.h"
#include "RenderModule.h"

#include "SceneObject.h"
#include "StaticMeshComponent.h"
#include "SkinnedMeshComponent.h"
#include "Skeleton.h"

#define USE_TINY_GLTF
#if defined(USE_TINY_GLTF)
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/tinyGLTF/tiny_gltf.h"
#include <optional>
#include "Model.h"
#include "ResourceManager.h"
#include "SceneRenderer.h"
#endif

namespace DK
{
	bool createSceneObjectConstantBuffer(SceneObject& sceneObject)
	{
		// for GPU resource
		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
		SceneObjectConstantBufferStruct sceneObjectConstantBufferData;
		sceneObject.get_worldTransform().tofloat4x4(sceneObjectConstantBufferData._worldMatrix);
		sceneObject._sceneObjectConstantBuffer = renderModule.createUploadBuffer(sizeof(sceneObjectConstantBufferData), L"SceneObject_Cbuffer");
		if (sceneObject._sceneObjectConstantBuffer.get() == nullptr)
		{
			DK_ASSERT_LOG(false, "SceneObjectConstantBuffer 생성에 실패");
			return false;
		}

		return true;
	}

	[[noreturn]] static void Fail(const std::string& message)
	{
		throw std::runtime_error("[glTF Loader] " + message);
	}
	template <typename T>
	static T ReadUnaligned(const uint8_t* src)
	{
		T value{};
		std::memcpy(&value, src, sizeof(T));
		return value;
	}
	template <typename T>
	struct CpuPrimitive
	{
		DKString name;

		// 실질적인 VertexBuffer / IndexBuffer 데이터.
		DKVector<T> vertices;
		DKVector<uint32_t> indices;

		// CpuModel::materials 인덱스.
		// 0은 glTF primitive에 material이 없을 때 사용하는 기본 material.
		uint32_t materialIndex = 0;

		// 원래 glTF mesh / primitive 위치
		uint32_t sourceMeshIndex = 0;
		uint32_t sourcePrimitiveIndex = 0;
	};
	SceneObject* SceneObjectManager::loadGLTF(const char* path)
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
			DK_ASSERT_LOG(false, "확장자는 .gltf 또는 .glb여야 합니다.");
			return false;
		}
		if (!loaded)
		{
			DK_ASSERT_LOG(false, "파일 로딩 실패: %s", error);
			return false;
		}

		RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();

		DKVector<ITextureRef> textures;
		for (const tinygltf::Image& src : gltfModel.images)
		{
			if (src.width <= 0 || src.height <= 0)
				throw std::runtime_error("Invalid CpuImage size.");
			if (src.bits != 8)
				throw std::runtime_error("Only 8-bit CpuImage is supported.");
			if (src.component < 1 || src.component > 4)
				throw std::runtime_error("CpuImage component must be 1~4. not 3");

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
				format = DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			default:
				throw std::runtime_error("CpuImage component must be 1~4. not 3");
				break;
			}

			ITextureRef newTexture = renderModule.createTexture(src.name, src.width, src.height, src.image.data(), 1, format, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, true, false);
			textures.push_back(DK::move(newTexture));
		}

		//// Texture 추출
		//result.model.textures.reserve(gltfModel.textures.size());

		//for (const tinygltf::Texture& src : gltfModel.textures)
		//{
		//	CpuTexture dst;
		//	dst.name = src.name;
		//	dst.imageIndex = src.source;

		//	if (dst.imageIndex >= static_cast<int>(result.model.images.size()))
		//	{
		//		Fail("texture가 존재하지 않는 image를 참조합니다.");
		//	}

		//	result.model.textures.push_back(std::move(dst));
		//}

		// Material 추출
		struct TextureBinding
		{
			// CpuModel::textures 인덱스. 없으면 -1.
			int textureIndex = -1;

			// glTF material textureInfo.texCoord 값.
			// 이 샘플 Vertex는 uv0, uv1만 보관합니다.
			int texCoord = 0;
		};
		struct CpuMaterial
		{
			std::string name;

			// glTF metallic-roughness PBR
			float4 baseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
			float metallicFactor = 1.0f;
			float roughnessFactor = 1.0f;
			float4 emissiveFactor{ 0.0f, 0.0f, 0.0f };

			TextureBinding baseColorTexture;
			TextureBinding metallicRoughnessTexture;
			TextureBinding normalTexture;
			TextureBinding occlusionTexture;
			TextureBinding emissiveTexture;

			float normalScale = 1.0f;
			float occlusionStrength = 1.0f;

			bool doubleSided = false;
			std::string alphaMode = "OPAQUE"; // OPAQUE / MASK / BLEND
			float alphaCutoff = 0.5f;
		};

		DKVector<CpuMaterial> materials;
		materials.reserve(gltfModel.materials.size());

		auto MakeTextureBinding = [](const tinygltf::Model & model, int textureIndex, int texCoord)->TextureBinding
		{
			TextureBinding result;
			result.textureIndex = textureIndex;
			result.texCoord = texCoord;

			if (textureIndex < 0)
				return result;

			if (textureIndex >= static_cast<int>(model.textures.size()))
				Fail("Material이 존재하지 않는 texture를 참조합니다.");

			return result;
		};
		auto ConvertMaterial = [&textures, &MakeTextureBinding](const tinygltf::Model& model, const tinygltf::Material& src)->CpuMaterial
		{
			CpuMaterial dst;
			//dst.name = src.name;
			//
			//{
			//	std::array<float, 4> factor{ dst.baseColorFactor.x, dst.baseColorFactor.y, dst.baseColorFactor.z, dst.baseColorFactor.w };
			//	CopyFactor(src.pbrMetallicRoughness.baseColorFactor, factor);
			//	dst.baseColorFactor = { factor[0], factor[1], factor[2], factor[3] };
			//}
			//
			//dst.metallicFactor = static_cast<float>(src.pbrMetallicRoughness.metallicFactor);
			//dst.roughnessFactor = static_cast<float>(src.pbrMetallicRoughness.roughnessFactor);
			//
			//{
			//	std::array<float, 3> factor{ dst.emissiveFactor.x, dst.emissiveFactor.y, dst.emissiveFactor.z };
			//	CopyFactor(src.emissiveFactor, factor);
			//	dst.emissiveFactor = { factor[0], factor[1], factor[2] };
			//}

			dst.baseColorTexture = MakeTextureBinding(model, src.pbrMetallicRoughness.baseColorTexture.index, src.pbrMetallicRoughness.baseColorTexture.texCoord);

			//dst.metallicRoughnessTexture = MakeTextureBinding(model, src.pbrMetallicRoughness.metallicRoughnessTexture.index, src.pbrMetallicRoughness.metallicRoughnessTexture.texCoord);

			dst.normalTexture = MakeTextureBinding(model, src.normalTexture.index, src.normalTexture.texCoord);

			//dst.occlusionTexture = MakeTextureBinding(model, src.occlusionTexture.index, src.occlusionTexture.texCoord);
			//
			//dst.emissiveTexture = MakeTextureBinding(model, src.emissiveTexture.index, src.emissiveTexture.texCoord);
			//
			//dst.normalScale = static_cast<float>(src.normalTexture.scale);
			//
			//dst.occlusionStrength = static_cast<float>(src.occlusionTexture.strength);
			//
			//dst.doubleSided = src.doubleSided;
			//dst.alphaMode = src.alphaMode.empty() ? "OPAQUE" : src.alphaMode;
			//dst.alphaCutoff = static_cast<float>(src.alphaCutoff);

			SceneRenderer& sceneRenderer = DuckingEngine::getInstance().getSceneRenderWritable();
			const MaterialDefinition* materialDefinition = sceneRenderer.getMaterialDefinition("StaticMeshStandard");
			if (materialDefinition == nullptr)
			{
				DK_ASSERT_LOG(false, "RenderPass에 없는 Material(%s)을 만들려합니다! 절대 발생하면 안됩니다!", "StaticMeshStandard");
				return dst;
			}

			// RenderPass로부터 Parameter를 세팅
			uint32 parameterBufferSize = 0;
			const uint32 parameterCount = static_cast<uint32>(materialDefinition->_parameters.size());
			Material* outMaterial = dk_new Material;
			outMaterial->_materialName = "StaticMeshStandard";
			outMaterial->_parameterArr.resize(parameterCount);
			for (uint32 i = 0; i < parameterCount; ++i)
			{
				MaterialParameter* newParameter = MaterialParameter::createMaterialParameter(materialDefinition->_parameters[i]);
				outMaterial->_parameterArr[i] = newParameter;

				parameterBufferSize += outMaterial->_parameterArr[i]->getParameterSize();
			}

			// CPU 및 GPU 버퍼 생성 및 Parameter ValuePtr 세팅
			uint32 offset = 0;
			outMaterial->_parameterBufferForCPU.resize(parameterBufferSize);
			for (uint32 i = 0; i < parameterCount; ++i)
			{
				outMaterial->_parameterArr[i]->setParameterValuePtr(&outMaterial->_parameterBufferForCPU[offset]);
				offset += outMaterial->_parameterArr[i]->getParameterSize();
			}

			RenderModule& renderModule = DuckingEngine::getInstance().GetRenderModuleWritable();
			outMaterial->_parameterBufferForGPU = renderModule.createUploadBuffer(parameterBufferSize, L"Material_CBuffer");

			ITextureRef baseColorTexture = textures[dst.baseColorTexture.textureIndex];
			for (uint32 i = 0; i < parameterCount; ++i)
			{
				MaterialParameter* parameter = outMaterial->_parameterArr[i].get();
				if (parameter->getParameterName() != "_diffuseTexture")
					continue;

				MaterialParameterTexture* textureParameter = static_cast<MaterialParameterTexture*>(parameter);
				textureParameter->setParameterValue(baseColorTexture);
				break;
			}

			outMaterial->_parameterBufferForGPU->upload(outMaterial->_parameterBufferForCPU.data());

			return dst;
		};
		for (const tinygltf::Material& src : gltfModel.materials)
			materials.push_back(ConvertMaterial(gltfModel, src));

		// Mesh Primitive -> VertexBuffer / IndexBuffer 변환
		auto FindAttributeAccessor = [](const tinygltf::Primitive& primitive, const char* semantic)->int
		{
			const auto it = primitive.attributes.find(semantic);
			if (it == primitive.attributes.end())
				return -1;

			return it->second;
		};
		struct AccessorSpan
		{
			const uint8_t* data = nullptr;

			size_t count = 0;
			size_t stride = 0;
			size_t elementSize = 0;
			size_t componentCount = 0;

			int componentType = 0;
			bool normalized = false;
		};
		auto GetAccessorSpan = [](const tinygltf::Model& model, int accessorIndex)->AccessorSpan
		{
			if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
				Fail("잘못된 accessor 인덱스");

			const tinygltf::Accessor& accessor = model.accessors[accessorIndex];

			// Sparse accessor는 base buffer data 위에 patch를 적용해야 합니다.
			// 이 예제는 명시적으로 거부합니다.
			if (accessor.sparse.isSparse)
				Fail("Sparse accessor는 이 샘플에서 지원하지 않습니다.");
			if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
				Fail("accessor에 유효한 bufferView가 없습니다.");

			const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
			if (view.buffer < 0 || view.buffer >= static_cast<int>(model.buffers.size()))
				Fail("bufferView가 잘못된 buffer를 참조합니다.");
			if (view.byteStride < 0)
				Fail("음수 byteStride는 허용되지 않습니다.");
			if (accessor.byteOffset > view.byteLength)
				Fail("accessor byteOffset이 bufferView 범위를 벗어났습니다.");

			const tinygltf::Buffer& buffer = model.buffers[view.buffer];
			if (view.byteOffset > buffer.data.size() || view.byteLength > buffer.data.size() - view.byteOffset)
				Fail("bufferView 범위가 buffer 범위를 벗어났습니다.");

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
				Fail("지원하지 않는 accessor componentType");
			}
			switch (accessor.type)
			{
			case TINYGLTF_TYPE_SCALAR: componentCount = 1; break;
			case TINYGLTF_TYPE_VEC2:   componentCount = 2; break;
			case TINYGLTF_TYPE_VEC3:   componentCount = 3; break;
			case TINYGLTF_TYPE_VEC4:   componentCount = 4; break;
			default:
				Fail("SCALAR/VEC2/VEC3/VEC4 이외 accessor는 이 로더에서 지원하지 않습니다.");
			}

			const size_t elementSize = componentSize * componentCount;
			const size_t stride = (view.byteStride != 0) ? static_cast<size_t>(view.byteStride) : elementSize;
			if (stride < elementSize)
				Fail("byteStride가 element size보다 작습니다.");

			const size_t availableBytes = view.byteLength - accessor.byteOffset;
			if (accessor.count > 0)
			{
				if (elementSize > availableBytes)
					Fail("accessor element가 bufferView 범위를 벗어났습니다.");

				const size_t remainingBytes = availableBytes - elementSize;
				const size_t elementCountAfterFirst = accessor.count - 1;
				if (elementCountAfterFirst > remainingBytes / stride)
					Fail("accessor 데이터가 bufferView 범위를 벗어났습니다.");
			}

			AccessorSpan result;
			result.count = accessor.count;
			result.stride = stride;
			result.elementSize = elementSize;
			result.componentCount = componentCount;
			result.componentType = accessor.componentType;
			result.normalized = accessor.normalized;

			if (accessor.count > 0)
				result.data = buffer.data.data() + view.byteOffset + accessor.byteOffset;

			return result;
		};
		auto ComponentByteSize = [](int componentType)->size_t
		{
			switch (componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return 1;

			case TINYGLTF_COMPONENT_TYPE_SHORT:
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return 2;

			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return 4;

			default:
				Fail("지원하지 않는 accessor componentType");
			}
		};
		auto ReadComponentAsFloat = [](const uint8_t* ptr, int componentType, bool normalized)->float
		{
			switch (componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_BYTE:
			{
				const float value = static_cast<float>(ReadUnaligned<int8_t>(ptr));
				return normalized ? std::max(-1.0f, value / 127.0f) : value;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			{
				const float value = static_cast<float>(ReadUnaligned<uint8_t>(ptr));
				return normalized ? value / 255.0f : value;
			}
			case TINYGLTF_COMPONENT_TYPE_SHORT:
			{
				const float value = static_cast<float>(ReadUnaligned<int16_t>(ptr));
				return normalized ? std::max(-1.0f, value / 32767.0f) : value;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			{
				const float value = static_cast<float>(ReadUnaligned<uint16_t>(ptr));
				return normalized ? value / 65535.0f : value;
			}
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			{
				const float value = static_cast<float>(ReadUnaligned<uint32_t>(ptr));
				return normalized ? value / 4294967295.0f : value;
			}
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				return ReadUnaligned<float>(ptr);
			default:
				Fail("float으로 변환할 수 없는 componentType");
			}
		};
		auto ReadFloatComponent = [&ComponentByteSize, &ReadComponentAsFloat](const AccessorSpan& accessor, size_t elementIndex, size_t componentIndex)->float
		{
			if (elementIndex >= accessor.count || componentIndex >= accessor.componentCount)
				Fail("accessor read 범위 초과");

			const size_t componentSize = ComponentByteSize(accessor.componentType);

			const uint8_t* ptr = accessor.data + elementIndex * accessor.stride + componentIndex * componentSize;

			return ReadComponentAsFloat(ptr, accessor.componentType, accessor.normalized);
		};
		auto ReadIndex = [](const AccessorSpan& accessor, size_t index)->uint32_t
		{
			if (accessor.componentCount != 1)
				Fail("Index accessor는 SCALAR여야 합니다.");
			if (index >= accessor.count)
				Fail("Index accessor 범위 초과");

			const uint8_t* ptr = accessor.data + index * accessor.stride;
			switch (accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
				return ReadUnaligned<uint8_t>(ptr);
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
				return ReadUnaligned<uint16_t>(ptr);
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
				return ReadUnaligned<uint32_t>(ptr);
			default:
				Fail("Index accessor는 UNSIGNED_BYTE/SHORT/INT만 지원합니다.");
			}
		};
		auto ConvertToTriangleList = [](const DKVector<uint32_t>& sourceIndices, int primitiveMode)->DKVector<uint32_t>
		{
			switch (primitiveMode)
			{
			case TINYGLTF_MODE_TRIANGLES:
			{
				if ((sourceIndices.size() % 3) != 0)
					Fail("TRIANGLES index 개수는 3의 배수여야 합니다.");
				return sourceIndices;
			}
			case TINYGLTF_MODE_TRIANGLE_STRIP:
			{
				DKVector<uint32_t> result;
				if (sourceIndices.size() < 3)
					return result;

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

				return result;
			}
			case TINYGLTF_MODE_TRIANGLE_FAN:
			{
				DKVector<uint32_t> result;
				if (sourceIndices.size() < 3)
					return result;

				result.reserve((sourceIndices.size() - 2) * 3);
				for (size_t i = 1; i + 1 < sourceIndices.size(); ++i)
				{
					result.push_back(sourceIndices[0]);
					result.push_back(sourceIndices[i]);
					result.push_back(sourceIndices[i + 1]);
				}

				return result;
			}
			default:
				Fail("POINTS/LINES 계열 primitive는 이 로더에서 지원하지 않습니다.");
			}
		};
		auto ConvertPrimitive = [&FindAttributeAccessor, &GetAccessorSpan, &ReadFloatComponent, &ReadIndex, &ConvertToTriangleList](const tinygltf::Model& model, const tinygltf::Primitive& primitive, uint32_t meshIndex, uint32_t primitiveIndex)->CpuPrimitive<StaticMeshModel::SubMeshType::VertexType>
		{
			const int positionAccessorIndex = FindAttributeAccessor(primitive, "POSITION");
			if (positionAccessorIndex < 0)
				Fail("glTF primitive에 POSITION attribute가 없습니다.");

			const AccessorSpan positions = GetAccessorSpan(model, positionAccessorIndex);
			if (positions.componentCount != 3)
				Fail("POSITION은 VEC3여야 합니다.");

			const size_t vertexCount = positions.count;

			std::optional<AccessorSpan> normals;
			std::optional<AccessorSpan> uv0;
			std::optional<AccessorSpan> uv1;

			const int normalAccessorIndex = FindAttributeAccessor(primitive, "NORMAL");
			const int uv0AccessorIndex = FindAttributeAccessor(primitive, "TEXCOORD_0");
			const int uv1AccessorIndex = FindAttributeAccessor(primitive, "TEXCOORD_1");
			if (normalAccessorIndex >= 0)
			{
				normals = GetAccessorSpan(model, normalAccessorIndex);
				if (normals->componentCount != 3 || normals->count != vertexCount)
					Fail("NORMAL accessor 형식 또는 vertex count가 POSITION과 다릅니다.");
			}
			if (uv0AccessorIndex >= 0)
			{
				uv0 = GetAccessorSpan(model, uv0AccessorIndex);
				if (uv0->componentCount != 2 || uv0->count != vertexCount)
					Fail("TEXCOORD_0 accessor 형식 또는 vertex count가 POSITION과 다릅니다.");
			}
			if (uv1AccessorIndex >= 0)
			{
				uv1 = GetAccessorSpan(model, uv1AccessorIndex);
				if (uv1->componentCount != 2 || uv1->count != vertexCount)
					Fail("TEXCOORD_1 accessor 형식 또는 vertex count가 POSITION과 다릅니다.");
			}

			CpuPrimitive<StaticMeshModel::SubMeshType::VertexType> result;
			//result.name = primitive.name;
			result.sourceMeshIndex = meshIndex;
			result.sourcePrimitiveIndex = primitiveIndex;

			result.vertices.resize(vertexCount);
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

				result.vertices.push_back(StaticMeshModel::SubMeshType::VertexType{ position, normal, uv });
			}

			DKVector<uint32_t> sourceIndices;
			if (primitive.indices >= 0)
			{
				const AccessorSpan indexAccessor = GetAccessorSpan(model, primitive.indices);
				if (indexAccessor.componentCount != 1)
					Fail("Index accessor는 SCALAR 타입이어야 합니다.");

				sourceIndices.resize(indexAccessor.count);
				for (size_t i = 0; i < indexAccessor.count; ++i)
				{
					const uint32_t index = ReadIndex(indexAccessor, i);
					if (index >= vertexCount)
						Fail("IndexBuffer가 VertexBuffer 범위를 벗어났습니다.");

					sourceIndices[i] = index;
				}
			}
			else
			{
				//// index accessor가 없는 glTF primitive도 유효합니다.
				//sourceIndices.resize(vertexCount);
				//std::iota(sourceIndices.begin(), sourceIndices.end(), 0u);
				Fail("");
			}

			result.indices = ConvertToTriangleList( sourceIndices, primitive.mode);

			if (primitive.material >= 0)
			{
				if (primitive.material >= static_cast<int>(model.materials.size()))
					Fail("primitive가 존재하지 않는 material을 참조합니다.");

				// CpuModel::materials[0]은 기본 material.
				result.materialIndex = static_cast<uint32_t>(primitive.material + 1);
			}
			else
			{
				//result.materialIndex = 0;
				Fail("");
			}

			return result;
		};

		SceneObject newSceneObject;

		for (uint32_t meshIndex = 0; meshIndex < static_cast<uint32_t>(gltfModel.meshes.size()); ++meshIndex)
		{
			const tinygltf::Mesh& mesh = gltfModel.meshes[meshIndex];

			StaticMeshComponent* staticMeshComponent = dk_new StaticMeshComponent;
			newSceneObject.addComponent(staticMeshComponent);
			staticMeshComponent->set_modelPath(mesh.name);
			staticMeshComponent->set_modelPropertyPath("GLTF_ModelProeprty");

			DKVector<StaticMeshModel::SubMeshType> subMeshArr;
			subMeshArr.reserve(static_cast<uint32_t>(mesh.primitives.size()));
			for (uint32_t primitiveIndex = 0; primitiveIndex < static_cast<uint32_t>(mesh.primitives.size()); ++primitiveIndex)
			{
				const tinygltf::Primitive& primitive = mesh.primitives[primitiveIndex];
				CpuPrimitive<StaticMeshModel::SubMeshType::VertexType> cpuPrimitive = ConvertPrimitive(gltfModel, primitive, meshIndex, primitiveIndex);
				//result.model.primitives.push_back();

				VertexBufferViewRef vertexBufferView;
				const bool vertexBufferSuccess = renderModule.createVertexBuffer(cpuPrimitive.vertices.data(), sizeof(StaticMeshModel::SubMeshType::VertexType), cpuPrimitive.vertices.size(), vertexBufferView, L"StaticMesh_VertexBuffer");
				if (vertexBufferSuccess == false)
					return nullptr;

				IndexBufferViewRef indexBufferView;
				const bool indexBufferSuccess = renderModule.createIndexBuffer(cpuPrimitive.indices.data(), cpuPrimitive.indices.size(), indexBufferView, L"StaticMesh_IndexBuffer");
				if (indexBufferSuccess == false)
					return nullptr;

				Material* newMaterial = Material::createMaterial(materialDefinitionArr[i]);
				if (newMaterial == nullptr)
					return nullptr;

				StaticMeshModel::SubMeshType subMesh(
					DK::move(cpuPrimitive.vertices), DK::move(cpuPrimitive.indices),
					DK::move(vertexBufferView), DK::move(indexBufferView),
					newMaterial
				);
				subMeshArr.push_back(DK::move(subMesh));
			}

			// TODO:  For문이 중간에 실패하면 container에서 지워줘야하는데, 현재는 GC가 없으니 일단 두자
			ResourceManager& resourceManager = DuckingEngine::getInstance().GetResourceManagerWritable();
			auto insertResult = resourceManager._staticMeshModelContainer.insert(DKPair<DKString, StaticMeshModelRef>(path, dk_new StaticMeshModel(DK::move(subMeshArr))));
			if (insertResult.second == false)
			{
				DK_ASSERT_LOG(false, "Container Insert Failed!");
				return nullptr;
			}
		}

		if (createSceneObjectConstantBuffer(newSceneObject) == false)
			return nullptr;

		const uint32 key = static_cast<uint32>(_sceneObjectContainer.size());
		auto success = _sceneObjectContainer.insert(DKPair<uint32, SceneObject>(key, DK::move(newSceneObject)));
		if (success.second == false)
			return nullptr;

		return &success.first->second;
	}

	SceneObject* SceneObjectManager::createSceneObject(const DKString& modelPath, const DKString& modelPropertyPath)
	{
		SceneObjectManager& thisXXX = DuckingEngine::getInstance().GetSceneObjectManagerWritable();

		SceneObject newSceneObject;
		StaticMeshComponent* staticMeshComponent = dk_new StaticMeshComponent;
		newSceneObject.addComponent(staticMeshComponent);
		staticMeshComponent->set_modelPath(modelPath);
		staticMeshComponent->set_modelPropertyPath(modelPropertyPath);
		if (staticMeshComponent->loadResource() == false)
			return nullptr;	

		if (createSceneObjectConstantBuffer(newSceneObject) == false)
			return nullptr;

		const uint32 key = static_cast<uint32>(thisXXX._sceneObjectContainer.size());
		auto success = thisXXX._sceneObjectContainer.insert(DKPair<uint32, SceneObject>(key, DK::move(newSceneObject)));
		if (success.second == false)
			return nullptr;

		return &success.first->second;
	}

	const AppearanceDataRef SceneObjectManager::loadCharacter_LoadAppearanceFile(const char* appearancePath)
	{
		using FindResult = DKHashMap<DKString, AppearanceDataRef>::iterator;
		FindResult findResult = _appearanceRawContainers.find(appearancePath);
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
		const int modelDataCount = DK::atoi(modelDataArrNode->ToElement()->Attribute("Count"));
		modelDataArr.reserve(modelDataCount);
		for (TiXmlNode* modelDataNode = modelDataArrNode->FirstChild(); modelDataNode != nullptr; modelDataNode = modelDataNode->NextSiblingElement())
		{
			const TiXmlElement* modelNode = modelDataNode->FirstChildElement("Model");
			const char* modelPath = modelNode->GetText();
			const TiXmlElement* modelPropertyNode = modelDataNode->FirstChildElement("ModelProperty");
			const char* modelPropertyPath = modelPropertyNode->GetText();

			modelDataArr.push_back(AppearanceData::ModelData(modelPath, modelPropertyPath));
		}

		auto insertResult = _appearanceRawContainers.insert(
			DKPair<const char*, AppearanceDataRef>(appearancePath, dk_new AppearanceData(skeletonPath, animationSetPath, DK::move(modelDataArr)))
		);

		return insertResult.first->second;
	}
	SceneObject* SceneObjectManager::createCharacter(const char* appearancePath)
	{
		SceneObjectManager& thisXXX = DuckingEngine::getInstance().GetSceneObjectManagerWritable();

		const AppearanceDataRef& appearanceData = thisXXX.loadCharacter_LoadAppearanceFile(appearancePath);
		if (appearanceData == nullptr)
			return nullptr;

		SceneObject newSceneObject;
		const uint32 skinnedMeshCount = static_cast<const uint32>(appearanceData->_modelDataArr.size());
		for (uint32 i = 0; i < skinnedMeshCount; ++i)
		{
			SkinnedMeshComponent* skinnedMeshComponent = dk_new SkinnedMeshComponent;
			newSceneObject.addComponent(skinnedMeshComponent);
			skinnedMeshComponent->set_modelPath(appearanceData->_modelDataArr[i]._modelPath);
			skinnedMeshComponent->set_modelPropertyPath(appearanceData->_modelDataArr[i]._modelPropertyPath);
			if (i == 0)	// MainSkinnedMesh
			{
				skinnedMeshComponent->set_skeletonPath(appearanceData->_skeletonPath);
				skinnedMeshComponent->set_animationPath(appearanceData->_animationSetPath);	// #todo- 나중에 AnimationSet으로 변경(AnimationController 만든 후에)
			}
			if (skinnedMeshComponent->loadResource() == false)
				return nullptr;
		}

		if (createSceneObjectConstantBuffer(newSceneObject) == false)
			return nullptr;

		const uint32 key = static_cast<uint32>(thisXXX._characterSceneObjectContainer.size());
		auto success = thisXXX._characterSceneObjectContainer.insert(DKPair<uint32, SceneObject>(key, DK::move(newSceneObject)));
		if (success.second == false)
			return nullptr;

		return &success.first->second;
	}

	void SceneObjectManager::update(float deltaTime)
	{
		for (auto iter = _characterSceneObjectContainer.begin(); iter != _characterSceneObjectContainer.end(); ++iter)
		{
			SceneObject& sceneObject = iter->second;
			uint32 componentCount = static_cast<uint32>(sceneObject._components.size());
			for (uint32 i = 0; i < componentCount; ++i)
				sceneObject._components[i]->update(deltaTime);
		}
	}
}
