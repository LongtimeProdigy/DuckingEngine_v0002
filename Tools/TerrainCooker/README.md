# Terrain Section 스트리밍

현재 구현 범위는 카메라 주변 3×3 Section의 저해상도 Height·Normal과 근거리 Chunk 스트리밍이다. 전체 월드 Proxy 메시·머티리얼 베이크와 Proxy 경계 전환은 후속 단계이며 아직 구현하지 않았다. 현재 3×3 밖과 로딩이 완료되지 않은 Section은 Terrain·Grass 패스에서 제외한다.

## 데이터 생성

기존 Heightmap → `.dkterrain` 생성 명령은 그대로 사용한다. 생성된 Archive를 Chunk 경계에 맞춰 Section으로 분할한다.

```text
TerrainCooker.exe --split Resource/Scenes/Cooked/forest_valley_extended.dkterrain Resource/Scenes/Cooked/forest_valley_world 2048
```

출력 디렉터리는 새 경로여야 한다. 분할 과정은 원본 Chunk의 Height·Normal·LOD 데이터 블록을 복사하므로 재양자화하지 않는다. 인접 Section의 경계 샘플과 법선도 원본과 동일하다. 원본 `.dkterrain`의 v3 바이너리 배치는 바뀌지 않았다.

```text
forest_valley_world/
    terrain.xml
    section_0_0.dkterrain
    section_1_0.dkterrain
    ...
```

`terrain.xml`은 전체 샘플 수, 샘플 간격, 공통 높이 변환 범위, Chunk·Section 크기, 저해상도 LOD와 각 Section의 좌표·Archive 상대 경로를 저장한다. Section ID는 `Z * SectionCountX + X`다. 전체 지형의 X/Z 셀 수(샘플 수 - 1)는 Section 한 변의 셀 수로 나누어떨어져야 한다. Cooker와 런타임 모두 나머지가 있는 입력을 거부하며 모든 Section은 같은 크기다. Section 내부 샘플 수는 셀 수보다 하나 많으며, 경계 샘플은 양쪽 Section에 들어 있다.

현재 자산은 8193×8193 샘플, 0.5m 간격, 2048셀 Section이다. 전체 크기는 4096×4096m, Section 한 변은 1024m이며 4×4개로 나뉜다. Section 저해상도 LOD는 3으로, 기본 간격이 4m인 257×257 Height·Normal을 사용한다.

레벨의 Terrain 설정 변경:

수정 전:

```xml
<Terrain Archive="Scenes/Cooked/forest_valley_extended.dkterrain"
         ModelProperty="Scenes/ModelProperty/TerrainClipMap.xml"
         HeightTexture="Scenes/textures/forest_valley_extended_height_8k.png"
         OriginX="-1024" OriginZ="-1024" />
```

수정 후:

```xml
<Terrain World="Scenes/Cooked/forest_valley_world/terrain.xml"
         ModelProperty="Scenes/ModelProperty/TerrainClipMap.xml"
         OriginX="-1024" OriginZ="-1024" />
```

## 프레임 흐름

1. CPU에서 카메라의 월드 위치를 Terrain 원점 기준 double 좌표로 바꾼다.
2. `floor(camera / sectionWorldSize)`로 중심 Section을 정하고 주변 3×3을 선택한다. 월드 밖 카메라를 가장자리 Section으로 강제 이동시키지 않는다.
3. 작업 스레드가 대상 Section의 Archive 디렉터리와 저해상도 Height·Normal을 읽는다. 범위에서 나간 Archive는 해제한다.
4. 렌더링 스레드가 완료된 저해상도 데이터를 GPU 3×3 atlas 슬롯에 올린다. 슬롯은 `(SectionX % 3, SectionZ % 3)`으로 재사용하며, 메타데이터 좌표가 일치할 때만 유효하다. Section 업로드는 프레임당 최대 2개다.
5. 근거리 Chunk 요청은 `(Section ID, LOD, X, Z)`로 식별한다. 모든 Section이 LOD당 72개 캐시 슬롯을 공유한다. Chunk 업로드는 프레임당 최대 4개다.
6. 페이지 테이블은 Section 슬롯·LOD별 주소 영역을 구분하고, 실제 Height·Normal 캐시는 공유한다. 불필요해진 요청과 늦게 완료된 결과는 현재 활성 집합을 기준으로 제외한다.
7. 셰이더는 Anchor 상대 위치에서 정수 샘플 좌표를 복원하고 Section을 찾는다. Section 저해상도를 먼저 샘플링한 뒤, 상주하는 Chunk의 상세 LOD를 거리·로딩 가중치로 혼합한다. 인접 Section의 Chunk도 경계 가중치 계산에 포함한다.
8. Terrain과 Grass가 같은 Section 선택과 Height 샘플링 함수를 사용한다.

공유 경계 정점은 양쪽 Section이 소유한다. 기본 선택 Section이 비상주 상태라면 같은 경계를 소유한 상주 Section에서 해당 경계 샘플을 읽는다.

현재 월드는 직사각형 Section 격자이며, 모든 Section은 샘플 간격·Chunk 크기·높이 변환 범위를 공유한다. Section 크기는 Chunk 크기 이상의 2의 거듭제곱이다. 가장 큰 상세 가상 타일도 Section 안에 들어가야 한다. GPU texture 크기와 Clipmap의 3×3 영역 커버 여부는 초기화 위치에서 ASSERT로 검증한다.

## 표준 API 사용 위치

- `TerrainWorld.cpp`, `TerrainStreaming.cpp`, `TerrainData.cpp`: 정렬·검색 알고리즘과 정수·double 수학은 기존 Terrain 코드와 같이 `std::`를 유지했다. 공용 `Math`의 min/max/clamp/floor는 float 전용이라 월드 좌표와 정수 크기를 전달하면 정밀도가 손실되며, 독립 Cooker에서도 사용할 수 없다. 대응되는 공용 정밀도 보존 래퍼가 필요하다.
- `TerrainStreaming.h/.cpp`: 스레드·mutex·condition_variable은 프로젝트 규칙대로 명시적인 `std::`를 유지한다.
- 새 Windows API 직접 호출은 추가하지 않았다.
