# DuckingEngine Blender Exporters

Blender 5.2 이상에서 사용하는 통합 애드온. 최소 지원 버전은 5.2이며 이전 버전 호환 코드는 포함하지 않는다.

## 설치와 사용

1. `python build_addon.py`로 `dist/dk_exporter.zip`을 생성한다.
2. Blender **Preferences > Add-ons > Install from Disk**에서 ZIP을 설치하고 활성화한다.
3. Object Mode에서 메시를 선택하고 **File > Export > DuckingEngine** 항목을 사용한다.
4. 캐릭터는 **Character Bundle**로 `.dc`, `.skeleton.xml`, `.animation.xml`을 함께 출력한다.
   Skeleton/Animation만 출력할 때는 Armature 또는 연결된 메시를 선택한다.

기존 파일과 정점 레이아웃은 같지만 본 정렬과 좌표 변환 규약을 수정했다.
**기존 `.dc`에 새 Skeleton/Animation을 혼합하지 말고 세 파일을 함께 다시 출력한다.**
엔진의 `ModelProperty`는 출력 SubMesh 수와 순서를 맞춰 별도로 지정한다.
기존 엔진 리소스는 자동으로 덮어쓰지 않는다.

## 파일 구성

| 파일 | 역할 |
|---|---|
| `dk_exporter/static_mesh_exporter.py` | StaticMesh 추출 및 `.dm` export |
| `dk_exporter/skinned_mesh_exporter.py` | SkinnedMesh 추출 및 `.dc` export |
| `dk_exporter/skeleton_exporter.py` | Rest pose Skeleton 추출 및 `.xml` export |
| `dk_exporter/animation_exporter.py` | 본 애니메이션 샘플링 및 `.xml` export |
| `dk_exporter/character_exporter.py` | 메시·Skeleton·Animation 일괄 export |
| `dk_exporter/mesh.py` | 공통 메시·UV·노멀·가중치 수집 |
| `dk_exporter/common.py` | 좌표 변환·씬 상태 보존·공통 export 옵션 |
| `dk_exporter/format.py` | 바이너리·XML 직렬화 및 파일 저장 |
| `dk_exporter/operators.py` | 익스포터 등록 및 메뉴 연결 |
| `dk_exporter/__init__.py` | 애드온 정보 및 진입점 |

파일을 분리해도 하나의 ZIP으로 설치하며, 각 익스포터는 기존 메뉴에서 개별 실행한다.

## 데이터 규약

- Little endian. `SubMeshCount`, 각 SubMesh의 `IndexCount + uint32[] + VertexCount + Vertex[]`.
- Static: `float3 position + float3 normal + float2 uv` (32 bytes).
- Skinned: Static + `uint32[4] boneIndices + float[4] weights` (64 bytes).
- 위치: `B = uniformScale(scene.scale_length * Unit Multiplier) * swapYZ`.
  메시에는 `B * object.matrix_world`를 적용한다. 스킨 메시와 Skeleton은 씬 시작 프레임을 기준으로 한다.
- 본 global: `G_engine = B * G_blender * inverse(B)`.
  자식 local은 `inverse(parentGlobal) * childGlobal`. 루트에도 같은 축/단위 규칙을 적용한다.
- Skeleton은 `Bone.matrix_local`의 rest pose, Animation은 평가된 `PoseBone.matrix`를 사용한다.
  행렬을 TRS로 표현할 수 없는 shear, 0 scale은 오류로 보고한다.
- 본 인덱스: 모든 본을 이름순 DFS로 순회하며 부모를 먼저 기록한다. 최대 256개.
  가중치는 deform 본만 사용하며 큰 값 4개를 선택하고 정규화한다.
  비본 Vertex Group은 무시하고, 가중치가 없는 사용 정점은 오류로 보고한다.
- 면 코너의 UV/분리 노멀을 보존하도록 출력 정점을 분리한다. 노멀은 역전치 변환 후 정규화한다.
  UV의 V를 뒤집고, 변환의 determinant가 음수면 삼각형 winding을 뒤집는다.
- 메시 이름순으로 SubMesh를 기록한다. 기본값은 오브젝트당 하나이며,
  **Split by Material**을 켜면 각 오브젝트의 사용 material slot 인덱스순으로 분리한다.
- Animation은 씬 프레임 구간(양 끝 포함)을 **30 FPS**로 재샘플링한다.
  원본 길이 `(end-start+1)/sourceFPS`를 1/30초 단위로 올림하고 마지막 소스 프레임 이후는 유지한다.
  현재 Action/NLA/Constraint의 평가 결과를 베이크한다.
- XML 속성은 따옴표와 escaping을 적용한다. Quaternion 순서는 `x y z w`.

원본 메시, UV, 선택, 현재 프레임과 Armature의 Pose/Rest 상태를 보존한다.
메시 Modifier는 임시 평가 메시에서 적용한다. UV가 없으면 `(0,0)`을 기록한다.

## 표현 범위

엔진은 본당 TRS와 선형 스키닝 4개 영향만 지원한다. Preserve Volume(Dual Quaternion),
Envelope, Armature Modifier 마스크, 여러 Armature Modifier, segmented B-Bone은 오류로 보고한다.
스킨 메시의 Armature에 대한 오브젝트 상대 변환은 애니메이션 동안 고정되어야 한다.
메시 자체의 독립적인 오브젝트 애니메이션, Shape Key 애니메이션, 재질/텍스처 파일은 이 포맷에 포함하지 않는다.
스킨 메시의 Shape Key는 reference frame의 평가 결과가 기준 메시가 된다.

엔진의 `SkinningLimits.h`/`Skeleton.h`/스키닝 셰이더는 본 제한을 256개로 공유한다.
`SkinnedMeshComponent`는 같은 크기의 행렬 버퍼를 할당하고, 로더는 본 인덱스를 검사한다.
`Transform::tofloat4x4`는 회전 행렬의 각 행 전체에 스케일을 곱한다.

## 검증 상태

2026-10-21에 Blender 5.2.2 LTS에서 설치·활성화 및 5개 익스포터의 등록·실행을 확인했다.
재질 분리·Modifier 옵션을 포함한 출력 9개 파일이 분리 전과 바이트 단위로 일치했다.
엔진 Debug x64 빌드, 스키닝 셰이더 컴파일 및 TRS 수치 검증은 2026-10-20에 완료했다.
GPU 화면과 실제 게임 내 애니메이션 재생은 미검증이다.
