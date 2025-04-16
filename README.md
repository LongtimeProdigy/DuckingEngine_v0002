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
**RenderPass/Pipeline XML 정의**
![image](https://github.com/user-attachments/assets/0fc44f75-78bf-463b-afd3-6f5831429358)
-----
**Atmosphere Scattering(Rayleigh scattering)**
![image](https://github.com/user-attachments/assets/d8922061-f769-4985-b10c-71cf0c0ac000)
-----
**Clipmap Terrain**
![image](https://github.com/user-attachments/assets/641e291f-7f51-4f78-bfcf-775b234f5cc3)
![image](https://github.com/user-attachments/assets/e1957ce8-df5a-4c62-9587-c3666e999a1a)
-----

## TODO
- RenderModule
  - Shader Reflection
  - Material/Pipeline Technique
- Graphics
  - Realtime Raytracing
  - Mie Scattering
  - PBR
  - Realtime Pathtracing
- Object
  - Foliage
