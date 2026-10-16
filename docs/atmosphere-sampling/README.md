# 대기 샘플링 단계별 변경

원래 저장소의 2026-10-15 작업 시작 시점에서 순서대로 적용한 diff입니다.
이미 적용된 변경이므로 아래 patch를 현재 파일에 다시 적용하지 마세요.

| 단계 | diff | 변경 내용 |
|---|---|---|
| 1 | [01-variable-intervals.patch](01-variable-intervals.patch) | 구간 경계와 실제 길이 사용. OPTIMZE_OPTICALDEPTH의 양쪽 분기 수정. |
| 2 | [02-adaptive-reference.patch](02-adaptive-reference.patch) | 중점 1개/반구간 2개의 오차 비교, 밀도 경계 분할, 한도 진단. |
| 3 | [03-transmittance-lut.patch](03-transmittance-lut.patch) | 512×128 광학 깊이 LUT, GPU 생성/캐시 무효화/조회와 직접 적분 대체 경로. |
| 4 | [04-cached-sampling.patch](04-cached-sampling.patch) | 부모 평가값 재사용, Production 초기 8구간, Reference 최소 32구간. |

## 화면에서 비교

Inspector의 Sampling quality에서 Atmosphere integration을 선택합니다.

- Production: LUT + 적응형 시선 적분. 기본 모드입니다.
- Direct reference: 태양 경로도 직접 적분합니다. 느린 비교용 모드입니다.
- Reference limits: 기준 적분에서 허용 오차를 충족하지 못한 픽셀은 빨강, 나머지는 녹색입니다. 실제 장면의 색이 아닙니다.
- Initial view/light intervals는 초기 구간 수입니다. 실제 평가 횟수는 세분 결과에 따라 증가합니다.
- Relative error는 국소 오차 추정 기준이며 최종 영상 전체 오차의 보장값이 아닙니다.
- Subdivision depth는 최대 분할 깊이입니다. 한도에 도달하면 반구간 추정치를 사용하고 경로를 버리지 않습니다.
- 더 엄격한 기준 비교는 Relative error=0.0002, Subdivision depth=10으로 합니다.

## 구현 범위

기존 저장소에 들어 있던 구형 지표 배경, 행성 그림자, 카메라 고도 제한은 이번 작업에서 변경하지 않았습니다.
밀도 구간 경계와 LUT 자체는 불투명한 지표를 만들지 않습니다. 기준 반지름 아래는 LUT 대신 직접 적분합니다.
가스 행성을 실제로 표시하려면 기존 지표/카메라 정책을 별도로 변경해야 합니다.

LUT는 태양 투과율을 계산하기 위한 Rayleigh/Mie 광학 길이를 RG 채널에 저장합니다.
B 채널은 적분 한도 도달 여부입니다. 보간에 쓰는 texel이 한도에 걸리면 직접 적분합니다.
반지름, 밀도 높이, 산란 배율, 적분 허용 오차, 분할 깊이, 초기 광선 구간 수 변경 시 다시 생성합니다.
카메라 위치와 태양 방향 변화만으로는 다시 생성하지 않습니다.
LUT 보간 오차는 별도 오차이며 적응형 적분의 국소 허용 오차에 포함되지 않습니다.
새 HLSL 파일은 기존 XML 기반 런타임 셰이더 로더로 로드합니다.

## 검증

실행: powershell -ExecutionPolicy Bypass -File tests/ValidateAtmosphereSampling.ps1

- DXC: Atmosphere VS/PS × 매크로 ON/OFF, LUT CS 컴파일.
- CPU double 정밀도 수치 모델: 수직 기체 해석해, 균일 발광 매질, 고밀도 기준 적분, 두 분기 동등성.
- 170개 LUT 좌표/방향 사례: 최대 투과율 절대 오차 약 0.000189.
- 8개 시선의 Production/더 엄격한 직접 적분 비교: 최대 복사휘도 절대 오차 약 0.002535, 최대 상대 오차 약 0.061%.
- 평가값 재사용으로 반복 계산 감소를 검증했습니다. CPU 평가 횟수이며 GPU 시간 측정은 아닙니다.
- C++ Debug x64 빌드를 확인했습니다.

수치 스크립트는 HLSL을 GPU에서 실행하는 테스트가 아닙니다. 실제 실행 화면, GPU 시간,
모든 행성 크기/밀도 조합에 대한 영상 검증은 아직 수행하지 않았습니다.
이 테스트는 기본 지구 파라미터 중심이며 LUT 해상도나 샘플 예산의 보편적 정확도를 보장하지 않습니다.

작업 전부터 존재한 tests/GBufferRendering.md, ValidateGraphicsShaders.ps1,
ValidateMaterialBindings.ps1은 수정하지 않았습니다.
