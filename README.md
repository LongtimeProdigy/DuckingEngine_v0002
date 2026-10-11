## 시스템 문서

- [Ocean 시스템 — 위상·스펙트럼·IFFT의 수학과 구현](docs/Ocean.md)

## Renderer (DX12, Windows)
- RenderPass, Pipeline과 Resource Binding을 XML을 이용하여 지정
- Material
- Bindless Resource
- PushConstant
- Deferred Rendering
- Runtime Shader Reloading

## Graphics
- Rayleigh scattering
- Mie scattering
- Simple Ocean

Atmosphere의 빛 전달 원리, Rayleigh/Mie 산란, ray marching, 장면 합성과 실시간 최적화는 [Atmosphere 렌더링 기술 이해 노트](docs/Atmosphere-System.md)에 정리되어 있다. [Notion 문서](https://app.notion.com/p/3f58d819296a81ecaaf3d28b3a270625?pvs=204)에서도 읽을 수 있다.

## Terrain
- ClipMap Terrain
- [ClipMap 시스템 이해 문서](docs/ClipMap.md)

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
#### **Ocean**
<img width="2538" height="1429" alt="image" src="https://github.com/user-attachments/assets/8fd3cc27-4c07-412d-a0da-e47620fe7a83" />

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
