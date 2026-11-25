# Terrain Section 스트리밍

카메라 주변 3×3 Section의 저해상도 Overview와 근거리 Tile을 스트리밍한다. 전체 월드 Proxy 메시·머티리얼 베이크는 후속 단계다. GPU Height·Normal atlas, 페이지 테이블과 셰이더의 샘플링 방식은 유지한다.

## 코드 위치

- `Terrain.h/cpp`: 파일 포맷, 배치 계산·검증, Archive·World 로딩, Section·Tile 선택과 Streamer.
- `Tools/TerrainCooker/TerrainCooker.cpp`: 높이맵 입력, 높이·법선 생성, Archive 저장, Section 분할, 생성 결과 검증과 CLI. 쿠킹 전용 타입과 함수는 이 파일 내부에서만 사용한다.
- `Resource/Material/Terrain.hlsl`: 높이·법선 샘플링과 Terrain 표면 재질 계산.
- `Resource/Material/TerrainShared.h`: CPU·GPU 공통 상수와 페이지 테이블 주소 수식.

## 데이터 생성

```text
TerrainCooker.exe <8-bit or 16-bit grayscale image> <output.dkterrain> [sampleWorldSpacing baseWorldHeight worldHeightRange chunkCellCountPerSide [overviewLod]]
TerrainCooker.exe --split <archive.dkterrain> <outputDirectory> <sectionCells>
TerrainCooker.exe --verify <archive.dkterrain>
TerrainCooker.exe --verify-world <terrain.xml>
```

기본값은 샘플 간격 0.5m, 높이 기준값 -16m, 높이 범위 128m, Chunk 128셀, Overview LOD3이다. `--verify`와 `--verify-world`는 Debug 빌드에서 모든 저장 블록을 실제 런타임 로더로 읽는다.

```text
TerrainCooker.exe Platform/height_16k.png Platform/forest_valley.dkterrain 0.5 -16 768 128 3
TerrainCooker.exe --split Platform/forest_valley.dkterrain Platform/forest_valley_world 2048
TerrainCooker.exe --verify-world Platform/forest_valley_world/terrain.xml
```

출력 Archive와 출력 디렉터리는 새 경로여야 한다. 입력 이미지는 셀 수가 모든 상세 LOD Tile 크기와 Overview 샘플 간격으로 나누어떨어져야 한다. 잘못된 크기를 자동 보정하지 않는다.

`--split`은 원본 Archive의 LOD0 Tile에서 전체 높이 원본을 복원한 뒤 Section들을 다시 쿠킹한다. 높이 정수는 재양자화하지 않는다. 법선 계산은 Section 경계를 넘어 원본 높이맵을 참조하므로, 같은 원본에서 나눈 인접 Section의 높이·법선이 일치한다.

## DKTRN004 파일 배치

```text
ArchiveHeader                         64 bytes
ArchiveBlockInfo[blockCount]          40 bytes per block
Overview Height                       sampleCountX × sampleCountZ × 2 bytes
Overview Normal                       sampleCountX × sampleCountZ × 4 bytes
LOD0 Tile(0, 0) Height / Normal
LOD0 Tile(1, 0) Height / Normal
...
LOD1 Tile(0, 0) Height / Normal
...
LOD(overviewLod - 1) Tiles
```

디렉터리 첫 항목은 Section 전체 Overview다. 나머지는 LOD가 증가하는 순서이며, 같은 LOD에서는 Z행 안에서 X가 증가한다. 모든 블록은 높이 `uint16` 배열과 법선 XZ `int16[2]` 배열을 연속 저장한다. 블록 내부도 Z행 안에서 X가 증가한다. 인접 Tile의 경계 샘플은 각각의 블록에 중복 저장한다.

`ArchiveHeader::_overviewLod`는 파일에 저장한 Overview LOD이며, `_blockCount`는 Overview를 포함한 전체 블록 수다. `ArchiveBlockInfo`는 Tile 인덱스, LOD, 가로·세로 샘플 수, 높이·법선 배열의 파일 바이트 offset을 저장한다. 구조체 크기와 멤버 배치는 컴파일 시 검증한다.

현재 Section 2048셀, Chunk 128셀, Overview LOD3 기준:

| 대상 | 개수 | 블록당 샘플 크기 | LOD0 기준 한 변의 셀 수 |
|---|---:|---:|---:|
| Overview | 1 | 257×257 | 2048 |
| LOD0 Tile | 16×16 | 129×129 | 128 |
| LOD1 Tile | 8×8 | 129×129 | 256 |
| LOD2 Tile | 4×4 | 129×129 | 512 |

Section마다 총 337개 블록을 저장한다. 더 높은 LOD의 청크별 Pyramid는 저장하지 않는다. v3 파일은 런타임에서 지원하지 않으며 새 cooker로 다시 생성해야 한다.

## 로딩 흐름

1. `TerrainWorld::open`이 Manifest의 전체 샘플 수·샘플 간격·높이 범위·Section 격자·Overview LOD를 `TerrainWorld`에 직접 읽는다. 바이너리 파일의 `ArchiveHeader`는 Archive에만 둔다.
2. `Streamer`가 주변 Section의 `Archive::open`을 호출한다. Header와 디렉터리를 읽어 블록 수, 크기, LOD, 순서, 파일 offset과 파일 길이를 검증한다.
3. `loadOverview`는 LOD 인자 없이 첫 번째 블록을 선택한다. `loadVirtualTile`은 LOD별 시작 인덱스와 Tile XZ로 블록을 직접 선택한다.
4. 공통 `loadBlock`이 시작 위치로 한 번 seek하고 높이·법선을 각각 한 번씩 read한다. 행별 파일 읽기와 청크 조립은 없다.
5. `TerrainSampleData`는 높이·법선 배열만 전달한다. Overview 크기와 LOD는 World의 공통 설정에서 가져와 배열 크기를 검증하고 GPU atlas 슬롯에 업로드한다. Section별 GPU 메타데이터는 Section X/Z와 상주 여부만 저장하며, 크기는 공통 상수를 사용한다.

현재 실행 레벨은 `Resource/Scenes/PracticeTerrain/Cooked/Sections_2048/terrain.xml`이다. 전체 16384×16384셀, 샘플 간격 0.5m로 8192×8192m를 표현한다. 최소 샘플 좌표는 (-8192, -8192)이며, Section 8×8개와 Section당 1024m를 사용한다.

`Platform/height_16k.png`는 `Resource/Scenes/textures/forest_valley_extended_height_8k.png`의 8193×8193 Gray16 샘플 사이에 쌍선형 보간한 중간 샘플을 채워 만든 16385×16385 Gray16 이미지다. 월드 높이는 `-16 + 768 × (샘플 / 65535)`m이며, 현재 높이 범위는 약 0.38~740.83m다. `--split`으로 만든 Section (X, Z)는 기존 manifest의 같은 좌표에 배치하고 `HeightScale`을 768로 맞춘다. 나무와 목초의 `Instance Y`는 새 높이맵의 해당 X/Z를 샘플링해 갱신한다.

## 공용 API 사용

파일 처리는 `FileReader`, `FileWriter`, `FileUtil`을 사용한다. 기존 정수 min, 부동소수점 수학·검증, 바이너리 비교·타입 검사는 `std::`를 유지한다. 대응 정밀도를 보존하는 공용 수학·메모리 래퍼가 아직 없다. 새로운 Windows API 직접 호출은 추가하지 않았다. Cooker의 기존 WIC·COM 호출과 콘솔 출력은 유지한다.
