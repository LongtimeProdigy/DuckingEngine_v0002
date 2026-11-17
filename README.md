# DuckingEngine

Windows에서 C++과 DirectX 12로 개발 중인 액션 게임 엔진입니다. 실사 자연환경 렌더링을 목표로, **Forest Valley** 데모에서 지형 스트리밍, 절차적 식생, 대기 산란을 한 장면에 통합합니다.

![Forest Valley의 초지와 숲 경계. 전경의 잔디와 들꽃에서 뒤쪽 능선의 나무까지 이어지는 엔진 실행 화면](docs/images/forest-valley.png)

## 핵심 구현

### 지형 스트리밍과 절차적 잔디

Forest Valley는 **4.096 × 4.096 km** 월드를 16개 Section으로 나눕니다. 카메라 주변 3 × 3 Section의 저해상도 높이·법선과 가까운 Chunk의 상세 데이터를 비동기로 읽어 GPU 캐시에 올립니다. Geometry Clipmap은 LOD 경계에서 부모 격자로 모핑하고, 잔디 셰이더는 지형과 같은 높이 샘플링을 사용해 뿌리 위치를 계산합니다.

[스트리밍 구현](TerrainStreaming.cpp) · [Clipmap 셰이더](Resource/Material/TerrainClipMap.hlsl) · [잔디 셰이더](Resource/Material/GrassPatch.hlsl) · [Terrain Cooker 문서](Tools/TerrainCooker/README.md)

### 행성 규모 대기 렌더링

지표와 궤도에서 하나의 구형 대기를 관찰합니다. Rayleigh·Mie 산란을 계산하고, 대기 투과율과 다중 산란은 LUT에 저장해 재사용합니다. 카메라 상대 좌표와 CPU에서 double로 계산한 고도로 지표 근처의 정밀도를 확보합니다. Inspector의 `Surface`와 `Orbit`으로 시점을 전환하면 같은 태양 설정에서 장면을 비교할 수 있습니다.

[대기 셰이더](Resource/Material/Atmosphere) · [카메라와 패스 실행](SceneRenderer.cpp) · [대기 렌더링 노트](output/Atmosphere_Rendering_Notes_Consistent.pptx)

### XML 기반 DirectX 12 렌더러

렌더 타깃, 패스, 파이프라인과 리소스 바인딩을 XML에서 정의합니다. 프레임은 **G-buffer → 디퍼드 조명 → 대기 합성 → Bloom → 톤 매핑** 순서로 실행됩니다. Bindless 리소스와 런타임 셰이더 리로드를 지원합니다.

[렌더 패스 XML](Resource/RenderPass/RenderPassGroup.xml) · [DX12 렌더 모듈](RenderModule.cpp)

GPU Compute Shader로 스펙트럼을 갱신하고 IFFT로 수면을 생성하는 [Ocean](Resource/Material/Ocean)과 메시·스켈레톤·애니메이션을 내보내는 [Blender 애드온](DKBlenderAddnos/README.md)도 포함합니다.

## 빌드 및 Forest Valley 실행

Visual Studio 2022의 **C++를 사용한 데스크톱 개발** 워크로드와 Windows SDK, DirectX 12 지원 GPU가 필요합니다. [DXC 릴리스](https://github.com/microsoft/DirectXShaderCompiler/releases/tag/v1.10.2605.37)의 `dxc_2026_08_11.zip`에서 `bin/x64/dxcompiler.dll`과 `bin/x64/dxil.dll`을 추출해 저장소의 `lib/dxc_2026_08_11/bin/x64/`에 둡니다.

저장소 루트에서 Visual Studio Developer PowerShell로 빌드합니다. 첫 명령은 NuGet의 DirectX 12 Agility SDK도 복원합니다.

```powershell
msbuild .\DuckingEngine_v0002.sln /restore /p:RestorePackagesConfig=true /p:Configuration=Debug /p:Platform=x64
msbuild .\Tools\TerrainCooker\TerrainCooker.vcxproj /p:Configuration=Release /p:Platform=x64 /p:OutDir=../../Platform/Tools/TerrainCooker/ /p:IntDir=../../Platform/Tools/TerrainCooker/Intermediate/
```

데모가 사용하는 `Resource/Scenes/Cooked/`는 Git에서 제외됩니다. 저장소에 포함된 높이맵으로 아카이브를 만든 뒤 Section으로 분할합니다. `--split`의 출력 폴더는 아직 존재하지 않아야 합니다.

```powershell
$cooker = '.\Platform\Tools\TerrainCooker\TerrainCooker.exe'
$cooked = '.\Resource\Scenes\Cooked'
New-Item -ItemType Directory -Path $cooked -Force | Out-Null
& $cooker .\Resource\Scenes\textures\forest_valley_extended_height_8k.png "$cooked\forest_valley_extended.dkterrain" 0.5 -16 128 128
& $cooker --split "$cooked\forest_valley_extended.dkterrain" "$cooked\forest_valley_world" 2048
.\Platform\x64\Debug\Binaries\DuckginEngine_v0002.exe --resourcePath=Resource --level=Scenes/ForestValley.xml
```

실행 후 `Inspector`에서 대기·조명·이미지 설정을 조절할 수 있습니다. `F1`은 UI를 숨기거나 표시합니다.
