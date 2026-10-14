## 시스템 문서

- [Ocean 시스템 — 위상·스펙트럼·IFFT의 수학과 구현](docs/Ocean.md)

## Renderer (DX12, Windows)
- RenderPass, Pipeline과 Resource Binding을 XML을 이용하여 지정
- Material
- Bindless Resource
- PushConstant
- Deferred Rendering
- Runtime Shader Reloading

### 렌더러 UI

- 기본 화면에는 FPS를 보여주는 작은 상태 표시줄만 나타난다.
- `Inspector`로 우측 설정 패널을 열고 `Scene / Light / Image / Debug` 탭에서 기능을 선택한다. 상세 항목은 접이식 섹션으로 정리되어 있다.
- `Close inspector` 또는 패널의 X로 설정을 닫는다. `F1`은 UI 전체를 숨기거나 다시 표시한다.
- 설정 패널의 제목 표시줄을 메인 창 밖으로 드래그하면 별도 Windows 창으로 분리된다. 다른 모니터로 이동하거나 크기를 조절할 수 있다.
- 메인 창 안으로 드래그한 뒤 도킹 표시 위에 놓으면 다시 붙일 수 있다. 패널 위치·크기·도킹 상태는 `imgui.ini`에 저장된다.
- `... > Bring inspector back`은 설정 패널을 메인 창의 기본 위치로 되돌린다. 내용이 길면 스크롤할 수 있다.
- Dear ImGui 코어와 Win32/DX12 백엔드는 공식 `v1.92.6-docking`으로 고정되어 있다. 업그레이드 정보는 `lib/imgui/UPSTREAM.md`를 참고한다.

## Graphics
- Rayleigh scattering
- Mie scattering
- Simple Ocean

### 실제 크기 지구의 대기 테스트

행성 반지름은 6,360,000m, 중심은 (0, -6360000, 0), 대기 두께는 30,000m이다.
별도의 축소 행성 모드 없이 같은 행성과 대기를 지표·궤도 카메라로 관찰한다.
기본 실행과 `Orbit`은 현재 태양 방향을 기준으로 지구와 태양을 한 화면에 배치한다.
행성 중심에서 대기 반지름의 4배 떨어진 위치에 카메라를 놓고 지구 중심과 태양 사이를 바라본다.
원점 부근의 지표에서는 기존 지형·바다·오브젝트를 렌더링하고, 궤도에서는 대기 패스의 구 교차로 전체 표면을 표시한다.
기존 평면 지형은 구면 전체 지형이 아니므로 카메라가 북극 지표 패치에서 수평 50km 또는 고도 10km 밖이면 계산·렌더링을 건너뛴다.

- `Orbit` / `Surface`: 카메라 위치와 방향만 전환한다. 대기·태양 설정은 유지된다. 태양 각도를 변경한 뒤 Orbit을 누르면 구도를 다시 맞춘다.
- `Surface`: 기존 지표 장면의 (3, 23, -50)m 부근으로 이동한다.
- `Reset Earth atmosphere`: 지구 반지름·중심·대기 두께와 Rayleigh/Mie 스케일 높이(7994m/1200m)를 복원한다.
- `Air thickness`, `Rayleigh height`, `Mie height`, `Density multiplier`: 미터 단위의 대기 형상과 산란을 개별 조절한다. 반지름에 따른 beta 역배율 보정은 적용하지 않는다.
- `Sun angle (deg)`: 태양 각도를 바꿔 낮·일몰·밤의 행성 차폐를 확인한다.

대기 계산은 카메라 기준 상대 좌표를 사용하고, CPU에서 double로 구한 카메라 고도로 지표 근처의 높이 정밀도를 보완한다.
카메라 이동 속도와 near/far 범위는 고도에 따라 자동으로 증가한다. Shift/CapsLock 가속은 유지한다.
고도에 따른 속도·클리핑과 구 표면 진입 제한은 SceneRenderer의 대기 장면 정책이다. 공용 Camera는 행성 정보를 저장하지 않고 외부에서 지정한 이동 속도와 near/far 값을 사용한다.
실제 지형 깊이가 있는 픽셀은 그 표면까지 대기를 적분하고, 지형이 없는 픽셀은 기준 구 표면으로 보완한다.
행성 표면은 단순 Lambert 색상이며, 표면에 입사하는 태양광과 카메라로 나오는 빛 모두 대기 감쇠를 적용한다.
카메라는 기준 구 표면의 최소 2m 위로 제한한다. 실제 산·건물의 충돌 처리는 별도이다.

태양은 기본 각반경 0.27도의 흰 원반이며 픽셀 각도 폭으로 경계를 부드럽게 처리한다.
`Irradiance`는 대기와 테스트 지표를 비추는 빛의 세기이다.
원반 radiance는 `irradiance / (pi * sin(각반경)^2)`로 계산하고, `Disk brightness`로 원반의 밝기만 추가 조절한다(기본 1).
일몰 색은 태양색을 바꾸지 않고 대기 투과율로 계산한다.

- `Exposure (EV)`: 양수 1마다 화면에 적용하는 노출이 2배가 된다.
- `Image > Bloom > Intensity`: HDR 밝은 부분의 화면 번짐 강도이며 0이면 bloom 계산을 건너뛴다.
- `Image > Bloom > Radius (px)`: 수평·수직 Gaussian 필터의 반경(1~32픽셀).
- `Image > Bloom > Threshold`: 노출 적용 후 HDR 밝기 기준의 bloom 시작점.

중간 렌더 타깃은 RGBA32 float이며 대기 합성 → 수평 bloom → 수직 bloom·HDR 합성 → 노출·ACES 톤매핑 순서로 처리한다.
Bloom.hlsl의 PSHorizontal/PSVertical을 각각 독립 패스로 실행한다. RT1은 원본 HDR, RT0은 수평 bloom, RT2는 수직 bloom까지 합성한 HDR을 저장한다.
GBuffer.hlsl은 렌더러가 지정한 HDR 텍스처에 노출·톤매핑만 적용한다. Bloom이 꺼져 있으면 원본 RT1, 켜져 있으면 합성된 RT2를 사용한다.
Bloom 가중치·필터·합성은 Bloom.hlsl, 톤매핑은 GBuffer.hlsl에 둔다.
필터는 지원 반경 안에 약 6 sigma를 배치하고 경계 가중치를 0으로 보정한다. 태양의 높은 HDR 밝기가 기존 3-sigma 필터의 사각형 절단 경계를 드러내던 문제를 줄인다.
Bloom은 카메라의 눈부심 표현이고, 대기 중 태양 주변의 산란은 Rayleigh/Mie 계산에서 별도로 나온다.
지표 태양광 감쇠는 현재 기준 구 표면에 적용하며, 기존 지형 재질의 직접광 계산은 별도이다.

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
