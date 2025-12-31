## Renderer (DX12, Windows)
- RenderPass, Pipeline과 Resource Binding을 XML을 이용하여 지정
- Material
- Bindless Resource
- PushConstant
- Deferred Rendering

## Graphics
- Rayleigh scattering

## Terrain
- ClipMap Terrain

## 그 외
- SkinnedMesh
- StaticMesh
- Animation
- Keyboard/Mouse, Pad 입력 지원

## 플러그인
- Blender StaticMesh Exporter
- Blender SkinnedMesh Exporter
- Blender Skeleton Exporter
- Blender Animation Exporter

## 결과
#### **RenderPass/Pipeline**
![RenderPass/Pipeline XML Definition](https://github.com/user-attachments/assets/0fc44f75-78bf-463b-afd3-6f5831429358)

-----
#### **Atmosphere Scattering**
![Rayleigh scattering](https://github.com/user-attachments/assets/97d5254b-ffe9-4f7f-aff6-d9f92c50e798)

-----
#### **Clipmap Terrain**
![Clipmap Terrain 1](https://github.com/user-attachments/assets/641e291f-7f51-4f78-bfcf-775b234f5cc3)
![Clipmap Terrain 2](https://github.com/user-attachments/assets/e1957ce8-df5a-4c62-9587-c3666e999a1a)

-----
## TODO
- RenderModule
  - Shader Reflection
  - Material/Pipeline Technique
- Graphics
  - PBR
  - Mie Scattering
  - Ocean, Water
  - Realtime Raytracing
  - Realtime Pathtracing(DI, GI, PT)
- Object
  - GLTF Model Loading
  - Foliage
  - Quad Tree Terrain
